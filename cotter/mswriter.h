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
		virtual void WriteBandInfo(const std::string &name, const std::vector<Writer::ChannelInfo> &channels, double refFreq, double totalBandwidth, bool flagRow) = 0;
		virtual void WriteAntennae(const std::vector<Writer::AntennaInfo> &antennae) = 0;
		virtual void WritePolarizationForLinearPols(bool flagRow) = 0;
		virtual void WriteField(const Writer::FieldInfo& field) = 0;
		
		virtual void WriteRow(double time, size_t antenna1, size_t antenna2, double u, double v, double w, const std::complex<float>* data, const bool* flags, const float *weights) = 0;
};

class MSWriter : public Writer
{
	public:
		MSWriter(const char *filename);
		~MSWriter();
		
		void WriteBandInfo(const std::string &name, const std::vector<ChannelInfo> &channels, double refFreq, double totalBandwidth, bool flagRow);
		void WriteAntennae(const std::vector<AntennaInfo> &antennae);
		void WritePolarizationForLinearPols(bool flagRow);
		void WriteField(const FieldInfo& field);
		
		void WriteRow(double time, size_t antenna1, size_t antenna2, double u, double v, double w, const std::complex<float>* data, const bool* flags, const float *weights);
		
	private:
		class MSWriterData *_data;
		size_t _rowIndex;
		
		size_t _nChannels;
};

#endif
