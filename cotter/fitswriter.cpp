#include "fitswriter.h"

#include <cstdio>

FitsWriter::FitsWriter(const char* filename)
{
	/** If the file already exists, remove it */
	FILE *fp = std::fopen(filename,"r");
  if (fp!=NULL) {
    std::fclose(fp);
    std::remove(filename);
  }
  
	int status = 0;
  if(fits_create_file(&_fptr, filename, &status))
		throwError(status, std::string("Cannot open file ") + filename);
}

FitsWriter::~FitsWriter()
{
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
	const unsigned nRows = 0;
  fits_write_grphdr(_fptr, TRUE, FLOAT_IMG, NAXIS, naxes, nGroupParams,
										nRows, TRUE, &status);
	checkStatus(status);
	
  setKeywordToFloat("BSCALE", 1.0);
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
}

void FitsWriter::WritePolarizationForLinearPols(bool flagRow)
{
}

void FitsWriter::WriteField(const FieldInfo& field)
{
	// RA and DEC in degrees
	setKeywordToDouble("OBSRA", field.referenceDirRA*180.0/M_PI);
	setKeywordToDouble("OBSDEC", field.referenceDirDec*180.0/M_PI);
  setKeywordToFloat("EPOCH", 2000.0);
}

void FitsWriter::WriteSource(const SourceInfo &source)
{
  setKeywordToString("OBJECT", source.name);
}

void FitsWriter::WriteObservation(const std::string& telescopeName, double startTime, double endTime, const std::string& observer, const std::string& scheduleType, const std::string& project, double releaseDate, bool flagRow)
{
	setKeywordToString("TELESCOP", telescopeName);
	setKeywordToString("INSTRUME", telescopeName);
}

void FitsWriter::WriteHistoryItem(const std::string &commandLine, const std::string &application, const std::vector<std::string> &params)
{
}

void FitsWriter::AddRows(size_t count)
{
}

void FitsWriter::WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, const std::complex<float>* data, const bool* flags, const float *weights)
{
}
