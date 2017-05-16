#ifndef WRITER_H
#define WRITER_H

#include <string>
#include <vector>
#include <complex>

class Writer
{
	public:
		struct AntennaInfo
		{
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
			int sourceId;
			double time, interval;
			int spectralWindowId, numLines;
			std::string name;
			int calibrationGroup;
			std::string code;
			double directionRA, directionDec;
			double properMotion[2];
		};
		
		struct FieldInfo
		{
			std::string name, code;
			double time;
			int numPoly;
			double delayDirRA, delayDirDec;
			double phaseDirRA, phaseDirDec;
			double referenceDirRA, referenceDirDec;
			int sourceId;
			bool flagRow;
		};
		
		struct ObservationInfo
		{
			std::string telescopeName;
			double startTime;
			double endTime;
			std::string observer;
			std::string scheduleType;
			std::string project;
			double releaseDate;
			bool flagRow;
		};
		
		virtual ~Writer() { }
		
		virtual void SetArrayLocation(double x, double y, double z) { }
		virtual void SetOffsetsPerGPUBox(const std::vector<int>& offsets) { }
		
		virtual void WriteBandInfo(const std::string &name, const std::vector<ChannelInfo> &channels, double refFreq, double totalBandwidth, bool flagRow) = 0;
		virtual void WriteAntennae(const std::vector<AntennaInfo> &antennae, double time) = 0;
		virtual void WritePolarizationForLinearPols(bool flagRow) = 0;
		virtual void WriteSource(const SourceInfo& source) = 0;
		virtual void WriteField(const FieldInfo& field) = 0;
		virtual void WriteObservation(const ObservationInfo& observation) = 0;
		virtual void WriteHistoryItem(const std::string &commandLine, const std::string &application, const std::vector<std::string> &params) = 0;
		
		virtual void AddRows(size_t count) = 0;
		virtual void WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, const std::complex<float>* data, const bool* flags, const float *weights) = 0;
		
		virtual bool AreAntennaPositionsLocal() const { return false; }
		virtual bool CanWriteStatistics() const { return false; }
		
		/**
		 * In case time is regridded, this returns 'true' when the current time samples fit on
		 * the grid. In case it is false, more timesteps should be added. */
		virtual bool IsTimeAligned(size_t antenna1, size_t antenna2) { return true; }
};

#endif
