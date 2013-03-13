#include "rmstable.h"

#include "offringastmanerror.h"

void RMSTable::Write(std::fstream& stream)
{
	for(size_t f=0; f!=_fieldCount; ++f)
	{
		for(size_t a1=0; a1!=_antennaCount; ++a1)
		{
			stream.write(reinterpret_cast<char*>(_rmsTable[f][a1]), sizeof(RMSTable::rms_t) * _antennaCount);
			if(stream.fail())
				throw casa::DataManError("I/O error: could not write RMSTable to file");
		}
	}
}

void RMSTable::Read(std::fstream& stream)
{
	for(size_t f=0; f!=_fieldCount; ++f)
	{
		for(size_t a1=0; a1!=_antennaCount; ++a1)
		{
			stream.read(reinterpret_cast<char*>(_rmsTable[f][a1]), sizeof(RMSTable::rms_t) * _antennaCount);
			if(stream.fail())
				throw casa::DataManError("I/O error: could not read RMSTable from file");
		}
	}
}

