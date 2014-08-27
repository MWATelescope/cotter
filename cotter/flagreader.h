#ifndef FLAG_READER_H
#define FLAG_READER_H

#include "fitsuser.h"

#include <fitsio.h>

#include <string>
#include <vector>
#include <stdexcept>
#include <cmath>

class FlagReader : private FitsUser
{
public:
	FlagReader(const std::string& templateName, const std::vector<int>& hduOffsetsPerGPUBox, const std::vector<size_t>& subbandToGPUBoxFileIndex, size_t sbStart, size_t sbEnd)
		:
		_hduOffsets(hduOffsetsPerGPUBox),
		_files(sbEnd - sbStart),
		_colNums(sbEnd - sbStart),
		_subbandToGPUBoxFileIndex(subbandToGPUBoxFileIndex),
		_sbStart(sbStart),
		_sbEnd(sbEnd)
	{
		size_t numberPos = templateName.find("%%");
		if(numberPos == std::string::npos)
			throw std::runtime_error("When reading flag files, the name of the flag file should be specified with two percent symbols (\"%%\"), e.g. \"Flagfile%%.mwaf\". These will be replaced by the gpubox number.");
		std::string name(templateName);

		int status = 0;
		for(size_t sb=_sbStart; sb!=_sbEnd; ++sb)
		{
			size_t fileIndex = sb - _sbStart;
			size_t gpuBoxNumber = _subbandToGPUBoxFileIndex[sb] + 1;
			name[numberPos] = (char) ('0' + (gpuBoxNumber/10));
			name[numberPos+1] = (char) ('0' + (gpuBoxNumber%10));
			if(fits_open_file(&_files[fileIndex], name.c_str(), READONLY, &status))
				throwError(status, std::string("Cannot open file ") + name);
			
			int nChans, nAnt, nScans;
			fits_read_key(_files[fileIndex], TINT, "NCHANS", &nChans, 0 /*comment*/, &status);
			fits_read_key(_files[fileIndex], TINT, "NANTENNA", &nAnt, 0 /*comment*/, &status);
			fits_read_key(_files[fileIndex], TINT, "NSCANS", &nScans, 0 /*comment*/, &status);
			checkStatus(status);
			if(sb==_sbStart)
			{
				_channelsPerGPUBox = nChans;
				_antennaCount = nAnt;
				_baselineCount = (nAnt * (nAnt+1)) / 2;
				_scanCount = nScans;
				_buffer.resize(_channelsPerGPUBox);
			}
			else {
				if(nChans != int(_channelsPerGPUBox))
					throw std::runtime_error("The flag files have an inconsistent number of frequency channels");
				if(nAnt != int(_antennaCount))
					throw std::runtime_error("The flag files have an inconsistent number of antennas");
			}
			
			int hduType;
			fits_movabs_hdu(_files[fileIndex], 2, &hduType, &status);
			checkStatus(status);
			fits_get_colnum(_files[fileIndex], CASESEN, const_cast<char*>("FLAGS"), &_colNums[fileIndex], &status);
			checkStatus(status);
		}
	}
	
	~FlagReader()
	{
		for(std::vector<fitsfile*>::iterator i=_files.begin(); i!=_files.end(); ++i)
		{
			int status = 0;
			fits_close_file(*i, &status);
		}
	}
	
	void Read(size_t timestep, size_t baseline, bool* bufferStartPos, size_t bufferStride)
	{
		int status = 0;
		for(size_t subband=_sbStart; subband!=_sbEnd; ++subband)
		{
			size_t fileIndex = subband - _sbStart;
			size_t row = (timestep + _hduOffsets[_subbandToGPUBoxFileIndex[subband]]) * _baselineCount + baseline + 1;
			fits_read_col(_files[fileIndex], TBIT, /*colnum*/ _colNums[fileIndex], /*firstrow*/ row, /*firstelem*/ 1,
       /*nelements*/ _channelsPerGPUBox, /*(*)nulval*/ 0, &_buffer[0], 0 /*(*)anynul*/, &status);
			for(size_t ch=0; ch!=_channelsPerGPUBox; ++ch)
			{
				size_t channelIndex = ch + fileIndex*_channelsPerGPUBox;
				bufferStartPos[channelIndex*bufferStride] = bool(_buffer[ch]);
			}
		}
	}
	
	size_t ChannelsPerGPUBox() const { return _channelsPerGPUBox; }
	size_t AntennaCount() const { return _antennaCount; }
	size_t ScanCount() const { return _scanCount; }
private:
	std::vector<int> _hduOffsets;
	std::vector<fitsfile*> _files;
	std::vector<int> _colNums;
	std::vector<char> _buffer;
	const std::vector<size_t> _subbandToGPUBoxFileIndex;
	size_t _channelsPerGPUBox, _antennaCount, _baselineCount, _scanCount;
	size_t _sbStart, _sbEnd;
};

#endif
