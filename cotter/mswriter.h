#ifndef MSWRITER_H
#define MSWRITER_H

#include <complex>
#include <vector>
#include <string>

class MSWriter
{
	public:
		MSWriter(const char *filename);
		~MSWriter();
		
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
		
		void WriteBandInfo(const std::string &name, const std::vector<ChannelInfo> &channels, double refFreq, double totalBandwidth, bool flagRow);
		void WriteAntennae(const std::vector<AntennaInfo> &antennae);
		void WritePolarizationForLinearPols(bool flagRow);
		void WriteField(const FieldInfo& field);
		
		void WriteRow(double time, size_t antenna1, size_t antenna2, const std::complex<float>* data, const bool* flags);
		
	private:
		class MSWriterData *_data;
		size_t _rowIndex;
		
		size_t _nChannels;
};

#endif
