#include "flagwriter.h"
#include <stdexcept>

const uint16_t
	FlagWriter::VERSION_MINOR = 0,
	FlagWriter::VERSION_MAJOR = 1;

FlagWriter::FlagWriter(const std::string &filename, int gpsTime, size_t timestepCount, size_t gpuBoxCount) :
	_timestepCount(timestepCount),
	_antennaCount(0),
	_channelCount(0),
	_channelsPerGPUBox(0),
	_polarizationCount(0),
	_rowStride(0),
	_rowCount(0),
	_gpuBoxCount(gpuBoxCount),
	_gpsTime(gpsTime),
	_files(gpuBoxCount)
{
	if(gpuBoxCount == 0)
		throw std::runtime_error("Flagwriter was initialized with zero gpuboxes");
		
	size_t numberPos = filename.find("%%");
	if(numberPos == std::string::npos)
		throw std::runtime_error("When writing flag files, multiple files will be written. Therefore, the name of the flagfile should contain two percent symbols (\"%%\"), e.g. \"Flagfile%%.mwaf\". These will be replaced by the gpubox(/subband) number.");
	std::string name(filename);
	for(size_t i=0; i!=gpuBoxCount; ++i)
	{
		name[numberPos] = (char) ('0' + (i/10));
		name[numberPos+1] = (char) ('0' + (i%10));
		_files[i] = new std::ofstream(name.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
		if(!_files[i]->good())
			throw std::runtime_error(std::string("Could not open flag file \"") + name + "\" for writing.");
	}
}

FlagWriter::~FlagWriter()
{
	for(std::vector<std::ofstream*>::iterator i=_files.begin(); i!=_files.end(); ++i)
		delete *i;
}

void FlagWriter::writeHeader()
{
	struct Header header;
	header.fileIdentifier[0] = 'M';
	header.fileIdentifier[1] = 'W';
	header.fileIdentifier[2] = 'A';
	header.fileIdentifier[3] = 'F';
	header.versionMajor = VERSION_MAJOR;
	header.versionMinor = VERSION_MINOR;
	header.timestepCount = _timestepCount;
	header.antennaCount = _antennaCount;
	header.channelCount = _channelsPerGPUBox;
	header.polarizationCount = 1;
	header.baselineSelection = 'B'; // both
	header.gpsTime = _gpsTime;
	for(size_t i=0; i!=_gpuBoxCount; ++i)
	{
		header.gpuBoxIndex = i;
		_files[i]->write(reinterpret_cast<char*>(&header), sizeof(Header));
	}
}

void FlagWriter::setStride()
{
	if(_channelCount % _gpuBoxCount != 0)
		throw std::runtime_error("Something is wrong: number of channels requested to be written by the flagwriter is not divisable by the gpubox count");
	_channelsPerGPUBox = _channelCount / _gpuBoxCount;
	
	// we assume we write only one polarization here
	_rowStride = (_channelsPerGPUBox + 7) / 8;
	
	_singlePolBuffer.resize(_channelsPerGPUBox);
	_packBuffer.resize(_rowStride);
}

void FlagWriter::writeRow(size_t antenna1, size_t antenna2, const bool* flags)
{
	for(size_t gpuBox=0; gpuBox != _gpuBoxCount; ++gpuBox)
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
		
		pack(&_packBuffer[0], &_singlePolBuffer[0], _channelsPerGPUBox);
		
		_files[gpuBox]->write(reinterpret_cast<char*>(&_packBuffer[0]), _rowStride);
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
