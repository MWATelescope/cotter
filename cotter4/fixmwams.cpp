#include "mwams.h"
#include "mwaconfig.h"
#include "version.h"

#include <iostream>

int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		std::cout <<
			"fixmwams will add several mwa-specific keywords to a measurement set that are normally added by Cotter.\n"
			"This is for example useful when outputting to uvfits file and importing this later into a measurement set.\n\n"
			"Syntax: fixmwams <ms> <metafitsfile>\n";
		return -1;
	}
	const char* outputFilename = argv[1];
	const char* metaFitsFilename = argv[2];
	
	MWAMS mwaMs(outputFilename);
	mwaMs.InitializeMWAFields();
	MWAConfig mwaConfig;
	mwaConfig.ReadMetaFits(metaFitsFilename, false);
	
	size_t nAnt = mwaConfig.NAntennae();
	for(size_t i=0; i!=nAnt; ++i)
	{
		const MWAAntenna &ant = mwaConfig.Antenna(i);
		const MWAInput &inpX = mwaConfig.AntennaXInput(i);
		const MWAInput &inpY = mwaConfig.AntennaYInput(i);
		
		MWAMS::MWAAntennaInfo antennaInfo;
		antennaInfo.inputX = inpX.inputIndex;
		antennaInfo.inputY = inpY.inputIndex;
		antennaInfo.tileNr = ant.tileNumber;
		antennaInfo.receiver = ant.receiver;
		antennaInfo.slotX = inpX.slot;
		antennaInfo.slotY = inpY.slot;
		antennaInfo.cableLengthX = inpX.cableLenDelta;
		antennaInfo.cableLengthY = inpY.cableLenDelta;
		
		mwaMs.UpdateMWAAntennaInfo(i, antennaInfo);
	}
	
	mwaMs.UpdateMWAFieldInfo(mwaConfig.HeaderExt().hasCalibrator);
	
	MWAMS::MWAObservationInfo obsInfo;
	obsInfo.gpsTime = mwaConfig.HeaderExt().gpsTime;
	obsInfo.filename = mwaConfig.HeaderExt().filename;
	obsInfo.observationMode = mwaConfig.HeaderExt().mode;
	obsInfo.rawFileCreationDate = 0;
	obsInfo.flagWindowSize = 0;
	obsInfo.dateRequested = mwaConfig.HeaderExt().dateRequestedMJD*86400.0;
	mwaMs.UpdateMWAObservationInfo(obsInfo);
	
	mwaMs.UpdateSpectralWindowInfo(mwaConfig.CentreSubbandNumber());
	
	mwaMs.WriteMWATilePointingInfo(mwaConfig.Header().GetStartDateMJD()*86400.0,
		mwaConfig.Header().GetDateLastScanMJD()*86400.0, mwaConfig.HeaderExt().delays,
		mwaConfig.HeaderExt().tilePointingRARad, mwaConfig.HeaderExt().tilePointingDecRad);
	
	for(int i=0; i!=24; ++i)
		mwaMs.WriteMWASubbandInfo(i, mwaConfig.HeaderExt().subbandGains[i], false);
	
	mwaMs.WriteMWAKeywords(mwaConfig.HeaderExt().metaDataVersion, mwaConfig.HeaderExt().mwaPyVersion, COTTER_VERSION_STR, COTTER_VERSION_DATE);
}
