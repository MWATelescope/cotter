#include "mwaconfig.h"

/** @file mwaconfig.cpp
 * @brief Several functions to read the MWA meta data files.
 * 
 * Most functions were converted to C++ from corr2uvfits.c that is
 * written by Randall Wayth.
 * @author André Offringa offringa@gmail.com
 */

#define MWA_LATTITUDE -26.703319        // Array latitude. degrees North
#define MWA_LONGITUDE 116.67081         // Array longitude. degrees East
#define MWA_HEIGHT 377               // Array altitude. meters above sea level

#include <cstdlib>
#include <cmath>

#include <fstream>
#include <sstream>
#include <iostream>

#include <stdexcept>

#include "geometry.h"
#include "metafitsfile.h"

#define VEL_FACTOR  1.204         // the velocity factor of electic fields in RG-6 like coax.

MWAHeader::MWAHeader() :
	nInputs(0),
	nScans(0),
	nChannels(0),
	correlationType(None),
	integrationTime(0.0),
	centralFrequencyMHz(0.0),
	bandwidthMHz(0.0),
	raHrs(-99.0),
	decDegs(-99.0),
	haHrsStart(-99.0),
	refEl(M_PI*0.5),
	refAz(0.0),
	year(0),
	month(0),
	day(0),
	refHour(0),
	refMinute(0),
	refSecond(0.0),
	invertFrequency(false),    // correlators have now been fixed
	conjugate(false),
	geomCorrection(true),
	fieldName(),
	polProducts("XXXYYXYY")
{
}

MWAHeaderExt::MWAHeaderExt() :
	gpsTime(0), observerName("Unknown"), projectName("Unknown"),
	gridName("Unknown"), mode("Unknown"), filename("Unknown"),
	hasCalibrator(false), centreSBNumber(0),
	fibreFactor(VEL_FACTOR)
{
	for(size_t i=0; i!=16; ++i) delays[i] = 0;
	for(size_t i=0; i!=24; ++i) subbandGains[i] = 0;
}

double MWAHeader::GetStartDateMJD() const
{
	return Geometry::GetMJD(year, month, day, refHour, refMinute, refSecond);
}

void MWAConfig::ReadMetaFits(const char* filename, bool lockPointing)
{
	MetaFitsFile metaFile(filename);
	
	metaFile.ReadHeader(_header, _headerExt);
	_header.Validate(lockPointing);
	std::cout << "Observation covers " << (ChannelFrequencyHz(0)/1000000.0) << '-' << (ChannelFrequencyHz(_header.nChannels-1)/1000000.0) << " MHz.\n";
	
	if(_headerExt.centreSBNumber != (int) CentreSubbandNumber())
	{
		std::stringstream msg;
		msg << "The central subband number in the meta data fits file did not match with the given frequency. The frequency corresponds with subband number " << CentreSubbandNumber() << ", but fits meta file specifies " << _headerExt.centreSBNumber;
		throw std::runtime_error(msg.str());
	}
	
	metaFile.ReadTiles(_inputs, _antennae);
	for(std::vector<MWAInput>::const_iterator i=_inputs.begin();
			i!=_inputs.end(); ++i)
	{
		const MWAInput &input = *i;
		if(input.polarizationIndex == 0)
			_antennaXInputs.insert(std::pair<size_t, MWAInput>(input.antennaIndex, input));
		else
			_antennaYInputs.insert(std::pair<size_t, MWAInput>(input.antennaIndex, input));
	}
}

void MWAConfig::ReadHeader(const char *filename, bool lockPointing)
{
	std::ifstream file(filename);
	
	std::string line;
	
	while(file.good())
	{
		getline(file, line);
    if(!line.empty() && line[0]!='#')
		{
			std::istringstream lineStr(line);
			std::string keyStr, valueStr;
			lineStr >> keyStr >> valueStr;
			if(lineStr.fail()) throw std::runtime_error("Failed to parse header, line: " + line);
			
			if(keyStr == "FIELDNAME") _header.fieldName = valueStr;
			else if(keyStr == "N_SCANS") _header.nScans = atoi(valueStr.c_str());
			else if(keyStr == "N_INPUTS") _header.nInputs = atoi(valueStr.c_str());
			else if(keyStr == "N_CHANS") _header.nChannels = atoi(valueStr.c_str());
			else if(keyStr == "CORRTYPE") {
				switch(valueStr[0])
				{
					case 'c': case 'C': _header.correlationType = MWAHeader::CrossCorrelation; break;
					case 'a': case 'A': _header.correlationType = MWAHeader::AutoCorrelation; break;
					case 'b': case 'B': _header.correlationType = MWAHeader::BothCorrelations; break;
					default: throw std::runtime_error("Invalid correlation type specified in CORRTYPE: should be 'a', 'c' or 'b'.");
				}
			}
			else if(keyStr == "INT_TIME") _header.integrationTime = atof(valueStr.c_str());
			else if(keyStr == "FREQCENT") _header.centralFrequencyMHz = atof(valueStr.c_str());
			else if(keyStr == "BANDWIDTH") _header.bandwidthMHz = atof(valueStr.c_str());
			else if(keyStr == "INVERT_FREQ") _header.invertFrequency = atoi(valueStr.c_str());
			else if(keyStr == "CONJUGATE") _header.conjugate = atoi(valueStr.c_str());
			else if(keyStr == "GEOM_CORRECT") _header.geomCorrection = atoi(valueStr.c_str());
			else if(keyStr == "REF_AZ") _header.refAz = atof(valueStr.c_str())*(M_PI/180.0);
			else if(keyStr == "REF_EL") _header.refEl = atof(valueStr.c_str())*(M_PI/180.0);
			else if(keyStr == "HA_HRS") _header.haHrsStart = atof(valueStr.c_str());
			else if(keyStr == "RA_HRS") _header.raHrs = atof(valueStr.c_str());
			else if(keyStr == "DEC_DEGS") _header.decDegs = atof(valueStr.c_str());
			else if(keyStr == "DATE") {
				_header.day = atoi(valueStr.c_str()+6); valueStr[6]='\0';
				_header.month = atoi(valueStr.c_str()+4); valueStr[4]='\0';
				_header.year = atoi(valueStr.c_str());
			}
			else if(keyStr == "TIME") {
					_header.refSecond = atof(valueStr.c_str()+4); valueStr[4]='\0';
					_header.refMinute = atoi(valueStr.c_str()+2); valueStr[2]='\0';
					_header.refHour = atoi(valueStr.c_str());
			}
		}
	}
	
	_header.Validate(lockPointing);
	std::cout << "Observation covers " << (ChannelFrequencyHz(0)/1000000.0) << '-' << (ChannelFrequencyHz(_header.nChannels-1)/1000000.0) << " MHz.\n";
}

void MWAHeader::Validate(bool lockPointing)
{
	/* sanity checks and defaults */
	if(nScans == 0)
		throw std::runtime_error("N_SCANS not specified in header");
	if(fieldName.empty())
		fieldName = "TEST_32T";
	if(nInputs == 0)
		throw std::runtime_error("N_INPUTS not specified in header");
	if(nChannels == 0)
		throw std::runtime_error("N_CHANNELS not specified in header");
	if(correlationType == MWAHeader::None)
	{
		correlationType = MWAHeader::BothCorrelations;
		std::cerr << "WARNING: CORRTYPE unspecified. Assuming BOTH.\n";
	}
	if(integrationTime == 0)
		throw std::runtime_error("INT_TIME not specified in header");
	if(bandwidthMHz == 0)
		throw std::runtime_error("BANDWIDTH not specified in header");
	if(centralFrequencyMHz == 0.0)
		throw std::runtime_error("FREQCENT not specified in header");
	if(raHrs == -99)
		throw std::runtime_error("RA_HRS not specified in header");
	if(decDegs == -99)
		throw std::runtime_error("DEC_DEGS not specified in header");
	if(year==0 || month==0 || day==0)
		throw std::runtime_error("DATE not specified in header");
	if(haHrsStart == -99.0 && lockPointing)
		throw std::runtime_error("HA must be specified in header when locking pointing.");
	
	dateFirstScanMJD = 0.5*(integrationTime/86400.0) + Geometry::GetMJD(
		year, month, day, refHour, refMinute, refSecond);
}

/** Read the mapping between antennas and correlator inputs. */
void MWAConfig::ReadInputConfig(const char *filename)
{
	std::ifstream file(filename);
 
	std::string line;
	size_t nFlaggedInput = 0;
  while(file.good())
	{
		std::getline(file, line);
    if(!line.empty() && line[0]!='#')
		{
			MWAInput input;
			std::istringstream str(line);
			
			std::string cableLen;
			unsigned dummy, inputFlag;
			char polChar;
			
			str >> dummy >> input.antennaIndex >> polChar >> cableLen;
			if(str.fail())
				throw std::runtime_error("Failed scanning instrument configuration file in line: " + line);
			str >> inputFlag;
			if(str.fail())
				input.isFlagged = false;
			else
				input.isFlagged = inputFlag;
			
			if(input.isFlagged) ++nFlaggedInput;
			
			// decode the string with the cable length. for a prefix of "EL_" this means the value is an electrical length
			// not a physical one, so no velocity factor should be applied.
			if(cableLen.substr(0, 3) == "EL_")
				input.cableLenDelta = std::atof(&(cableLen.c_str()[3]));
			else
				input.cableLenDelta = std::atof(cableLen.c_str()) * VEL_FACTOR;

			input.polarizationIndex = polCharToIndex(polChar);
			input.inputIndex = _inputs.size();
			
			//std::cout << "input: " << input.inputIndex << " is antenna " << antennaIndex << " with pol " << input.polarizationIndex <<
			//	". Length delta: " << input.cableLenDelta << '\n';
				
			_inputs.push_back(input);
			if(input.polarizationIndex == 0)
				_antennaXInputs.insert(std::pair<size_t, MWAInput>(input.antennaIndex, input));
			else
				_antennaYInputs.insert(std::pair<size_t, MWAInput>(input.antennaIndex, input));
		}
  }
  std::cout << "Read " << _inputs.size() << " inputs from " << filename << ", of which " << nFlaggedInput << " were flagged.\n";
}

void MWAConfig::ReadAntennaPositions(const char *filename) {
	std::ifstream file(filename);
	const double arrayLattitudeRad = MWA_LATTITUDE*(M_PI/180.0);

  /* Scan through lines. Convert east, north, height units to XYZ units */
  while(file.good()) {
		std::string line;
		std::getline(file, line);
		if(!line.empty() && line[0]!='#')
		{
			MWAAntenna antenna;
			
			std::istringstream lineStr(line);
			double east, north, height;
			lineStr >> antenna.name >> east >> north >> height;
			if(lineStr.fail())
				throw std::runtime_error("Parsing antenna file failed on line: " + line);
			
			Geometry::ENH2XYZ_local(east, north, height, arrayLattitudeRad, antenna.position[0], antenna.position[1], antenna.position[2]);
			
			antenna.stationIndex = _antennae.size();
			
			if(antenna.name.size() > 4 && antenna.name.substr(0, 4) == "Tile")
				antenna.tileNumber = atoi(antenna.name.substr(4).c_str());
			else
				antenna.tileNumber = 0;
			
			_antennae.push_back(antenna);
		}
  }
  std::cout << "Read positions for " << _antennae.size() << " tiles.\n";
}

void MWAConfig::CheckSetup()
{
	if(_antennaXInputs.size() != _antennae.size() || _antennaYInputs.size() != _antennae.size())
		throw std::runtime_error("Number of tiles does not match number of X or Y inputs");
}

int MWAConfig::polCharToIndex(char pChar)
{
	switch(pChar)
	{
		case 'X': case 'x':
		case 'R': case 'r':
		case 'I': case 'i':
			return 0;
		case 'Y': case 'y':
		case 'L': case 'l':
			return 1;
		default:
			throw std::runtime_error(std::string("Unknown pol char: ") + pChar);
	}
}

double MWAConfig::ArrayLattitudeRad()
{
	return MWA_LATTITUDE*(M_PI/180.0);
}

double MWAConfig::ArrayLongitudeRad()
{
	return MWA_LONGITUDE*(M_PI/180.0);
}

double MWAConfig::ArrayHeightMeters()
{
	return MWA_HEIGHT;
}

size_t MWAConfig::CentreSubbandNumber() const
{
	return round(12.0 + ChannelFrequencyHz(0) / 1280000.0);
}

double MWAConfig::VelocityFactor()
{
	return VEL_FACTOR;
}
