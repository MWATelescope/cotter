#include "fitswriter.h"

FitsWriter::FitsWriter(const char* filename)
{
	int status = 0;
	if(fits_open_file(&_fptr, filename, READONLY, &status))
		throwError(status, std::string("Cannot open file ") + filename);
}

FitsWriter::~FitsWriter()
{
	int status;
	if(fits_close_file(_fptr, &status))
		throwError(status, std::string("Cannot close file "));
}

void FitsWriter::WriteBandInfo(const std::string& name, const std::vector<ChannelInfo>& channels, double refFreq, double totalBandwidth, bool flagRow)
{
}

void FitsWriter::WriteAntennae(const std::vector<AntennaInfo>& antennae, double time)
{
}

void FitsWriter::WritePolarizationForLinearPols(bool flagRow)
{
}

void FitsWriter::WriteField(const FieldInfo& field)
{
}

void FitsWriter::WriteSource(const SourceInfo &source)
{
}

void FitsWriter::WriteObservation(const std::string& telescopeName, double startTime, double endTime, const std::string& observer, const std::string& scheduleType, const std::string& project, double releaseDate, bool flagRow)
{
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
