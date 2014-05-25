#ifndef FORWARDING_WRITER_H
#define FORWARDING_WRITER_H

#include "writer.h"

class ForwardingWriter : public Writer
{
	public:
		ForwardingWriter(Writer *parentWriter)
		: _writer(parentWriter)
		{
		}
		
		virtual ~ForwardingWriter()
		{
			delete _writer;
		}
		
		virtual void WriteAntennae(const std::vector<Writer::AntennaInfo> &antennae, double time)
		{
			_writer->WriteAntennae(antennae, time);
		}
		
		void WriteBandInfo(const std::string &name, const std::vector<Writer::ChannelInfo> &channels, double refFreq, double totalBandwidth, bool flagRow)
		{
			_writer->WriteBandInfo(name, channels, refFreq, totalBandwidth, flagRow);
		}

		virtual void WritePolarizationForLinearPols(bool flagRow)
		{
			_writer->WritePolarizationForLinearPols(flagRow);
		}
		
		virtual void WriteSource(const Writer::SourceInfo &source)
		{
			_writer->WriteSource(source);
		}
		
		virtual void WriteField(const Writer::FieldInfo& field)
		{
			_writer->WriteField(field);
		}
		
		virtual void WriteObservation(const std::string& telescopeName, double startTime, double endTime, const std::string& observer, const std::string& scheduleType, const std::string& project, double releaseDate, bool flagRow)
		{
			_writer->WriteObservation(telescopeName, startTime, endTime, observer, scheduleType, project, releaseDate, flagRow);
		}
		
		virtual void SetArrayLocation(double x, double y, double z)
		{
			_writer->SetArrayLocation(x, y, z);
		}
		
		virtual void SetOffsetsPerGPUBox(const std::vector<int>& offsets)
		{
			_writer->SetOffsetsPerGPUBox(offsets);
		}
		
		virtual void AddRows(size_t rowCount)
		{
			_writer->AddRows(rowCount);
		}
		
		virtual void WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, const std::complex<float>* data, const bool* flags, const float *weights)
		{
			_writer->WriteRow(time, timeCentroid, antenna1, antenna2, u, v, w, interval, data, flags, weights);
		}
		
		virtual void WriteHistoryItem(const std::string &commandLine, const std::string &application, const std::vector<std::string> &params)
		{
			_writer->WriteHistoryItem(commandLine, application, params);
		}
		
		virtual bool IsTimeAligned(size_t antenna1, size_t antenna2) {
			return _writer->IsTimeAligned(antenna1, antenna2);
		}
		
		virtual bool AreAntennaPositionsLocal() const
		{
			return _writer->AreAntennaPositionsLocal();
		}
		
		virtual bool CanWriteStatistics() const
		{
			return _writer->CanWriteStatistics();
		}
		
		Writer &ParentWriter() { return *_writer; }
		
	private:
		Writer *_writer;
};

#endif
