#include "metafitsfile.h"
#include "mwaconfig.h"
#include "geometry.h"

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <cmath>

MetaFitsFile::MetaFitsFile(const char* filename)
{
	int status = 0;
	if(fits_open_file(&_fptr, filename, READONLY, &status))
		throwError(status, std::string("Cannot open file ") + filename);
	
	int hduCount;
	fits_get_num_hdus(_fptr, &hduCount, &status);
	checkStatus(status);
	
	if(hduCount < 2)
		throw std::runtime_error("At least two HDUs are expected to be in the meta fitsfile");
}

MetaFitsFile::~MetaFitsFile()
{
	int status = 0;
	if(fits_close_file(_fptr, &status))
		throwError(status, "Could not close metafits file");
}

void MetaFitsFile::ReadHeader(MWAHeader& header, MWAHeaderExt &headerExt)
{
	int status = 0, hduType;
	fits_movabs_hdu(_fptr, 1, &hduType, &status);
	checkStatus(status);
	
	int keywordCount;
	fits_get_hdrspace(_fptr, &keywordCount, 0, &status);
	checkStatus(status);
	
	headerExt.tilePointingDecRad = -99.0;
	headerExt.tilePointingRARad = -99.0;
	header.raHrs = -99.0;
	header.decDegs = -99.0;
	
	for(int i=0; i!=keywordCount; ++i)
	{
		char keyName[80], keyValue[80];
		fits_read_keyn(_fptr, i+1, keyName, keyValue, 0, &status);
		parseKeyword(header, headerExt, keyName, keyValue);
	}
	header.correlationType = MWAHeader::BothCorrelations;
	
	if(headerExt.tilePointingDecRad == -99.0 || headerExt.tilePointingRARad == -99.0)
		throw std::runtime_error("The metafits file does not specify a pointing direction (keywords RA and DEC)");
	if(header.raHrs == -99.0 || header.decDegs == -99.0)
	{
		std::cout << "The metafits file does not specify a phase centre; will use pointing centre as phase\n"
		"centre, unless overridden on command line.\n";
		header.raHrs = headerExt.tilePointingRARad * (12.0/M_PI);
		header.decDegs = headerExt.tilePointingDecRad * (180.0/M_PI);
	}
}

void MetaFitsFile::ReadTiles(std::vector<MWAInput>& inputs, std::vector<MWAAntenna>& antennae)
{
	int status = 0;
	
	int hduType;
	fits_movabs_hdu(_fptr, 2, &hduType, &status);
	checkStatus(status);
	
	char
		inputColName[] = "Input",
		antennaColName[] = "Antenna",
		tileColName[] = "Tile",
		polColName[] = "Pol",
		rxColName[] = "Rx",
		slotColName[] = "Slot",
		flagColName[] = "Flag",
		lengthColName[] = "Length",
		eastColName[] = "East",
		northColName[] = "North",
		heightColName[] = "Height",
		gainsColName[] = "Gains";
	int inputCol, antennaCol, tileCol, polCol, rxCol, slotCol, flagCol, lengthCol, northCol, eastCol, heightCol, gainsCol;
		
	fits_get_colnum(_fptr, CASESEN, inputColName, &inputCol, &status);
	fits_get_colnum(_fptr, CASESEN, antennaColName, &antennaCol, &status);
	fits_get_colnum(_fptr, CASESEN, tileColName, &tileCol, &status);
	fits_get_colnum(_fptr, CASESEN, polColName, &polCol, &status);
	fits_get_colnum(_fptr, CASESEN, rxColName, &rxCol, &status);
	fits_get_colnum(_fptr, CASESEN, slotColName, &slotCol, &status);
	fits_get_colnum(_fptr, CASESEN, flagColName, &flagCol, &status);
	fits_get_colnum(_fptr, CASESEN, lengthColName, &lengthCol, &status);
	fits_get_colnum(_fptr, CASESEN, eastColName, &eastCol, &status);
	fits_get_colnum(_fptr, CASESEN, northColName, &northCol, &status);
	fits_get_colnum(_fptr, CASESEN, heightColName, &heightCol, &status);
	checkStatus(status);
	// The gains col is not present in older metafits files
	fits_get_colnum(_fptr, CASESEN, gainsColName, &gainsCol, &status);
	if(status != 0)
	{
		gainsCol = -1;
		status = 0;
	}
	else {
		std::cout << "Meta fits file contains per-input pfb digital subband gains.\n";
	}
	
	long int nrow;
	fits_get_num_rows(_fptr, &nrow, &status);
	checkStatus(status);
	antennae.resize(nrow/2);
	inputs.resize(nrow);
	
	for(long int i=0; i!=nrow; ++i)
	{
		int input, antenna, tile, rx, slot, flag, gainValues[24];
		double north, east, height;
		char pol;
		char length[81] = "";
		char *lengthPtr[1] = { length };
		fits_read_col(_fptr, TINT, inputCol, i+1, 1, 1, 0, &input, 0, &status);
		fits_read_col(_fptr, TINT, antennaCol, i+1, 1, 1, 0, &antenna, 0, &status);
		fits_read_col(_fptr, TINT, tileCol, i+1, 1, 1, 0, &tile, 0, &status);
		fits_read_col(_fptr, TBYTE, polCol, i+1, 1, 1, 0, &pol, 0, &status);
		fits_read_col(_fptr, TINT, rxCol, i+1, 1, 1, 0, &rx, 0, &status);
		fits_read_col(_fptr, TINT, slotCol, i+1, 1, 1, 0, &slot, 0, &status);
		fits_read_col(_fptr, TINT, flagCol, i+1, 1, 1, 0, &flag, 0, &status);
		fits_read_col(_fptr, TSTRING, lengthCol, i+1, 1, 1, 0, lengthPtr, 0, &status);
		fits_read_col(_fptr, TDOUBLE, eastCol, i+1, 1, 1, 0, &east, 0, &status);
		fits_read_col(_fptr, TDOUBLE, northCol, i+1, 1, 1, 0, &north, 0, &status);
		fits_read_col(_fptr, TDOUBLE, heightCol, i+1, 1, 1, 0, &height, 0, &status);
		if(gainsCol != -1)
			fits_read_col(_fptr, TINT, gainsCol, i+1, 1, 24, 0, gainValues, 0, &status);
		checkStatus(status);
		
		if(pol == 'X')
		{
			std::stringstream nameStr;
			nameStr << "Tile";
			if(tile<100) nameStr << '0';
			if(tile<10) nameStr << '0';
			nameStr << tile;
			
			MWAAntenna &ant = antennae[antenna];
			ant.name = nameStr.str();
			ant.tileNumber = tile;
			Geometry::ENH2XYZ_local(east, north, height, MWAConfig::ArrayLattitudeRad(), ant.position[0], ant.position[1], ant.position[2]);
			ant.stationIndex = antenna;
			//std::cout << ant.name << ' ' << input << ' ' << antenna << ' ' << east << ' ' << north << ' ' << height << '\n';
		} else if(pol != 'Y')
			throw std::runtime_error("Error parsing polarization");
		
		length[80] = 0;
		std::string lengthStr = length;
		inputs[input].inputIndex = input;
		inputs[input].antennaIndex = antenna;
		if(lengthStr.substr(0, 3) == "EL_")
			inputs[input].cableLenDelta = atof(&(lengthStr.c_str()[3]));
		else
			inputs[input].cableLenDelta = atof(lengthStr.c_str()) * MWAConfig::VelocityFactor();
		inputs[input].polarizationIndex = (pol=='X') ? 0 : 1;
		inputs[input].isFlagged = (flag!=0);
		if(gainsCol != -1) {
			for(size_t sb=0; sb!=24; ++sb) {
				// The digital pfb gains are multiplied with 64 to allow more careful
				// fine tuning of the gains.
				inputs[input].pfbGains[sb] = (double) gainValues[sb] / 64.0;
			}
		}
	}
}

void MetaFitsFile::parseKeyword(MWAHeader &header, MWAHeaderExt &headerExt, const char *keyName, const char *keyValue)
{
	std::string name(keyName);
	
	if(name == "SIMPLE" || name == "BITPIX" || name == "NAXIS" || name == "EXTEND" || name == "CONTINUE")
		; //standard FITS headers; ignore.
	else if(name == "GPSTIME")
		headerExt.gpsTime = atoi(keyValue);
	else if(name == "FILENAME")
	{
		headerExt.filename = parseFitsString(keyValue);
		header.fieldName = stripBand(headerExt.filename);
	}
	else if(name == "DATE-OBS")
	{
		parseFitsDate(keyValue, header.year, header.month, header.day, header.refHour, header.refMinute, header.refSecond);
		headerExt.dateRequestedMJD = parseFitsDateToMJD(keyValue);
	}
	else if(name == "RAPHASE")
		header.raHrs = atof(keyValue) * (24.0 / 360.0);
	else if(name == "DECPHASE")
		header.decDegs = atof(keyValue);
	else if(name == "RA")
		headerExt.tilePointingRARad = atof(keyValue) * (M_PI / 180.0);
	else if(name == "DEC")
		headerExt.tilePointingDecRad = atof(keyValue) * (M_PI / 180.0);
	else if(name == "GRIDNAME")
		headerExt.gridName = parseFitsString(keyValue);
	else if(name == "CREATOR")
		headerExt.observerName = parseFitsString(keyValue);
	else if(name == "PROJECT")
		headerExt.projectName = parseFitsString(keyValue);
	else if(name == "MODE")
		headerExt.mode = parseFitsString(keyValue);
	else if(name == "DELAYS")
		parseIntArray(keyValue, headerExt.delays, 16);
	else if(name == "CALIBRAT")
		headerExt.hasCalibrator = parseBool(keyValue);
	else if(name == "CENTCHAN")
		headerExt.centreSBNumber = atoi(keyValue);
	else if(name == "CHANGAIN") {
		parseIntArray(keyValue, headerExt.subbandGains, 24);
		headerExt.hasGlobalSubbandGains = true;
	}
	else if(name == "FIBRFACT")
		headerExt.fiberFactor = atof(keyValue);
	else if(name == "INTTIME")
		header.integrationTime = atof(keyValue);
	else if(name == "NSCANS")
		header.nScans = atoi(keyValue);
	else if(name == "NINPUTS")
		header.nInputs = atoi(keyValue);
	else if(name == "NCHANS")
		header.nChannels = atoi(keyValue);
	else if(name == "BANDWDTH")
		header.bandwidthMHz = atof(keyValue);
	else if(name == "FREQCENT")
		header.centralFrequencyMHz = atof(keyValue);
	else if(name == "DATESTRT")
		; //parseFitsDate(keyValue, header.year, header.month, header.day, header.refHour, header.refMinute, header.refSecond);
	else if(name == "DATE")
		; // Date that metafits was created; ignored.
	else if(name == "EXPOSURE" || name == "MJD" || name == "LST" || name == "HA" || name == "AZIMUTH" || name == "ALTITUDE" || name == "SUN-DIST" || name == "MOONDIST" || name == "JUP-DIST" || name == "GRIDNUM" || name == "RECVRS" || name == "CHANNELS" || name == "SUN-ALT" || name == "TILEFLAG" || name == "NAV_FREQ" || name == "FINECHAN" || name == "TIMEOFF")
		; // Ignore these fields, they can be derived from others.
	else
		std::cout << "Ignored keyword: " << name << '\n';
}

std::string MetaFitsFile::parseFitsString(const char* valueStr)
{
	if(valueStr[0] != '\'')
		throw std::runtime_error(std::string("Error parsing fits string: ") + valueStr);
	std::string value(valueStr+1);
	if((*value.rbegin())!='\'')
		throw std::runtime_error(std::string("Error parsing fits string: ") + valueStr);
	int s = value.size() - 1;
	while(s > 0 && value[s-1]==' ') --s;
	return value.substr(0, s);
}

void MetaFitsFile::parseFitsDate(const char* valueStr, int& year, int& month, int& day, int& hour, int& min, double& sec)
{
	std::string dateStr = parseFitsString(valueStr);
	if(dateStr.size() != 19)
		throw std::runtime_error("Error parsing fits date");
	year = (dateStr[0]-'0')*1000 + (dateStr[1]-'0')*100 +
		(dateStr[2]-'0')*10 + (dateStr[3]-'0');
	month = (dateStr[5]-'0')*10 + (dateStr[6]-'0');
	day = (dateStr[8]-'0')*10 + (dateStr[9]-'0');
	
	hour = (dateStr[11]-'0')*10 + (dateStr[12]-'0');
	min = (dateStr[14]-'0')*10 + (dateStr[15]-'0');
	sec = (dateStr[17]-'0')*10 + (dateStr[18]-'0');
}

double MetaFitsFile::parseFitsDateToMJD(const char* valueStr)
{
	int year, month, day, hour, min;
	double sec;
	parseFitsDate(valueStr, year, month, day, hour, min, sec);
	return Geometry::GetMJD(year, month, day, hour, min, sec);
}

void MetaFitsFile::parseIntArray(const char* valueStr, int* values, size_t count)
{
	std::string str = parseFitsString(valueStr);
	size_t pos = 0;
	for(size_t i=0; i!=count-1; ++i)
	{
		size_t next = str.find(',', pos);
		if(next == str.npos)
			throw std::runtime_error("Error parsing integer list in metafits file");
		*values = atoi(str.substr(pos, next-pos).c_str());
		++values;
		pos = next+1;
	}
	*values = atoi(str.substr(pos).c_str());
}

bool MetaFitsFile::parseBool(const char* valueStr)
{
	if(valueStr[0] != 'T' && valueStr[0] != 'F')
		throw std::runtime_error("Error parsing boolean in fits file");
	return valueStr[0]=='T';
}

std::string MetaFitsFile::stripBand(const std::string& input)
{
	if(!input.empty())
	{
		size_t pos = input.size()-1;
		while(pos>0 && isDigit(input[pos]))
		{
			--pos;
		}
		if(pos > 0 && input[pos] == '_')
		{
			int band = atoi(&input[pos+1]);
			if(band > 0 && band <= 256)
			{
				return input.substr(0, pos);
			}
		}
	}
	return input;
}
