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
	FlagReader(const std::string& templateName, const std::vector<int>& hduOffsets, size_t gpuBoxCount, const std::vector<size_t>& subbandToGPUBoxFileIndex)
		:
		_hduOffsets(hduOffsets),
		_files(gpuBoxCount),
		_colNums(gpuBoxCount),
		_subbandToGPUBoxFileIndex(subbandToGPUBoxFileIndex)
	{
		size_t numberPos = templateName.find("%%");
		if(numberPos == std::string::npos)
			throw std::runtime_error("When reading flag files, the name of the flag file should be specified with two percent symbols (\"%%\"), e.g. \"Flagfile%%.mwaf\". These will be replaced by the gpubox number.");
		std::string name(templateName);

		int status = 0;
		for(size_t i=0; i!=gpuBoxCount; ++i)
		{
			size_t gpuBoxNumber = _subbandToGPUBoxFileIndex[i] + 1;
			name[numberPos] = (char) ('0' + (gpuBoxNumber/10));
			name[numberPos+1] = (char) ('0' + (gpuBoxNumber%10));
			if(fits_open_file(&_files[i], name.c_str(), READONLY, &status))
				throwError(status, std::string("Cannot open file ") + name);
			
			int nChans, nAnt, nScans;
			fits_read_key(_files[i], TINT, "NCHANS", &nChans, 0 /*comment*/, &status);
			fits_read_key(_files[i], TINT, "NANTENNA", &nAnt, 0 /*comment*/, &status);
			fits_read_key(_files[i], TINT, "NSCANS", &nScans, 0 /*comment*/, &status);
			checkStatus(status);
			if(i==0)
			{
				_channelsPerGPUBox = nChans;
				_antennaCount = nAnt;
				_baselineCount = nAnt * (nAnt+1) / 2;
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
			fits_movabs_hdu(_files[i], 2, &hduType, &status);
			checkStatus(status);
			fits_get_colnum(_files[i], CASESEN, const_cast<char*>("FLAGS"), &_colNums[i], &status);
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
		for(size_t i=0; i!=_files.size(); ++i)
		{
			size_t row = (timestep + _hduOffsets[_subbandToGPUBoxFileIndex[i]]) * _baselineCount + baseline + 1;
			fits_read_col(_files[i], TBIT, /*colnum*/ _colNums[i], /*firstrow*/ row, /*firstelem*/ 1,
       /*nelements*/ _channelsPerGPUBox, /*(*)nulval*/ 0, &_buffer[0], 0 /*(*)anynul*/, &status);
			for(size_t ch=0; ch!=_channelsPerGPUBox; ++ch)
			{
				bufferStartPos[ch*bufferStride] = bool(_buffer[ch]);
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
};

#endif
