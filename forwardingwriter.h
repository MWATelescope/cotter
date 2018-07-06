#ifndef FORWARDING_WRITER_H
#define FORWARDING_WRITER_H

#include "writer.h"

#include <memory>

class ForwardingWriter : public Writer
{
	public:
		ForwardingWriter(std::unique_ptr<Writer>&& parentWriter)
		: _writer(std::move(parentWriter))
		{
		}
		
		virtual ~ForwardingWriter() override
		{ }
		
		virtual void WriteAntennae(const std::vector<Writer::AntennaInfo> &antennae, double time) override
		{
			_writer->WriteAntennae(antennae, time);
		}
		
		void WriteBandInfo(const std::string &name, const std::vector<Writer::ChannelInfo> &channels, double refFreq, double totalBandwidth, bool flagRow) override
		{
			_writer->WriteBandInfo(name, channels, refFreq, totalBandwidth, flagRow);
		}

		virtual void WritePolarizationForLinearPols(bool flagRow) override
		{
			_writer->WritePolarizationForLinearPols(flagRow);
		}
		
		virtual void WriteSource(const Writer::SourceInfo &source) override
		{
			_writer->WriteSource(source);
		}
		
		virtual void WriteField(const Writer::FieldInfo& field) override
		{
			_writer->WriteField(field);
		}
		
		virtual void WriteObservation(const ObservationInfo& observation) override
		{
			_writer->WriteObservation(observation);
		}
		
		virtual void SetArrayLocation(double x, double y, double z) override
		{
			_writer->SetArrayLocation(x, y, z);
		}
		
		virtual void SetOffsetsPerGPUBox(const std::vector<int>& offsets) override
		{
			_writer->SetOffsetsPerGPUBox(offsets);
		}
		
		virtual void AddRows(size_t rowCount) override
		{
			_writer->AddRows(rowCount);
		}
		
		virtual void WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, const std::complex<float>* data, const bool* flags, const float *weights) override
		{
			_writer->WriteRow(time, timeCentroid, antenna1, antenna2, u, v, w, interval, data, flags, weights);
		}
		
		virtual void WriteHistoryItem(const std::string &commandLine, const std::string &application, const std::vector<std::string> &params) override
		{
			_writer->WriteHistoryItem(commandLine, application, params);
		}
		
		virtual bool IsTimeAligned(size_t antenna1, size_t antenna2) override {
			return _writer->IsTimeAligned(antenna1, antenna2);
		}
		
		virtual bool AreAntennaPositionsLocal() const override
		{
			return _writer->AreAntennaPositionsLocal();
		}
		
		virtual bool CanWriteStatistics() const override
		{
			return _writer->CanWriteStatistics();
		}
		
		Writer &ParentWriter() { return *_writer; }
		
	private:
		std::unique_ptr<Writer> _writer;
};

#endif
