#include "fitswriter.h"
#include "fitsreader.h"

#include <stdexcept>
#include <sstream>
#include <vector>

#include <fitsio.h>
#include <cmath>
#include <cstdio>
#include <limits>
#include <iostream>

void FitsWriter::checkStatus(int status, const std::string& filename) const
{
	if(status) {
		/* fits_get_errstatus returns at most 30 characters */
		char err_text[31];
		fits_get_errstatus(status, err_text);
		char err_msg[81];
		std::stringstream errMsg;
		errMsg << "CFITSIO reported error when performing IO on file '" << filename << "':" << err_text << " (";
		while(fits_read_errmsg(err_msg))
			errMsg << err_msg;
		errMsg << ')';
		throw std::runtime_error(errMsg.str());
	}
}

template<typename NumType>
void FitsWriter::Write(const std::string& filename, const NumType* image) const
{
	int status = 0;
	fitsfile *fptr;
	fits_create_file(&fptr, (std::string("!") + filename).c_str(), &status);
	checkStatus(status, filename);
	
	// append image HDU
	int bitPixInt = FLOAT_IMG;
	long naxes[4];
	naxes[0] = _width;
	naxes[1] = _height;
	naxes[2] = 1;
	naxes[3] = 1;
	fits_create_img(fptr, bitPixInt, 4, naxes, &status);
	checkStatus(status, filename);
	float zero = 0, one = 1, equinox = 2000.0;
	fits_write_key(fptr, TFLOAT, "BSCALE", (void*) &one, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "BZERO", (void*) &zero, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TSTRING, "BUNIT", (void*) "JY/BEAM", "Units are in Jansky per beam", &status); checkStatus(status, filename);
	
	if(_hasBeam)
	{
		double
			majDeg = setNotFiniteToZero(_beamMajorAxisRad * 180.0 / M_PI),
			minDeg = setNotFiniteToZero(_beamMinorAxisRad * 180.0 / M_PI), 
			posAngle = setNotFiniteToZero(_beamPositionAngle * 180.0 / M_PI);
		fits_write_key(fptr, TDOUBLE, "BMAJ", (void*) &majDeg, "", &status); checkStatus(status, filename);
		fits_write_key(fptr, TDOUBLE, "BMIN", (void*) &minDeg, "", &status); checkStatus(status, filename);
		fits_write_key(fptr, TDOUBLE, "BPA", (void*) &posAngle, "", &status); checkStatus(status, filename);
	}
	
	fits_write_key(fptr, TFLOAT, "EQUINOX", (void*) &equinox, "J2000", &status); checkStatus(status, filename);
	fits_write_key(fptr, TSTRING, "BTYPE", (void*) "Intensity", "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TSTRING, "ORIGIN", (void*) _origin.c_str(), _originComment.c_str(), &status); checkStatus(status, filename);
	float phaseCentreRADeg = (_phaseCentreRA/M_PI)*180.0, phaseCentreDecDeg = (_phaseCentreDec/M_PI)*180.0;
	float
		centrePixelX = _pixelSizeX!=0.0 ? ((_width / 2.0)+1.0 + _phaseCentreDL/_pixelSizeX) : (_width / 2.0)+1.0,
		centrePixelY = _pixelSizeY!=0.0 ? ((_height / 2.0)+1.0 - _phaseCentreDM/_pixelSizeY) : (_height / 2.0)+1.0,
		stepXDeg = (-_pixelSizeX / M_PI)*180.0,
		stepYDeg = ( _pixelSizeY / M_PI)*180.0;
	fits_write_key(fptr, TSTRING, "CTYPE1", (void*) "RA---SIN", "Right ascension angle cosine", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CRPIX1", (void*) &centrePixelX, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CRVAL1", (void*) &phaseCentreRADeg, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CDELT1", (void*) &stepXDeg, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TSTRING, "CUNIT1", (void*) "deg", "", &status); checkStatus(status, filename);
	
	fits_write_key(fptr, TSTRING, "CTYPE2", (void*) "DEC--SIN", "Declination angle cosine", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CRPIX2", (void*) &centrePixelY, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CRVAL2", (void*) &phaseCentreDecDeg, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CDELT2", (void*) &stepYDeg, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TSTRING, "CUNIT2", (void*) "deg", "", &status); checkStatus(status, filename);

	fits_write_key(fptr, TSTRING, "CTYPE3", (void*) "FREQ", "Central frequency", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CRPIX3", (void*) &one, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TDOUBLE, "CRVAL3", (void*) &_frequency, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TDOUBLE, "CDELT3", (void*) &_bandwidth, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TSTRING, "CUNIT3", (void*) "Hz", "", &status); checkStatus(status, filename);
	
	float pol;
	switch(_polarization)
	{
		case Polarization::StokesI: pol = 1.0; break;
		case Polarization::StokesQ: pol = 2.0; break;
		case Polarization::StokesU: pol = 3.0; break;
		case Polarization::StokesV: pol = 4.0; break;
		case Polarization::RR: pol = -1.0; break;
		case Polarization::LL: pol = -2.0; break;
		case Polarization::RL: pol = -3.0; break;
		case Polarization::LR: pol = -4.0; break;
		case Polarization::XX: pol = -5.0; break;
		case Polarization::YY: pol = -6.0; break; //yup, this is really the right value
		case Polarization::XY: pol = -7.0; break;
		case Polarization::YX: pol = -8.0; break;
	}
	fits_write_key(fptr, TSTRING, "CTYPE4", (void*) "STOKES", "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CRPIX4", (void*) &one, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CRVAL4", (void*) &pol, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CDELT4", (void*) &one, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TSTRING, "CUNIT4", (void*) "Hz", "", &status); checkStatus(status, filename);
	
	// RESTFRQ ?
	fits_write_key(fptr, TSTRING, "SPECSYS", (void*) "TOPOCENT", "", &status); checkStatus(status, filename);
	
  int year, month, day, hour, min, sec, deciSec;
	julianDateToYMD(_dateObs + 2400000.5, year, month, day);
	mjdToHMS(_dateObs, hour, min, sec, deciSec);
	char dateStr[40];
  std::sprintf(dateStr, "%d-%02d-%02dT%02d:%02d:%02d.%01d", year, month, day, hour, min, sec, deciSec);
	fits_write_key(fptr, TSTRING, "DATE-OBS", (void*) dateStr, "", &status); checkStatus(status, filename);
	
	// Extra keywords
	for(std::map<std::string, std::string>::const_iterator i=_extraStringKeywords.begin(); i!=_extraStringKeywords.end(); ++i)
	{
		const char* name = i->first.c_str();
		char* valueStr = const_cast<char*>(i->second.c_str());
		fits_write_key(fptr, TSTRING, name, valueStr, "", &status);
		checkStatus(status, filename);
	}
	for(std::map<std::string, double>::const_iterator i=_extraNumKeywords.begin(); i!=_extraNumKeywords.end(); ++i)
	{
		const char* name = i->first.c_str();
		fits_write_key(fptr, TDOUBLE, name, (void*) &i->second, "", &status);
		checkStatus(status, filename);
	}
	
	// History
	std::ostringstream histStr;
	for(std::vector<std::string>::const_iterator i=_history.begin(); i!=_history.end(); ++i)
	{
		fits_write_history(fptr, i->c_str(), &status);
		checkStatus(status, filename);
	}
	
	long firstpixel[4];
	for(int i=0;i < 4;i++) firstpixel[i] = 1;
	if(sizeof(NumType)==8)
	{
		double nullValue = std::numeric_limits<double>::max();
		fits_write_pixnull(fptr, TDOUBLE, firstpixel, _width*_height, const_cast<double*>(reinterpret_cast<const double*>(image)), &nullValue, &status);
	}
	else if(sizeof(NumType)==4)
	{
		float nullValue = std::numeric_limits<float>::max();
		fits_write_pixnull(fptr, TFLOAT, firstpixel, _width*_height, const_cast<float*>(reinterpret_cast<const float*>(image)), &nullValue, &status);
	}
	else
	{
		double nullValue = std::numeric_limits<double>::max();
		size_t totalSize = _width*_height;
		std::vector<double> copy(totalSize);
		for(size_t i=0;i!=totalSize;++i) copy[i] = image[i];
		fits_write_pixnull(fptr, TDOUBLE, firstpixel, totalSize, &copy[0], &nullValue, &status);
	}
	checkStatus(status, filename);
	
	fits_close_file(fptr, &status);
	checkStatus(status, filename);
}

template void FitsWriter::Write<long double>(const std::string& filename, const long double* image) const;
template void FitsWriter::Write<double>(const std::string& filename, const double* image) const;
template void FitsWriter::Write<float>(const std::string& filename, const float* image) const;

void FitsWriter::SetMetadata(const FitsReader& reader)
{
	_width = reader.ImageWidth();
	_height = reader.ImageHeight();
	_phaseCentreRA = reader.PhaseCentreRA();
	_phaseCentreDec = reader.PhaseCentreDec();
	_pixelSizeX = reader.PixelSizeX();
	_pixelSizeY = reader.PixelSizeY();
	_frequency = reader.Frequency();
	_bandwidth = reader.Bandwidth();
	_dateObs = reader.DateObs();
	_polarization = reader.Polarization();
	_hasBeam = reader.HasBeam();
	if(_hasBeam)
	{
		_beamMajorAxisRad = reader.BeamMajorAxisRad();
		_beamMinorAxisRad = reader.BeamMinorAxisRad();
		_beamPositionAngle = reader.BeamPositionAngle();
	}
	_phaseCentreDL = reader.PhaseCentreDL();
	_phaseCentreDM = reader.PhaseCentreDM();
	_origin = reader.Origin();
	_originComment = reader.OriginComment();
	_history = reader.History();
}

void FitsWriter::julianDateToYMD(double jd, int &year, int &month, int &day) const
{
  int z = jd+0.5;
  int w = (z-1867216.25)/36524.25;
  int x = w/4;
  int a = z+1+w-x;
  int b = a+1524;
  int c = (b-122.1)/365.25;
  int d = 365.25*c;
  int e = (b-d)/30.6001;
  int f = 30.6001*e;
  day = b-d-f;
  while (e-1 > 12) e-=12;
  month = e-1;
  year = c-4715-((e-1)>2?1:0);
}

void FitsWriter::mjdToHMS(double mjd, int& hour, int& minutes, int& seconds, int& deciSec) const
{
	hour = int(fmod(mjd * 24.0, 24.0));
	minutes = int(fmod(mjd*60.0 * 24.0, 60.0));
	seconds = int(fmod(mjd*3600.0 * 24.0, 60.0));
	deciSec = int(fmod(mjd*36000.0 * 24.0, 10.0));
}
