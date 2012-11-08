#include "gpufilereader.h"

#include <complex>
#include <iostream>
#include <sstream>
#include <stdexcept>

inline void GPUFileReader::checkStatus(int status)
{
	if(status != 0)
	{
		throwError(status);
	}
}

void GPUFileReader::throwError(int status, const std::string &msg)
{
	std::stringstream msgStr;
	
	if(!msg.empty()) msgStr << msg << ". ";
	
	char statusStr[FLEN_STATUS], errMsg[FLEN_ERRMSG];
	fits_get_errstatus(status, statusStr);
	msgStr << "CFitsio reported error while reading GPU files. Status = " << status << ": " << statusStr;

	if(fits_read_errmsg(errMsg))
	{
		msgStr << ". Error message stack: " << errMsg << '.';

		while( fits_read_errmsg(errMsg) )
			msgStr << ' ' << errMsg << '.';
	}
	throw std::runtime_error(msgStr.str());
}

void GPUFileReader::openFiles()
{
	int status = 0;

	for(size_t i=0; i!=_filenames.size(); ++i)
	{
		fitsfile *fptr;
		if(!fits_open_file(&fptr, _filenames[i].c_str(), READONLY, &status))
		{
			_fitsFiles.push_back(fptr);
			
			int hduCount;
			fits_get_num_hdus(fptr, &hduCount, &status);
			checkStatus(status);
			
			_fitsHDUCounts.push_back(hduCount);
			std::cout << "There are " << hduCount << " HDUs in file " << _filenames[i] << '\n';
		}
		else {
			throwError(status, std::string("Cannot open file ") + _filenames[i]);
			exit(1);
		}
	}
}

void GPUFileReader::closeFiles()
{
	for(size_t i=0; i!=_fitsFiles.size(); ++i)
	{
		int status = 0;
		fitsfile *fptr = _fitsFiles[i];
		fits_close_file(fptr, &status);
		checkStatus(status);
	}
	_fitsFiles.clear();
}

bool GPUFileReader::Read(size_t &bufferPos) {
	size_t startScan = 0;

	_nAntenna = 32;
	_nChannelsInTotal = 1536*2;
	
	const size_t nPol = 4;
	const size_t nBaselines = (_nAntenna + 1) * _nAntenna / 2;
	const size_t gpuMatrixSizePerFile = _nChannelsInTotal * nBaselines * nPol / _filenames.size(); // cuda matrix length per file

	std::vector<std::complex<float> > gpuMatrix(gpuMatrixSizePerFile);
	
	if(!_isOpen)
	{
		openFiles();
		initMapping();
		_isOpen = true;
		
		size_t primary = 1; //(mode == 0) ? 0 : 1; // header to start reading
		_currentHDU = 1 + startScan + primary;
	}

	findStopHDU();
	std::cout << "Start HDU: " << _currentHDU << ", stop HDU: " << _stopHDU << '\n';

	while (_currentHDU <= _stopHDU && bufferPos < _bufferSize) {
		for (size_t iFile = 0; iFile != _filenames.size(); ++iFile) {
			std::cout << "Reading file " << _filenames[iFile] << " for HDU " << _currentHDU << '\n';

			fitsfile *fptr = _fitsFiles[iFile];

			int status = 0, hduType = 0;
			fits_movabs_hdu(fptr, _currentHDU, &hduType, &status);
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
					<< "total channels count (" << _nChannelsInTotal << ": are the FITS files the dimension you expected them to be?";
					throw std::runtime_error(s.str());
				}
				// Test the first axis; note that we assert the number of floats, not complex, hence the factor of two.
				if(baselTimesPolInFile != nBaselines * nPol * 2) {
					std::stringstream s;
					s << "Unexpected number of visibilities in axis of GPU file. Expected=" << (nBaselines*nPol*2) << ", actual=" << baselTimesPolInFile;
					throw std::runtime_error(s.str());
				}

				std::complex<float> *matrixPtr = &gpuMatrix[0];
				fits_read_img(fptr, TFLOAT, fpixel, channelsInFile * baselTimesPolInFile, &nullval, (float *) matrixPtr, &anynull, &status);
				checkStatus(status);
				
				
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
						// around, we change the order and take the complex conjugates later. TODO
						BaselineBuffer &buffer = getMappedBuffer(antenna2, antenna1);
						size_t destChanIndex = bufferPos + channelStart * _bufferSize;
						//std::cout << antenna1 << 'x' << antenna2 << "--" << index << '\n';
						for(size_t ch=channelStart; ch!=channelEnd; ++ch)
						{
							std::complex<float> *dataPtr = &gpuMatrix[index];
							
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
		}

		++_currentHDU;
		++bufferPos;
	}
	
	closeFiles();
	return false; // TODO
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
				_stopHDU = *iter;
				std::cout << "Warning: GPU files with same observation time range have unequal amount of HDU's.\n";
			}
			++iter;
		}
	}
}

void GPUFileReader::initMapping()
{
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
					
					std::cout
					<< a1 << 'x' << a2 << ':' << p1 << 'x' << p2 << " -> "
					<< (pfbInp1/2) << 'x' << (pfbInp2/2) << ':' << (pfbInp1%2) << 'x' << (pfbInp2%2) << " -> " 
					<< actA1 << 'x' << actA2 << ':' << actP1 << 'x' << actP2 << '\n';
					
					// Note that while reading, the antenna indices are reversed
					// again. Therefore, if the antenna indices are in the right
					// order here, we need to conjugate the visibility.
					if(actA1 <= actA2)
					{
						size_t conjIndex = (actA1 * 2 + actP1) * _nAntenna * 2 + (actA2 * 2 + actP2);
						_isConjugated[conjIndex] = (a1!=a2);
						getMappedBuffer(a1, a2).real[p1 * 2 + p2] = getBuffer(actA1, actA2).real[actP1 * 2 + actP2];
						getMappedBuffer(a1, a2).imag[p1 * 2 + p2] = getBuffer(actA1, actA2).imag[actP1 * 2 + actP2];
					} else {
						size_t conjIndex = (actA2 * 2 + actP2) * _nAntenna * 2 + (actA1 * 2 + actP1);
						_isConjugated[conjIndex] = false;
						getMappedBuffer(a1, a2).real[p1 * 2 + p2] = getBuffer(actA2, actA1).real[actP2 * 2 + actP1];
						getMappedBuffer(a1, a2).imag[p1 * 2 + p2] = getBuffer(actA2, actA1).imag[actP2 * 2 + actP1];
					}
				}
			}
		}
	}
}

const int GPUFileReader::pfb_output_to_input[64] = {
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
