#ifndef MSWRITER_H
#define MSWRITER_H

#include <complex>
#include <vector>
#include <string>

class Writer
{
	public:
		struct AntennaInfo
		{
			AntennaInfo() { }
			AntennaInfo(const AntennaInfo& source);
			AntennaInfo &operator=(const AntennaInfo& source);
			
			std::string name, station;
			std::string type, mount;
			double x, y, z;
			double diameter;
			bool flag;
		};
		
		struct ChannelInfo
		{
			double chanFreq, chanWidth, effectiveBW, resolution;
		};
		
		struct SourceInfo
		{
			SourceInfo() { }
			int sourceId;
			double time, interval;
			int spectralWindowId, numLines;
			std::string name;
			int calibrationGroup;
			std::string code;
			double directionRA, directionDec;
			double properMotion[2];
		private:
			SourceInfo(const SourceInfo&) { }
			void operator=(const SourceInfo&) { }
		};
		
		struct FieldInfo
		{
			FieldInfo() { }
			std::string name, code;
			double time;
			int numPoly;
			double delayDirRA, delayDirDec;
			double phaseDirRA, phaseDirDec;
			double referenceDirRA, referenceDirDec;
			int sourceId;
			bool flagRow;
		private:
			FieldInfo(const FieldInfo&) { }
			void operator=(const FieldInfo&) { }
		};
		
		virtual ~Writer() { }
		virtual void WriteBandInfo(const std::string &name, const std::vector<ChannelInfo> &channels, double refFreq, double totalBandwidth, bool flagRow) = 0;
		virtual void WriteAntennae(const std::vector<AntennaInfo> &antennae, double time) = 0;
		virtual void WritePolarizationForLinearPols(bool flagRow) = 0;
		virtual void WriteSource(const SourceInfo &source) = 0;
		virtual void WriteField(const FieldInfo& field) = 0;
		virtual void WriteObservation(const std::string& telescopeName, double startTime, double endTime, const std::string& observer, const std::string& scheduleType, const std::string& project, double releaseDate, bool flagRow) = 0;
		virtual void WriteHistoryItem(const std::string &commandLine, const std::string &application) = 0;
		
		virtual void AddRows(size_t count) = 0;
		virtual void WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, const std::complex<float>* data, const bool* flags, const float *weights) = 0;
		
		/**
		 * In case time is regridded, this returns 'true' when the current time samples fit on
		 * the grid. In case it is false, more timesteps should be added. */
		virtual bool IsTimeAligned(size_t antenna1, size_t antenna2) { return true; }
};

class MSWriter : public Writer
{
	public:
		MSWriter(const char *filename);
		~MSWriter();
		
		// TODO write SOURCE table
		
		void WriteBandInfo(const std::string& name, const std::vector<ChannelInfo>& channels, double refFreq, double totalBandwidth, bool flagRow);
		void WriteAntennae(const std::vector<AntennaInfo>& antennae, double time);
		void WritePolarizationForLinearPols(bool flagRow);
		void WriteField(const FieldInfo& field);
		void WriteSource(const SourceInfo &source);
		void WriteObservation(const std::string& telescopeName, double startTime, double endTime, const std::string& observer, const std::string& scheduleType, const std::string& project, double releaseDate, bool flagRow);
		void WriteHistoryItem(const std::string &commandLine, const std::string &application);
		
		void AddRows(size_t count);
		void WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, const std::complex<float>* data, const bool* flags, const float *weights);
		
	private:
		void writeDataDescEntry(size_t spectralWindowId, size_t polarizationId, bool flagRow);
		void writeFeedEntries(const std::vector<Writer::AntennaInfo>& antennae, double time);
		
		class MSWriterData *_data;
		size_t _rowIndex;
		
		size_t _nChannels;
};

#endif
