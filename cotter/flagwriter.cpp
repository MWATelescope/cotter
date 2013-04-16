#include "flagwriter.h"

const uint16_t
	FlagWriter::VERSION_MINOR = 0,
	FlagWriter::VERSION_MAJOR = 1;

void FlagWriter::writeHeader()
{
	struct Header header;
	header.versionMajor = VERSION_MAJOR;
	header.versionMinor = VERSION_MINOR;
	header.antennaCount = _antennaCount;
	header.channelCount = _channelCount;
	header.polarizationCount = 1;
	header.baselineSelection = 'B'; // both
	header.gpsTime = _gpsTime;
	_file.write(reinterpret_cast<char*>(&header), sizeof(Header));
}

void FlagWriter::writeRow(size_t antenna1, size_t antenna2, const bool* flags)
{
	for(std::vector<unsigned char>::iterator i=_singlePolBuffer.begin();
			i!=_singlePolBuffer.end(); ++i)
		*i = 0;
	
	std::vector<unsigned char>::iterator singlePolIter = _singlePolBuffer.begin();
	for(size_t i=0; i!=_channelCount; ++i)
	{
		*singlePolIter = flags[i] ? 1 : 0;
		++flags;
		
		for(size_t p=0; p!=_polarizationCount; ++p)
		{
			*singlePolIter |= flags[i] ? 1 : 0;
			++flags;
		}
		++singlePolIter;
	}
	
	pack(&_packBuffer[0], &_singlePolBuffer[0], _channelCount);
	
	_file.write(reinterpret_cast<char*>(&_packBuffer[0]), _rowStride);
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
