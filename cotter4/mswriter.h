#ifndef MSWRITER_H
#define MSWRITER_H

#include "writer.h"

#include <complex>
#include <vector>
#include <string>

class MSWriter : public Writer
{
	public:
		MSWriter(const std::string& filename);
		virtual ~MSWriter() final override;
		
		void EnableCompression(size_t dataBitRate, size_t weightBitRate, const std::string& distribution, double distTruncation, const std::string& normalization);
		
		virtual void WriteBandInfo(const std::string& name, const std::vector<ChannelInfo>& channels, double refFreq, double totalBandwidth, bool flagRow) final override;
		virtual void WriteAntennae(const std::vector<AntennaInfo>& antennae, double time) final override;
		virtual void WritePolarizationForLinearPols(bool flagRow) final override;
		virtual void WriteField(const FieldInfo& field) final override;
		virtual void WriteSource(const SourceInfo &source) final override;
		virtual void WriteObservation(const ObservationInfo& observation) final override;
		virtual void WriteHistoryItem(const std::string &commandLine, const std::string &application, const std::vector<std::string> &params) final override;
		
		virtual void AddRows(size_t count) final override;
		virtual void WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, const std::complex<float>* data, const bool* flags, const float *weights) final override;
		
		virtual bool CanWriteStatistics() const final override
		{
			return true;
		}
	private:
		void writeDataDescEntry(size_t spectralWindowId, size_t polarizationId, bool flagRow);
		void writePolarizationForLinearPols();
		void writeFeedEntries();
		void writeBandInfo();
		void writeAntennae();
		void writeSource();
		void writeField();
		void writeObservation();
		void writeHistoryItem();
		void initialize();
		
		class MSWriterData *_data;
		bool _isInitialized;
		size_t _rowIndex;
		
		std::string _filename;
		bool _useDysco;
		
		std::vector<AntennaInfo> _antennae;
		double _antennaDate;
		bool _flagPolarizationRow;
		
		struct {
			std::string name;
			std::vector<ChannelInfo> channels;
			double refFreq;
			double totalBandwidth;
			bool flagRow;
		} _bandInfo;
		
		double _arrayX, _arrayY, _arrayZ;
		SourceInfo _source;
		FieldInfo _field;
		ObservationInfo _observation;
		std::string _historyCommandLine, _historyApplication;
		std::vector<std::string> _historyParams;
};

#endif
