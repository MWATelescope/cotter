#include "fitsreader.h"
#include "polarizationenum.h"

#include <stdexcept>
#include <sstream>
#include <cmath>

#include <fits/FITS/FITSDateUtil.h>
#include <casa/Quanta/MVTime.h>
#include <measures/Measures/MeasConvert.h>

void FitsReader::checkStatus(int status, const std::string &operation) 
{
	if(status) {
		/* fits_get_errstatus returns at most 30 characters */
		char err_text[31];
		fits_get_errstatus(status, err_text);
		char err_msg[81];
		std::stringstream errMsg;
		if(!operation.empty())
			errMsg << "During operation " << operation << ", ";
		errMsg << "CFITSIO reported error when performing IO on file '" << _filename << "': " << err_text << " (";
		while(fits_read_errmsg(err_msg))
			errMsg << err_msg;
		errMsg << ')';
		throw std::runtime_error(errMsg.str());
	}
}

float FitsReader::readFloatKey(const char *key)
{
	int status = 0;
	float floatValue;
	fits_read_key(_fitsPtr, TFLOAT, key, &floatValue, 0, &status);
	checkStatus(status, key);
	return floatValue;
}

double FitsReader::readDoubleKey(const char *key)
{
	int status = 0;
	double value;
	fits_read_key(_fitsPtr, TDOUBLE, key, &value, 0, &status);
	checkStatus(status, key);
	return value;
}

bool FitsReader::readFloatKeyIfExists(const char *key, float &dest)
{
	int status = 0;
	float floatValue;
	fits_read_key(_fitsPtr, TFLOAT, key, &floatValue, 0, &status);
	if(status == 0)
		dest = floatValue;
	return status == 0;
}

bool FitsReader::readDoubleKeyIfExists(const char *key, double &dest)
{
	int status = 0;
	double doubleValue;
	fits_read_key(_fitsPtr, TDOUBLE, key, &doubleValue, 0, &status);
	if(status == 0)
		dest = doubleValue;
	return status == 0;
}

bool FitsReader::readDateKeyIfExists(const char *key, double &dest)
{
	int status = 0;
	char keyStr[256];
	fits_read_key(_fitsPtr, TSTRING, key, keyStr, 0, &status);
	if(status == 0)
	{
		dest = parseFitsDateToMJD(keyStr);
		return true;
	}
	else return false;
}

std::string FitsReader::readStringKey(const char *key)
{
	int status = 0;
	char keyStr[256];
	fits_read_key(_fitsPtr, TSTRING, key, keyStr, 0, &status);
	checkStatus(status, key);
	return std::string(keyStr);
}

bool FitsReader::readStringKeyIfExists(const char *key, std::string& value, std::string& comment)
{
	int status = 0;
	char valueStr[256], commentStr[256];
	fits_read_key(_fitsPtr, TSTRING, key, valueStr, commentStr, &status);
	checkStatus(status, key);
	if(status == 0)
	{
		value = valueStr;
		comment = commentStr;
	}
	return status == 0;
}

void FitsReader::initialize()
{
	int status = 0;
	fits_open_file(&_fitsPtr, _filename.c_str(), READONLY, &status);
	checkStatus(status);
	
	// Move to first HDU
	int hduType;
	fits_movabs_hdu(_fitsPtr, 1, &hduType, &status);
	checkStatus(status);
	if(hduType != IMAGE_HDU) throw std::runtime_error("First HDU is not an image");
	
	int naxis = 0;
	fits_get_img_dim(_fitsPtr, &naxis, &status);
	checkStatus(status);
	if(naxis < 2) throw std::runtime_error("NAxis in image < 2");
	
	std::vector<long> naxes(naxis);
	fits_get_img_size(_fitsPtr, naxis, &naxes[0], &status);
	checkStatus(status);
	for(int i=2;i!=naxis;++i)
		if(naxes[i] != 1) throw std::runtime_error("Multiple images in fits file");
	_imgWidth = naxes[0];
	_imgHeight = naxes[1];
	
	double bScale = 1.0, bZero = 0.0, equinox = 2000.0;
	readDoubleKeyIfExists("BSCALE", bScale);
	readDoubleKeyIfExists("BZERO", bZero);
	readDoubleKeyIfExists("EQUINOX", equinox);
	if(bScale != 1.0)
		throw std::runtime_error("Invalid value for BSCALE");
	if(bZero != 0.0)
		throw std::runtime_error("Invalid value for BZERO");
	if(equinox != 2000.0)
		throw std::runtime_error("Invalid value for EQUINOX: "+readStringKey("EQUINOX"));
	
	if(readStringKey("CTYPE1") != "RA---SIN")
		throw std::runtime_error("Invalid value for CTYPE1");
	_phaseCentreRA = readDoubleKey("CRVAL1") * (M_PI / 180.0);
	_pixelSizeX = readDoubleKey("CDELT1") * (-M_PI / 180.0);
	if(readStringKey("CUNIT1") != "deg")
		throw std::runtime_error("Invalid value for CUNIT1");
	double centrePixelX = readDoubleKey("CRPIX1");
	_phaseCentreDL = (centrePixelX - ((_imgWidth / 2.0)+1.0)) * _pixelSizeX;

	if(readStringKey("CTYPE2") != "DEC--SIN")
		throw std::runtime_error("Invalid value for CTYPE2");
	_phaseCentreDec = readDoubleKey("CRVAL2") * (M_PI / 180.0);
	_pixelSizeY = readDoubleKey("CDELT2") * (M_PI / 180.0);
	if(readStringKey("CUNIT2") != "deg")
		throw std::runtime_error("Invalid value for CUNIT2");
	double centrePixelY = readDoubleKey("CRPIX2");
	_phaseCentreDM = ((_imgHeight / 2.0)+1.0 - centrePixelY) * _pixelSizeY;
	
	readDateKeyIfExists("DATE-OBS", _dateObs);
	if(naxis >= 3)
	{
		if(readStringKey("CTYPE3") == "FREQ")
		{
			_frequency = readDoubleKey("CRVAL3");
			_bandwidth = readDoubleKey("CDELT3");
		}
	} else {
		_frequency = 0.0;
		_bandwidth = 0.0;
	}
	if(naxis >= 4)
	{
		if(readStringKey("CTYPE4") == "STOKES")
		{
			double val = readDoubleKey("CRVAL4");
			switch(int(val))
			{
				default: throw std::runtime_error("Unknown polarization specified in fits file");
				case 1: _polarization = Polarization::StokesI; break;
				case 2: _polarization = Polarization::StokesQ; break;
				case 3: _polarization = Polarization::StokesU; break;
				case 4: _polarization = Polarization::StokesV; break;
				case -1: _polarization = Polarization::RR; break;
				case -2: _polarization = Polarization::LL; break;
				case -3: _polarization = Polarization::RL; break;
				case -4: _polarization = Polarization::LR; break;
				case -5: _polarization = Polarization::XX; break;
				case -6: _polarization = Polarization::YY; break;
				case -7: _polarization = Polarization::XY; break;
				case -8: _polarization = Polarization::YX; break;
			}
		}
	}
	double bMaj=0.0, bMin=0.0, bPa=0.0;
	if(readDoubleKeyIfExists("BMAJ", bMaj) && readDoubleKeyIfExists("BMIN", bMin) && readDoubleKeyIfExists("BPA", bPa))
	{
		_hasBeam = true;
		_beamMajorAxisRad = bMaj * (M_PI / 180.0);
		_beamMinorAxisRad = bMin * (M_PI / 180.0);
		_beamPositionAngle = bPa * (M_PI / 180.0);
	}
	else {
		_hasBeam = false;
		_beamMajorAxisRad = 0.0;
		_beamMinorAxisRad = 0.0;
		_beamPositionAngle = 0.0;
	}
	readStringKeyIfExists("ORIGIN", _origin, _originComment);
	readHistory();
}

template void FitsReader::Read(double* image);

template<typename NumType>
void FitsReader::Read(NumType* image)
{
	int status = 0;
	int naxis = 0;
	fits_get_img_dim(_fitsPtr, &naxis, &status);
	checkStatus(status);
	std::vector<long> firstPixel(naxis);
	for(int i=0;i!=naxis;++i) firstPixel[i] = 1;
	
	if(sizeof(NumType)==8)
		fits_read_pix(_fitsPtr, TDOUBLE, &firstPixel[0], _imgWidth*_imgHeight, 0, image, 0, &status);
	else
		throw std::runtime_error("sizeof(NumType)!=8 not implemented");
	checkStatus(status);
}

void FitsReader::readHistory()
{
	int status = 0;
	int npos, moreKeys;
	fits_get_hdrspace(_fitsPtr, &npos, &moreKeys, &status);
	checkStatus(status);
	char keyCard[256];
	for(int pos=1; pos<=npos; ++pos)
	{
		fits_read_record(_fitsPtr, pos, keyCard, &status);
		keyCard[7] = 0;
		if(std::string("HISTORY") == keyCard) {
			_history.push_back(&keyCard[8]);
		}
	}
}

FitsReader::~FitsReader()
{
	int status = 0;
	fits_close_file(_fitsPtr, &status);
	checkStatus(status);
}

double FitsReader::parseFitsDateToMJD(const char* valueStr)
{
	casa::MVTime time;
	casa::MEpoch::Types systypes;
	bool parseSuccess = casa::FITSDateUtil::fromFITS(time, systypes, valueStr, "UTC");
	if(!parseSuccess)
		throw std::runtime_error("Could not parse FITS date");
	casa::MEpoch epoch(time.get(), systypes);
	return epoch.getValue().get();
}
