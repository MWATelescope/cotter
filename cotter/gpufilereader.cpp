#include "gpufilereader.h"
#include "progressbar.h"

#include <boost/thread/thread.hpp>

#include <complex>
#include <iostream>
#include <sstream>
#include <stdexcept>

void GPUFileReader::openFiles()
{
	int status = 0;
	bool hasStartTime = false;
	for(size_t i=0; i!=_filenames.size(); ++i)
	{
		const std::string &curFilename = _filenames[i];
		fitsfile *fptr = 0;
		if(curFilename.empty())
		{
			std::cout << "(Skipping unavailable file)\n";
			_fitsFiles.push_back(0);
		}
		else if(!fits_open_file(&fptr, curFilename.c_str(), READONLY, &status))
		{
			_fitsFiles.push_back(fptr);
			
			int hduCount;
			fits_get_num_hdus(fptr, &hduCount, &status);
			checkStatus(status);
			
			_fitsHDUCounts.push_back(hduCount);
			std::cout << "There are " << hduCount << " HDUs in file " << _filenames[i] << '\n';
			
			long thisFileTime;
			fits_read_key(fptr, TLONG, "TIME", &thisFileTime, 0, &status);
			checkStatus(status);
			
			if(!hasStartTime) 
			{
				_startTime = thisFileTime;
				hasStartTime = true;
			}
			if(_startTime != thisFileTime)
			{
				std::cout << "WARNING: file number " << (i+1) << " of current time range has different start time!\n"
					"Current file start time: " << thisFileTime << " previous file had: " << _startTime << ".\n";
			}
		}
		else {
			throwError(status, std::string("Cannot open file ") + _filenames[i]);
			exit(1);
		}
	}
	_isOpen = true;
}

void GPUFileReader::closeFiles()
{
	for(size_t i=0; i!=_fitsFiles.size(); ++i)
	{
		int status = 0;
		fitsfile *fptr = _fitsFiles[i];
		if(fptr != 0)
		{
			fits_close_file(fptr, &status);
			checkStatus(status);
		}
	}
	_fitsFiles.clear();
	_isOpen = false;
}

bool GPUFileReader::Read(size_t &bufferPos, size_t count) {
	const size_t nPol = 4;
	const size_t nBaselines = (_nAntenna + 1) * _nAntenna / 2;
	const size_t gpuMatrixSizePerFile = _nChannelsInTotal * nBaselines * nPol / _filenames.size(); // cuda matrix length per file

	_shuffleTasks.reset();
	_availableGPUMatrixBuffers.reset();
	std::vector<std::vector<std::complex<float> > > gpuMatrixBuffers(_threadCount);
	boost::thread_group threadGroup;
	for(size_t i=0; i!=_threadCount; ++i)
	{
		gpuMatrixBuffers[i].resize(gpuMatrixSizePerFile);
		_availableGPUMatrixBuffers.write(&gpuMatrixBuffers[i][0]);
		threadGroup.create_thread(boost::bind(&GPUFileReader::shuffleThreadFunc, this));
	}

	if(!_isOpen)
	{
		openFiles();
		
		size_t primary = 1; //(mode == 0) ? 0 : 1; // header to start reading
		_currentHDU = 1 + primary;
		
		findStopHDU();
	}
	
	initMapping();

	ProgressBar progressBar("Reading GPU files");
	
	size_t fileHDU = _currentHDU, fileBufferPos = bufferPos;
	for (size_t iFile = 0; iFile != _filenames.size(); ++iFile) {
		fileHDU = _currentHDU;
		fileBufferPos = bufferPos;
		while (fileHDU <= _stopHDU && fileBufferPos < count)
		{
			progressBar.SetProgress(fileHDU + iFile*_stopHDU, _stopHDU*_filenames.size());

			fitsfile *fptr = _fitsFiles[iFile];

			if(fptr != 0)
			{
				int status = 0, hduType = 0;
				fits_movabs_hdu(fptr, fileHDU, &hduType, &status);
				checkStatus(status);
				if (hduType == BINARY_TBL) {
					throw std::runtime_error("GPU file seems not to contain image headers; format not understood.");
				}
				else {

					long fpixel = 1;
					float nullval = 0;
					int anynull = 0x0;
					int nfound = 0;
					long naxes[2];

					fits_read_keys_lng(fptr, "NAXIS", 1, 2, naxes, &nfound, &status);
					checkStatus(status);

					size_t channelsInFile = naxes[1];
					size_t baselTimesPolInFile = naxes[0];

					if(_nChannelsInTotal != (channelsInFile*_filenames.size())) {
						std::stringstream s;
						s << "Number of GPU files (" << _filenames.size() << ") in time range x row count of image chunk in file (" << channelsInFile << ") != "
						<< "total channels count (" << _nChannelsInTotal << "): are the FITS files the dimension you expected them to be?";
						throw std::runtime_error(s.str());
					}
					// Test the first axis; note that we assert the number of floats, not complex, hence the factor of two.
					if(baselTimesPolInFile != nBaselines * nPol * 2) {
						std::stringstream s;
						s << "Unexpected number of visibilities in axis of GPU file. Expected=" << (nBaselines*nPol*2) << ", actual=" << baselTimesPolInFile;
						throw std::runtime_error(s.str());
					}

					std::complex<float> *matrixPtr;
					_availableGPUMatrixBuffers.read(matrixPtr);
					fits_read_img(fptr, TFLOAT, fpixel, channelsInFile * baselTimesPolInFile, &nullval, (float *) matrixPtr, &anynull, &status);
					checkStatus(status);
					
					ShuffleTask shuffleTask;
					shuffleTask.iFile = iFile;
					shuffleTask.channelsInFile = channelsInFile;
					shuffleTask.fileBufferPos = fileBufferPos;
					shuffleTask.gpuMatrix = matrixPtr;
					_shuffleTasks.write(shuffleTask);
				}
			} else {
				// this file is not available
			}
			
			++fileHDU;
			++fileBufferPos;
		}
	}
	
	_shuffleTasks.write_end();
	threadGroup.join_all();
	
	_currentHDU = fileHDU;
	bufferPos = fileBufferPos;
	
	bool moreAvailable = _currentHDU <= _stopHDU;
	if(!moreAvailable)
		closeFiles();
	return moreAvailable;
}

void GPUFileReader::shuffleThreadFunc()
{
	ShuffleTask task;
	while(_shuffleTasks.read(task))
	{
		shuffleBuffer(task.iFile, task.channelsInFile, task.fileBufferPos, task.gpuMatrix);
		_availableGPUMatrixBuffers.write(task.gpuMatrix);
	}
}

void GPUFileReader::shuffleBuffer(size_t iFile, size_t channelsInFile, size_t fileBufferPos, const std::complex<float> *gpuMatrix)
{
	const size_t nPol = 4;
	const size_t nBaselines = (_nAntenna + 1) * _nAntenna / 2;
	
	/** Note that the following antenna indices do not refer to the actual
	* antennae indices, but to correlator input indices. These need to be
	* mapped to the actual antenna indices. */
	size_t correlationIndex = 0;
	for(size_t antenna1=0; antenna1!=_nAntenna; ++antenna1)
	{
		for(size_t antenna2=0; antenna2<=antenna1; ++antenna2)
		{
			size_t channelStart = iFile * channelsInFile;
			size_t channelEnd = (iFile+1) * channelsInFile;
			size_t index = correlationIndex * nPol;
			// Because possibly antenna2 <= antenna1 in the GPU file, and Casa MS expects it the other way
			// around, we change the order and take the complex conjugates later.
			BaselineBuffer &buffer = getMappedBuffer(antenna2, antenna1);
			size_t destChanIndex = fileBufferPos + channelStart * _bufferSize;
			for(size_t ch=channelStart; ch!=channelEnd; ++ch)
			{
				const std::complex<float> *dataPtr = &gpuMatrix[index];
				
				*(buffer.real[0] + destChanIndex) = dataPtr->real();
				*(buffer.imag[0] + destChanIndex) = dataPtr->imag();
				++dataPtr;
				
				*(buffer.real[2] + destChanIndex) = dataPtr->real();
				*(buffer.imag[2] + destChanIndex) = dataPtr->imag();
				++dataPtr;
				
				*(buffer.real[1] + destChanIndex) = dataPtr->real();
				*(buffer.imag[1] + destChanIndex) = dataPtr->imag();
				++dataPtr;
				
				*(buffer.real[3] + destChanIndex) = dataPtr->real();
				*(buffer.imag[3] + destChanIndex) = dataPtr->imag();

				index += nBaselines * nPol;
				destChanIndex += _bufferSize;
			}
			++correlationIndex;
		}
	}
}

	// Check the number of HDUs in each file. Only extract the amount of time
	// that there is actually data for in all files.
void GPUFileReader::findStopHDU()
{
	if(!_fitsHDUCounts.empty())
	{
		std::vector<size_t>::const_iterator iter = _fitsHDUCounts.begin();
		_stopHDU = *iter;
		++iter;
		while(iter != _fitsHDUCounts.end())
		{
			if(*iter != _stopHDU)
			{
				if(*iter < _stopHDU)
					_stopHDU = *iter;
				std::cout << "Warning: GPU files with same observation time range have unequal amount of HDU's.\n";
			}
			++iter;
		}
	}
}

void GPUFileReader::initMapping()
{
	initializePFBMapping();
	_isConjugated.resize(_nAntenna*_nAntenna*4);
	for(size_t a1 = 0; a1 != _nAntenna; ++a1) {
		for(size_t a2 = a1; a2 != _nAntenna; ++a2) {
			for(size_t p1 = 0; p1 != 2; ++p1) {
				for(size_t p2 = 0; p2 != 2; ++p2) {
					size_t pfbInp1 = pfb_output_to_input[a1 * 2 + p1];
					size_t pfbInp2 = pfb_output_to_input[a2 * 2 + p2];
					
					size_t actualOut1 = _corrInputToOutput[pfbInp1];
					size_t actualOut2 = _corrInputToOutput[pfbInp2];
					size_t actA1 = actualOut1/2;
					size_t actA2 = actualOut2/2;
					size_t actP1 = actualOut1%2;
					size_t actP2 = actualOut2%2;
					
					// Note that while reading, the antenna indices are reversed
					// again. Therefore, if the antenna indices are in the right
					// order here, we need to conjugate the visibility.
					size_t sourceIndex1 = a1*2 + p1;
					size_t sourceIndex2 = a2*2 + p2;
					bool isConjugated = 
						(actualOut1 < actualOut2 && sourceIndex1 < sourceIndex2) ||
						(actualOut1 > actualOut2 && sourceIndex1 > sourceIndex2);
					
					if(actA1 <= actA2)
					{
						size_t conjIndex = (actA1 * 2 + actP1) * _nAntenna * 2 + (actA2 * 2 + actP2);
						_isConjugated[conjIndex] = isConjugated;
						getMappedBuffer(a1, a2).real[p1 * 2 + p2] = getBuffer(actA1, actA2).real[actP1 * 2 + actP2];
						getMappedBuffer(a1, a2).imag[p1 * 2 + p2] = getBuffer(actA1, actA2).imag[actP1 * 2 + actP2];
					} else {
						size_t conjIndex = (actA2 * 2 + actP2) * _nAntenna * 2 + (actA1 * 2 + actP1);
						_isConjugated[conjIndex] = isConjugated;
						getMappedBuffer(a1, a2).real[p1 * 2 + p2] = getBuffer(actA2, actA1).real[actP2 * 2 + actP1];
						getMappedBuffer(a1, a2).imag[p1 * 2 + p2] = getBuffer(actA2, actA1).imag[actP2 * 2 + actP1];
					}
				}
			}
		}
	}
}

void GPUFileReader::initializePFBMapping()
{
	//       Output matrix has ordering
	//       [channel][station][station][polarization][polarization][complexity]
	const size_t nPFB = 4;
	pfb_output_to_input.resize(nPFB * 64);
	for(size_t p=0;p<nPFB;p++) {
		for(size_t inp1=0;inp1<64;inp1++) {
			pfb_output_to_input[(p*64) + inp1] = single_pfb_output_to_input[inp1] + p*64;
		}
	}
}

const int GPUFileReader::single_pfb_output_to_input[64] = {
		0,
		16,
		32,
		48,
		1,
		17,
		33,
		49,
		2,
		18,
		34,
		50,
		3,
		19,
		35,
		51,
		4,
		20,
		36,
		52,
		5,
		21,
		37,
		53,
		6,
		22,
		38,
		54,
		7,
		23,
		39,
		55,
		8,
		24,
		40,
		56,
		9,
		25,
		41,
		57,
		10,
		26,
		42,
		58,
		11,
		27,
		43,
		59,
		12,
		28,
		44,
		60,
		13,
		29,
		45,
		61,
		14,
		30,
		46,
		62,
		15,
		31,
		47,
		63
};
