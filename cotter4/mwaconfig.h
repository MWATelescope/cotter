#ifndef MWA_CONFIG_H
#define MWA_CONFIG_H

#include <vector>
#include <cstring>
#include <string>
#include <map>

#include "mwainput.h"

struct MWAHeader
{
	public:
		MWAHeader();
		
		size_t nInputs;                 // the number of inputs to the correlator
		size_t nScans;                  // the number of time samples
		size_t nChannels;               // the number of spectral channels
		enum CorrType { None, CrossCorrelation, AutoCorrelation, BothCorrelations } correlationType;
		double integrationTime;         // per time sample, in seconds
		double centralFrequencyMHz;     // observing central frequency and 
		double bandwidthMHz;            // bandwidth (MHz)
		double raHrs, decDegs;          // ra,dec of phase centre.
		double haHrsStart;              // the HA of the phase center at the start of the integration
		double refEl, refAz;            // the el/az of the normal to the plane of the array (radian)
		int    year, month, day;        // date/time in UTC.
		int    refHour, refMinute;
		double refSecond;
		//bool   invertFrequency;         // flag to indicate that freq decreases with increasing channel number.
		bool   conjugate;               // conjugate the final vis to correct for any sign errors
		bool   geomCorrection;          // apply geometric phase correction
		std::string fieldName;
		std::string polProducts;
		
		double dateFirstScanMJD;        // The central time of the first time step. Not in header file, derived from it.
		
		double GetDateLastScanMJD() const
		{
			return dateFirstScanMJD + (integrationTime/86400.0)*nScans;
		}
		double GetStartDateMJD() const;
		double GetDateFirstScanFromFields() const;
		
		void Validate(bool lockPointing);
	private:
		MWAHeader(const MWAHeader &) { }
		void operator=(const MWAHeader &) { }
};

struct MWAHeaderExt
{
	MWAHeaderExt();
	
	int gpsTime;
	std::string observerName, projectName, gridName, mode, filename, mwaPyVersion, mwaPyDate, metaDataVersion;
	int delays[16], subbandGains[24], subbandNumbers[24];
	bool hasCalibrator, hasGlobalSubbandGains;
	int centreSBNumber;
	//double fiberFactor;
	double tilePointingRARad, tilePointingDecRad;
	double dateRequestedMJD;
private:
	MWAHeaderExt(const MWAHeaderExt &source) { }
	void operator=(const MWAHeaderExt &source) { }
};

struct MWAAntenna
{
	public:
		MWAAntenna() : tileNumber(0), receiver(0) { }
		MWAAntenna(const MWAAntenna& source) :
			name(source.name), stationIndex(source.stationIndex),
			tileNumber(source.tileNumber), receiver(source.receiver)
		{
			position[0] = source.position[0]; position[1] = source.position[1]; position[2] = source.position[2];
		}
		void operator=(const MWAAntenna& source)
		{
			name = source.name;
			position[0] = source.position[0]; position[1] = source.position[1]; position[2] = source.position[2];
			stationIndex = source.stationIndex;
			tileNumber = source.tileNumber;
			receiver = source.receiver;
		}
		
		std::string name;
		double position[3];
		size_t stationIndex;
		size_t tileNumber;
		size_t receiver;
		
	private:
};

class MWAConfig
{
	public:
		void ReadHeader(const std::string& filename, bool lockPointing);
		void ReadInputConfig(const std::string& filename);
		void ReadAntennaPositions(const std::string& filename);
		void ReadMetaFits(const std::string& filename, bool lockPointing);
		
		void CheckSetup();
		
		const MWAInput &Input(const size_t index) const { return _inputs[index]; }
		const MWAInput &AntennaXInput(const size_t antennaIndex) const { return *_antennaXInputs.find(antennaIndex)->second; }
		const MWAInput &AntennaYInput(const size_t antennaIndex) const { return *_antennaYInputs.find(antennaIndex)->second; }
		const MWAAntenna &Antenna(const size_t antennaIndex) const {
			return _antennae[antennaIndex];
		}
		size_t NAntennae() const { return _antennae.size(); }
		
		const MWAHeader &Header() const { return _header; }
		const MWAHeaderExt &HeaderExt() const { return _headerExt; }
		
		MWAInput &InputRW(const size_t index) { return _inputs[index]; }
		MWAHeader &HeaderRW() { return _header; }
		MWAHeaderExt &HeaderExtRW() { return _headerExt; }
		
		double ChannelFrequencyHz(size_t channelIndex) const
		{
			const double channelWidthMHz = _header.bandwidthMHz / _header.nChannels;
			return (_header.centralFrequencyMHz +
				(channelIndex - _header.nChannels*0.5) * channelWidthMHz) * 1000000.0;
		}
		double ChannelFrequencyHz(size_t coarseChannelNr, size_t channelIndexInSubband) const
		{
			const double channelWidthHz = 1000000.0*_header.bandwidthMHz / _header.nChannels;
			return (double(coarseChannelNr) - 0.5) * 1280000.0 + double(channelIndexInSubband)*channelWidthHz;
		}
		
		size_t CentreSubbandNumber() const;
		
		static double ArrayLattitudeRad();
		static double ArrayLongitudeRad();
		static double ArrayHeightMeters();
		static double VelocityFactor();
		
	private:
		std::vector<MWAInput> _inputs;
		std::vector<MWAAntenna> _antennae;
		std::map<size_t, MWAInput*> _antennaXInputs, _antennaYInputs;
		MWAHeader _header;
		MWAHeaderExt _headerExt;
		
		static int polCharToIndex(char polarizationChar);
};

#endif

