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
		void writeAntennaTable();
		
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
		
		void setKeywordToInt(const char *keywordName, int value) const
		{
			int status = 0;
			if(fits_update_key(_fptr, TINT, keywordName, &value, NULL, &status))
				throwError(status, "Could not write keyword");
		}
		
		void setKeywordToString(const char *keywordName, const char *value) const
		{
			int status = 0;
			if(fits_update_key(_fptr, TSTRING, keywordName, const_cast<char*>(value), NULL, &status))
				throwError(status, "Could not write keyword");
		}
		
		void setKeywordToString(const char *keywordName, const std::string &value) const
		{
			setKeywordToString(keywordName, value.c_str());
		}
		
		static void julianDateToYMD(double jd, int &year, int &month, int &day);
		
		static float baselineIndex(int b1, int b2) {
			return (b2 > 255) ?
				b1*2048 + b2 + 65536 :
				b1*256 + b2;
		}
		
		fitsfile *_fptr;
		
		std::vector<AntennaInfo> _antennae;
		double _antennaDate;
		std::string _telescopeName;
		size_t _nRowsWritten;
		bool _groupHeadersInitialized;
		
		struct {
			std::string name;
			std::vector<ChannelInfo> channels;
			double refFreq;
			double totalBandwidth;
			bool flagRow;
		} _bandInfo;
		
		double _fieldRA, _fieldDec;
		double _startTime;
		std::string _sourceName;
};

#endif
