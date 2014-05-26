#include "flagwriter.h"

#include <stdexcept>
#include <cstdio>

#include "version.h"

const uint16_t
	FlagWriter::VERSION_MINOR = 0,
	FlagWriter::VERSION_MAJOR = 1;

FlagWriter::FlagWriter(const std::string &filename, int gpsTime, size_t timestepCount, size_t gpuBoxCount, const std::vector<size_t>& subbandToGPUBoxFileIndex) :
	_timestepCount(timestepCount),
	_antennaCount(0),
	_channelCount(0),
	_channelsPerGPUBox(0),
	_polarizationCount(0),
//	_rowStride(0),
	_rowsAdded(0),
	_rowsWritten(0),
	_gpuBoxCount(gpuBoxCount),
	_gpsTime(gpsTime),
	_files(gpuBoxCount),
	_subbandToGPUBoxFileIndex(subbandToGPUBoxFileIndex)
{
	if(gpuBoxCount == 0)
		throw std::runtime_error("Flagwriter was initialized with zero gpuboxes");
		
	size_t numberPos = filename.find("%%");
	if(numberPos == std::string::npos)
		throw std::runtime_error("When writing flag files, multiple files will be written. Therefore, the name of the flagfile should contain two percent symbols (\"%%\"), e.g. \"Flagfile%%.mwaf\". These will be replaced by the gpubox number.");
	std::string name(filename);
	for(size_t i=0; i!=gpuBoxCount; ++i)
	{
		size_t gpuBoxIndex = _subbandToGPUBoxFileIndex[i] + 1;
		name[numberPos] = (char) ('0' + (gpuBoxIndex/10));
		name[numberPos+1] = (char) ('0' + (gpuBoxIndex%10));
		
		// If the file already exists, remove it
		FILE *fp = std::fopen(name.c_str(), "r");
		if (fp != NULL) {
			std::fclose(fp);
			std::remove(name.c_str());
		}
  
		int status = 0;
		if(fits_create_file(&_files[i], name.c_str(), &status))
			throwError(status, "Cannot open flag file " + name + " for writing.");
	}
}

FlagWriter::~FlagWriter()
{
	for(std::vector<fitsfile*>::iterator i=_files.begin(); i!=_files.end(); ++i)
	{
		int status = 0;
		fits_close_file(*i, &status);
	}
}

void FlagWriter::writeHeader()
{
	std::ostringstream formatOStr;
	formatOStr << _channelsPerGPUBox << 'X';
	std::string formatStr = formatOStr.str();
  const char *columnNames[] = {"FLAGS"};
  const char *columnFormats[] = {formatStr.c_str()};
  const char *columnUnits[] = {""};
	
	for(size_t i=0; i!=_gpuBoxCount; ++i)
	{
		int status = 0;
		long dimensionZero = 0;
		
		fits_create_img(_files[i], BYTE_IMG, 0, &dimensionZero, &status);
		checkStatus(status);
		
		std::ostringstream versionStr;
		versionStr << VERSION_MAJOR << '.' << VERSION_MINOR;
		fits_update_key(_files[i], TSTRING, "VERSION", const_cast<char*>(versionStr.str().c_str()), NULL, &status);
		checkStatus(status);
		
		updateIntKey(i, "GPSTIME", _gpsTime);
		updateIntKey(i, "NCHANS", _channelsPerGPUBox);
		updateIntKey(i, "NANTENNA", _antennaCount);
		updateIntKey(i, "NSCANS", _timestepCount);
		updateIntKey(i, "NPOLS", 1);
		updateIntKey(i, "GPUBOXNO", _subbandToGPUBoxFileIndex[i] + 1);
		
		fits_update_key(_files[i], TSTRING, "COTVER", const_cast<char*>(COTTER_VERSION_STR), NULL, &status);
		fits_update_key(_files[i], TSTRING, "COTVDATE", const_cast<char*>(COTTER_VERSION_DATE), NULL, &status);
		checkStatus(status);
		
		fits_create_tbl(_files[i], BINARY_TBL, 0 /*nrows*/, 1 /*tfields*/,
			const_cast<char**>(columnNames), const_cast<char**>(columnFormats),
			const_cast<char**>(columnUnits), 0, &status);
		checkStatus(status);
	}
}

void FlagWriter::setStride()
{
	if(_channelCount % _gpuBoxCount != 0)
		throw std::runtime_error("Something is wrong: number of channels requested to be written by the flagwriter is not divisable by the gpubox count");
	_channelsPerGPUBox = _channelCount / _gpuBoxCount;
	
	// we assume we write only one polarization here
//	_rowStride = (_channelsPerGPUBox + 7) / 8;
	
	_singlePolBuffer.resize(_channelsPerGPUBox);
//	_packBuffer.resize(_rowStride);
}

void FlagWriter::SetOffsetsPerGPUBox(const std::vector<int>& offsets)
{
	if(_rowsAdded != 0)
		throw std::runtime_error("SetOffsetsPerGPUBox() called after rows were added to flagwriter");
	_hduOffsets = offsets;
}

void FlagWriter::writeRow(size_t antenna1, size_t antenna2, const bool* flags)
{
	++_rowsWritten;
	const size_t baselineCount = _antennaCount * (_antennaCount+1) / 2;
	for(size_t subband=0; subband != _gpuBoxCount; ++subband)
	{
		std::vector<unsigned char>::iterator singlePolIter = _singlePolBuffer.begin();
		for(size_t i=0; i!=_channelsPerGPUBox; ++i)
		{
			*singlePolIter = *flags ? 1 : 0;
			++flags;
			
			for(size_t p=1; p!=_polarizationCount; ++p)
			{
				*singlePolIter |= *flags ? 1 : 0;
				++flags;
			}
			++singlePolIter;
		}
		
		int status = 0;
		if(_rowsWritten > _hduOffsets[_subbandToGPUBoxFileIndex[subband]] * baselineCount + 1)
		{
			size_t unalignedRow = _rowsWritten - _hduOffsets[_subbandToGPUBoxFileIndex[subband]] * baselineCount;
			fits_write_col(_files[subband], TBIT, 1 /*colnum*/, unalignedRow /*firstrow*/,
				1 /*firstelem*/, _channelsPerGPUBox /*nelements*/, &_singlePolBuffer[0], &status);
			checkStatus(status);
		}
		
		//pack(&_packBuffer[0], &_singlePolBuffer[0], _channelsPerGPUBox);
		//_files[gpuBox]->write(reinterpret_cast<char*>(&_packBuffer[0]), _rowStride);
	}
}

void FlagWriter::pack(unsigned char* output, const unsigned char* input, size_t count)
{
	const unsigned char* endPtr = input + count;
	const unsigned char* fullBytesEndPtr = input + (count/8)*8;
	while(input != fullBytesEndPtr)
	{
		*output = *input;
		++input;
		*output |= *input << 1;
		++input;
		*output |= *input << 2;
		++input;
		*output |= *input << 3;
		++input;
		*output |= *input << 4;
		++input;
		*output |= *input << 5;
		++input;
		*output |= *input << 6;
		++input;
		*output |= *input << 7;
		++input;
		++output;
	}
	if(input != endPtr)
	{
		*output = *input;
		++input;
		if(input != endPtr)
		{
			*output |= *input << 1;
			++input;
			if(input != endPtr)
			{
				*output |= *input << 2;
				++input;
				if(input != endPtr)
				{
					*output |= *input << 3;
					++input;
					if(input != endPtr)
					{
						*output |= *input << 4;
						++input;
						if(input != endPtr)
						{
							*output |= *input << 5;
							++input;
							if(input != endPtr)
							{
								*output |= *input << 6;
							}
						}
					}
				}
			}
		}
	}
}
