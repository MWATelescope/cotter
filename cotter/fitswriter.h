#ifndef FITSWRITER_H
#define FITSWRITER_H

#include "fitsuser.h"
#include "writer.h"

#include <fitsio.h>

#include <complex>
#include <vector>
#include <string>

class FitsWriter : public Writer, private FitsUser
{
	public:
		FitsWriter(const char *filename);
		virtual ~FitsWriter();
		
		virtual void WriteBandInfo(const std::string& name, const std::vector<ChannelInfo>& channels, double refFreq, double totalBandwidth, bool flagRow);
		virtual void WriteAntennae(const std::vector<AntennaInfo>& antennae, double time);
		virtual void WritePolarizationForLinearPols(bool flagRow);
		virtual void WriteField(const FieldInfo& field);
		virtual void WriteSource(const SourceInfo &source);
		virtual void WriteObservation(const std::string& telescopeName, double startTime, double endTime, const std::string& observer, const std::string& scheduleType, const std::string& project, double releaseDate, bool flagRow);
		virtual void WriteHistoryItem(const std::string &commandLine, const std::string &application, const std::vector<std::string> &params);
		
		virtual void AddRows(size_t count);
		virtual void WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, const std::complex<float>* data, const bool* flags, const float *weights);
		
	private:
		void initGroupHeader();
		void setKeywordToDouble(const char *keywordName, double value) const
		{
			int status = 0;
			if(fits_update_key(_fptr, TDOUBLE, keywordName, &value, NULL, &status))
				throwError(status, "Could not write keyword");
		}
		void setKeywordToFloat(const char *keywordName, float value) const
		{
			int status = 0;
			if(fits_update_key(_fptr, TFLOAT, keywordName, &value, NULL, &status))
				throwError(status, "Could not write keyword");
		}
		void setKeywordToString(const char *keywordName, const char *value) const
		{
			int status = 0;
			if(fits_update_key(_fptr, TFLOAT, keywordName, const_cast<char*>(value), NULL, &status))
				throwError(status, "Could not write keyword");
		}
		void setKeywordToString(const char *keywordName, const std::string &value) const
		{
			setKeywordToString(keywordName, value.c_str());
		}
		
		fitsfile *_fptr;
		
		struct {
			std::string name;
			std::vector<ChannelInfo> channels;
			double refFreq;
			double totalBandwidth;
			bool flagRow;
		} _bandInfo;
};

#endif
