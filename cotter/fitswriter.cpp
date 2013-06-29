#include "fitswriter.h"

#include <cstdio>

#include "slalib.h"

#define VLIGHT 299792458.0  // speed of light in m/s

FitsWriter::FitsWriter(const char* filename) : _nRowsWritten(0), _groupHeadersInitialized(false)
{
	/** If the file already exists, remove it */
	FILE *fp = std::fopen(filename, "r");
  if (fp != NULL) {
    std::fclose(fp);
    std::remove(filename);
  }
  
	int status = 0;
  if(fits_create_file(&_fptr, filename, &status))
		throwError(status, std::string("Cannot open file ") + filename);
}

FitsWriter::~FitsWriter()
{
	setKeywordToInt("GCOUNT", _nRowsWritten);
	
	writeAntennaTable();
	
	int status = 0;
	if(fits_close_file(_fptr, &status))
		throwError(status, std::string("Cannot close file "));
}

void FitsWriter::initGroupHeader()
{
	int status = 0;
  /* Write the group header and initialize random groups.
	 * The data are grouped in visibilities that belong to the same time step and
	 * baseline. Within a group, the visibilities are sorted as 
	 * (real, imag, weight) x nrpolarizations (XX,XY,..) x frequency channel
	 * The preamble consists of:
	 * - U, V, W in nanoseconds
	 * - baseline
	 * - time offset in days
	 * This call will set the keywords
	 * - PCOUNT=5
	 * - GROUPS=T
	 * - GCOUNT=nr baselines x time step count
	 */
	const unsigned NAXIS = 6;
	long naxes[NAXIS];
  naxes[0] = 0;
  naxes[1] = 3;  // real, imaginary, weight
  naxes[2] = 4;  // polarization count
  naxes[3] = _bandInfo.channels.size();
  naxes[4] = 1;
  naxes[5] = 1;
	const unsigned nGroupParams = 5; // u,v,w,baseline,time
	// Total number of rows: don't know yet, start with one timestep.
	const unsigned nRows = _antennae.size() * (_antennae.size() + 1) / 2;
  fits_write_grphdr(_fptr, TRUE, FLOAT_IMG, NAXIS, naxes, nGroupParams,
										nRows, TRUE, &status);
	checkStatus(status);
	
  setKeywordToFloat("BSCALE", 1.0);
	
  // Set the UU,VV,WW,BASELINE and DATE header names and scales
  const char *parameterNames[] = {"UU", "VV", "WW", "BASELINE", "DATE"};
  for(unsigned i=0; i<nGroupParams; i++)
	{
		std::ostringstream ptypeName;
		ptypeName << "PTYPE" << (i+1);
		setKeywordToString(ptypeName.str().c_str(), parameterNames[i]);
		
		std::ostringstream pscaleName;
		pscaleName << "PSCAL" << (i+1);
		setKeywordToFloat(pscaleName.str().c_str(), 1.0);
		
		if(i != 4) // The 'DATE' zerolevel will be set later
		{
			std::ostringstream pzeroName;
			pzeroName << "PZERO" << (i+1);
			setKeywordToFloat(pzeroName.str().c_str(), 0.0);
		}
  }
	// Set the zero level for the DATE column.
  setKeywordToDouble("PZERO5", floor(_startTime / (60.0*60.0*24.0) + 2400000.5) - 0.5);

  int year, month, day;
	julianDateToYMD(_startTime / (60.0*60.0*24.0) + 2400000.5, year, month, day);
	char dateStr[40];
  std::sprintf(dateStr, "%d-%02d-%02dT00:00:00.0", year, month, day);
	setKeywordToString("DATE-OBS", dateStr);
	
  // The dimensions...
  setKeywordToString("CTYPE2", "COMPLEX");
  setKeywordToFloat("CRVAL2", 1.0);
  setKeywordToFloat("CRPIX2", 1.0);
  setKeywordToFloat("CDELT2", 1.0);
	
  setKeywordToString("CTYPE3", "STOKES");
	setKeywordToFloat("CRVAL3", -5); // Pol type = -5 : linear polarizations
	setKeywordToFloat("CDELT3", -1); // Required for linear pols
  setKeywordToFloat("CRPIX3", 1.0);
	
	setKeywordToString("CTYPE4", "FREQ");
  setKeywordToFloat("CRVAL4", (_bandInfo.channels[_bandInfo.channels.size()/2].chanFreq));
  setKeywordToFloat("CDELT4", _bandInfo.totalBandwidth / (double) _bandInfo.channels.size());
  setKeywordToFloat("CRPIX4", _bandInfo.channels.size()/2 + 1);

	const double
		raDeg = _fieldRA*180.0/M_PI,
		decDeg = _fieldDec*180.0/M_PI;
	
	setKeywordToString("CTYPE5", "RA");
  setKeywordToFloat("CRVAL5", raDeg);
	setKeywordToFloat("CRPIX5", 1);
	setKeywordToFloat("CDELT5", 1);
	
	setKeywordToString("CTYPE6", "DEC");
  setKeywordToFloat("CRVAL6", decDeg);
	setKeywordToFloat("CRPIX6", 1);
	setKeywordToFloat("CDELT6", 1);
	
	// RA and DEC in degrees
	setKeywordToDouble("OBSRA", raDeg);
	setKeywordToDouble("OBSDEC", decDeg);
  setKeywordToFloat("EPOCH", 2000.0);
	
  setKeywordToString("OBJECT", _sourceName);
	
	setKeywordToString("TELESCOP", _telescopeName);
	setKeywordToString("INSTRUME", _telescopeName);
	
	// This is apparently required...
	if(fits_write_history(_fptr, "AIPS WTSCAL =  1.0", &status))
		throwError(status, "Could not write history to FITS file");
	
	_groupHeadersInitialized = true;
}

void FitsWriter::WriteBandInfo(const std::string& name, const std::vector<ChannelInfo>& channels, double refFreq, double totalBandwidth, bool flagRow)
{
	_bandInfo.name = name;
	_bandInfo.channels = channels;
	_bandInfo.refFreq = refFreq;
	_bandInfo.totalBandwidth = totalBandwidth;
	_bandInfo.flagRow = flagRow;
}

void FitsWriter::WriteAntennae(const std::vector<AntennaInfo>& antennae, double time)
{
	_antennae = antennae;
	_antennaDate = time;
}

void FitsWriter::WritePolarizationForLinearPols(bool flagRow)
{
}

void FitsWriter::WriteField(const FieldInfo& field)
{
	_fieldRA = field.referenceDirRA;
	_fieldDec = field.referenceDirDec;
}

void FitsWriter::WriteSource(const SourceInfo &source)
{
	_sourceName = source.name;
}

void FitsWriter::WriteObservation(const std::string& telescopeName, double startTime, double endTime, const std::string& observer, const std::string& scheduleType, const std::string& project, double releaseDate, bool flagRow)
{
	_telescopeName = telescopeName;
	_startTime = startTime;
}

void FitsWriter::WriteHistoryItem(const std::string &commandLine, const std::string &application, const std::vector<std::string> &params)
{
	int status = 0;
  if(fits_write_comment(_fptr, (std::string("Created by ") + application).c_str(), &status))
		throwError(status, std::string("Could not write history comment to uvfits file"));
	
  if(fits_write_comment(_fptr, (std::string("Cmdline: ") + commandLine).c_str(), &status))
		throwError(status, std::string("Could not write history comment to uvfits file"));
}

void FitsWriter::AddRows(size_t count)
{
	if(!_groupHeadersInitialized)
		initGroupHeader();
}

void FitsWriter::WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, const std::complex<float>* data, const bool* flags, const float *weights)
{
	const size_t nGroupParameters = 5;
	
	// 3 dimensions (real,imag,weight), 4 pol, nch
	const size_t nElements = 3 * 4 * _bandInfo.channels.size();
	
	// TODO might be efficient to declare this outside function
	std::vector<float> rowData(nElements + nGroupParameters); 

	rowData[0] = u / VLIGHT;
	rowData[1] = v / VLIGHT;
	rowData[2] = w / VLIGHT;
	rowData[3] = baselineIndex(antenna1+1, antenna2+1);
	double zeroTimeLevel = floor(_startTime / (60.0*60.0*24.0) + 2400000.5) + 0.5;
	rowData[4] = time / (60.0*60.0*24.0) + 2400000.5 - zeroTimeLevel;

	float *rowDataPtr = &rowData[5];
	const float *weightPtr = weights;
	const bool *flagPtr = flags;
	const std::complex<float> *dataPtr = data;
	for(size_t ch=0; ch != _bandInfo.channels.size(); ++ch)
	{
		const std::complex<float> xx = *dataPtr; ++dataPtr;
		const std::complex<float> xy = *dataPtr; ++dataPtr;
		const std::complex<float> yx = *dataPtr; ++dataPtr;
		const std::complex<float> yy = *dataPtr; ++dataPtr;
		const float weightXX = (*flagPtr) ? -(*weightPtr) : (*weightPtr); ++ weightPtr; ++flagPtr;
		const float weightXY = (*flagPtr) ? -(*weightPtr) : (*weightPtr); ++ weightPtr; ++flagPtr;
		const float weightYX = (*flagPtr) ? -(*weightPtr) : (*weightPtr); ++ weightPtr; ++flagPtr;
		const float weightYY = (*flagPtr) ? -(*weightPtr) : (*weightPtr); ++ weightPtr; ++flagPtr;
		
		*rowDataPtr = xx.real();
		++rowDataPtr;
		*rowDataPtr = xx.imag();
		++rowDataPtr;
		*rowDataPtr = weightXX;
		++rowDataPtr;
		
		*rowDataPtr = yy.real();
		++rowDataPtr;
		*rowDataPtr = yy.imag();
		++rowDataPtr;
		*rowDataPtr = weightYY;
		++rowDataPtr;
		
		*rowDataPtr = xy.real();
		++rowDataPtr;
		*rowDataPtr = xy.imag();
		++rowDataPtr;
		*rowDataPtr = weightXY;
		++rowDataPtr;
		
		*rowDataPtr = yx.real();
		++rowDataPtr;
		*rowDataPtr = yx.imag();
		++rowDataPtr;
		*rowDataPtr = weightYX;
		++rowDataPtr;
	}
	
	int status = 0;
	++_nRowsWritten;
	fits_write_grppar_flt(_fptr, _nRowsWritten, 1 ,nElements + nGroupParameters, &rowData[0], &status);
	checkStatus(status);
}

void FitsWriter::writeAntennaTable()
{
  const char *columnNames[] = {"ANNAME", "STABXYZ", "NOSTA", "MNTSTA", "STAXOF",
		"POLTYA", "POLAA", "POLCALA", "POLTYB", "POLAB", "POLCALB"};
  const char *columnFormats[] = {"8A","3D","1J","1J","1E", "1A","1E","3E","1A","1E","3E"};
  const char *columnUnits[] = {"","METERS","","","METERS","","DEGREES","","","DEGREES",""};

	int status = 0;
  fits_create_tbl(_fptr, BINARY_TBL, 0, 11 ,
									const_cast<char**>(columnNames),
									const_cast<char**>(columnFormats),
									const_cast<char**>(columnUnits),
									"AIPS AN", &status);
	checkStatus(status);

  setKeywordToDouble("ARRAYX", _arrayX);
	setKeywordToDouble("ARRAYY", _arrayY);
	setKeywordToDouble("ARRAYZ", _arrayZ);
  setKeywordToFloat("FREQ", (_bandInfo.channels[_bandInfo.channels.size()/2].chanFreq));

  // GSTIAO is the GST at zero hours in the time system of TIMSYS (i.e. UTC)
  double mjd = trunc(_antennaDate / (60.0*60.0*24.0));
	
	// technically, slaGmst takes UT1, but it won't matter here.
  setKeywordToDouble("GSTIA0", slaGmst(mjd)*180.0/M_PI);
  setKeywordToDouble("DEGPDY", 3.60985e2); // Earth's rotation rate

	int year, mon, day;
  julianDateToYMD(_antennaDate / (60.0*60.0*24.0) + 2400000.5, year, mon, day);
	char tempstr[80];
  std::sprintf(tempstr,"%d-%02d-%02dT00:00:00.0", year, mon, day);
  setKeywordToString("RDATE", tempstr);

  setKeywordToDouble("POLARX", 0.0);
  setKeywordToDouble("POLARY", 0.0);
  setKeywordToDouble("UT1UTC", 0.0);
  setKeywordToDouble("DATUTC", 0.0);
	
  setKeywordToString("TIMSYS", "UTC");
  setKeywordToString("ARRNAM", _telescopeName.c_str());
  setKeywordToInt("NUMORB", 0); // number of orbital parameters in table
  setKeywordToInt("NOPCAL", 3); // Nr pol calibration values / IF(N_pcal)
  setKeywordToInt("FREQID", -1); // Frequency setup number
  setKeywordToDouble("IATUTC", 33.0);

  // Write data row by row.
	// CFITSIO automatically adjusts the size of the table
	int row = 1;
  for(std::vector<AntennaInfo>::const_iterator antIter = _antennae.begin(); antIter != _antennae.end(); ++antIter) {
    const char *antennaName = antIter->name.c_str();
		const char *polTypeA = "X", *polTypeB = "Y";
    int mountType = 0;
		float angleA = 0.0, angleB = 90.0, polCal = 0.0;
		double pos[3];
		pos[0] = antIter->x; pos[1] = antIter->y; pos[2] = antIter->z;
		
		fits_write_col_str(_fptr, 1, row, 1, 1, const_cast<char**>(&antennaName), &status);  // ANNAME
		fits_write_col_dbl(_fptr, 2, row, 1, 3, pos, &status); // STABXYZ
		fits_write_col_int(_fptr, 3, row, 1, 1, &row, &status);  // NOSTA
		fits_write_col_int(_fptr, 4, row, 1, 1, &mountType, &status);  // MNTSTA
		fits_write_col_str(_fptr, 6, row, 1, 1, const_cast<char**>(&polTypeA), &status);  // POLTYA
		fits_write_col_flt(_fptr, 7, row, 1, 1, &angleA, &status); // POLAA
		fits_write_col_flt(_fptr, 8, row, 1, 1, &polCal, &status); // POL calA
		fits_write_col_str(_fptr, 9, row, 1, 1, const_cast<char**>(&polTypeB), &status);  // POLTYB
		fits_write_col_flt(_fptr, 10, row, 1, 1, &angleB, &status); // POLAB
		fits_write_col_flt(_fptr, 11, row, 1, 1, &polCal, &status); // POL calB
		checkStatus(status);
		
		++row;
  }
}

void FitsWriter::julianDateToYMD(double jd, int &year, int &month, int &day)
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
