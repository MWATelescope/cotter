#ifndef MWA_INPUT_H
#define MWA_INPUT_H

#include <cstring>

struct MWAInput
{
	MWAInput() :
	inputIndex(0), antennaIndex(0), cableLenDelta(0.0), polarizationIndex(0), isFlagged(false), slot(0)
	{
		for(size_t pfbBank=0; pfbBank!=24; ++pfbBank)
			pfbGains[pfbBank] = 0.0;
	}
	
	MWAInput(const MWAInput& source) :
	inputIndex(source.inputIndex), antennaIndex(source.antennaIndex), cableLenDelta(source.cableLenDelta),
	polarizationIndex(source.polarizationIndex), isFlagged(source.isFlagged), slot(source.slot)
	{
		for(size_t pfbBank=0; pfbBank!=24; ++pfbBank)
			pfbGains[pfbBank] = source.pfbGains[pfbBank];
	}
	
	MWAInput& operator=(const MWAInput& source)
	{
		inputIndex = source.inputIndex;
		antennaIndex = source.antennaIndex;
		cableLenDelta = source.cableLenDelta;
		polarizationIndex = source.polarizationIndex;
		isFlagged = source.isFlagged;
		slot = source.slot;
		for(size_t pfbBank=0; pfbBank!=24; ++pfbBank)
			pfbGains[pfbBank] = source.pfbGains[pfbBank];
		return *this;
	}
	
	size_t inputIndex;
	size_t antennaIndex;
	double cableLenDelta;
	unsigned polarizationIndex;
	bool isFlagged;
	size_t slot;
	double pfbGains[24];
private:
};

#endif
