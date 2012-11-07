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
	
	size_t nBaselines = (_nAntenna + 1) * _nAntenna / 2;
	size_t gpuMatrixSizePerFile = _nChannelsInTotal * nBaselines / _filenames.size(); // cuda matrix length per file

	std::vector<std::complex<float> > gpuMatrix(gpuMatrixSizePerFile);
	
	if(!_isOpen)
	{
		openFiles();
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
				const size_t nPol = 4;

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
				
				size_t baselineIndex = 0;
				for(size_t antenna1=0; antenna1!=_nAntenna; ++antenna1)
				{
					for(size_t antenna2=0; antenna2<=antenna1; ++antenna2)
					{
						size_t channelStart = iFile * channelsInFile;
						size_t channelEnd = (iFile+1) * channelsInFile;
						size_t index = baselineIndex * nPol;
						// Because antenna2 <= antenna1 in the GPU file, and Casa MS expects it the other way
						// around, we change the order and take the complex conjugates.
						BaselineBuffer &destBuffer = getBuffer(antenna2, antenna1);
						size_t destChanIndex = bufferPos + channelStart * destBuffer.nElementsPerRow;
						for(size_t ch=channelStart; ch!=channelEnd; ++ch)
						{
							std::complex<float> *dataPtr = &gpuMatrix[index];
							
							// Take conjugate during assignment.
							*(destBuffer.realXX + destChanIndex) = dataPtr->real();
							*(destBuffer.imagXX + destChanIndex) = -dataPtr->imag();
							++dataPtr;
							*(destBuffer.realXY + destChanIndex) = dataPtr->real();
							*(destBuffer.imagXY + destChanIndex) = -dataPtr->imag();
							++dataPtr;
							*(destBuffer.realYX + destChanIndex) = dataPtr->real();
							*(destBuffer.imagYX + destChanIndex) = -dataPtr->imag();
							++dataPtr;
							*(destBuffer.realYY + destChanIndex) = dataPtr->real();
							*(destBuffer.imagYY + destChanIndex) = -dataPtr->imag();
							
							index += nBaselines * nPol;
							destChanIndex += destBuffer.nElementsPerRow;
						}
						++baselineIndex;
						
					}
				}
			}
		}

		++_currentHDU;
		++bufferPos;
	}
	
	closeFiles();
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

