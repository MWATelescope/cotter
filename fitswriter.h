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
		FitsWriter(const std::string& filename);
		virtual ~FitsWriter() final override;
		
		virtual void WriteBandInfo(const std::string& name, const std::vector<ChannelInfo>& channels, double refFreq, double totalBandwidth, bool flagRow) final override;
		virtual void WriteAntennae(const std::vector<AntennaInfo>& antennae, double time) final override;
		virtual void WritePolarizationForLinearPols(bool flagRow) final override;
		virtual void WriteField(const FieldInfo& field) final override;
		virtual void WriteSource(const SourceInfo &source) final override;
		virtual void WriteObservation(const ObservationInfo& observation) final override;
		virtual void WriteHistoryItem(const std::string &commandLine, const std::string &application, const std::vector<std::string> &params) final override;
		virtual void SetArrayLocation(double x, double y, double z) final override
		{
			_arrayX = x;
			_arrayY = y;
			_arrayZ = z;
		}
		
		virtual void AddRows(size_t count) final override;
		virtual void WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, const std::complex<float>* data, const bool* flags, const float *weights) final override;
		virtual bool AreAntennaPositionsLocal() const final override { return true; }
		
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
		
		/**
		 * Returns the beginnin of the JD to which the first timestep belongs.
		 * This is calculated by truncating the time within the day.
		 * The times associated with the visibilities are then increments to
		 * this truncated start-of-jd time. JDs start at 12:00h on a day.
		 */
		double timeZeroLevel() const
		{
			return floor(_startTime / (60.0*60.0*24.0) + 2400000.5) + 0.5;
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
		double _arrayX, _arrayY, _arrayZ;
		std::string _sourceName, _historyCommandLine, _historyApplication;
};

#endif
