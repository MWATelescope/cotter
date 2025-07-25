#ifndef AVERAGING_MS_WRITER_H
#define AVERAGING_MS_WRITER_H

#include "writer.h"

#include <iostream>
#include <memory>

class UVWCalculater
{
	public:
    virtual ~UVWCalculater() = default;
		virtual void CalculateUVW(double date, size_t antenna1, size_t antenna2, double &u, double &v, double &w) = 0;
};

class AveragingWriter final : public Writer
{
	public:
		AveragingWriter(std::unique_ptr<Writer>&& writer, size_t timeCount, size_t freqAvgFactor, UVWCalculater& uvwCalculater)
		: _writer(std::move(writer)), _timeAvgFactor(timeCount), _freqAvgFactor(freqAvgFactor), _rowsAdded(0),
		_originalChannelCount(0), _avgChannelCount(0), _antennaCount(0), _uvwCalculater(uvwCalculater)
		{
		}
		
		virtual ~AveragingWriter() final override
		{
			destroyBuffers();
		}
		
		virtual void WriteBandInfo(const std::string &name, const std::vector<Writer::ChannelInfo> &channels, double refFreq, double totalBandwidth, bool flagRow) final override
		{
			if(channels.size()%_freqAvgFactor != 0)
			{
				std::cout << " Warning: channels averaging factor is not a multiply of total number of channels. Last channel(s) will be left out.\n";
			}
			
			_avgChannelCount = channels.size() / _freqAvgFactor;
			_originalChannelCount = channels.size();
			
			std::vector<Writer::ChannelInfo> avgChannels(_avgChannelCount);
			for(size_t ch=0; ch!=_avgChannelCount; ++ch)
			{
				Writer::ChannelInfo channel;
				channel.chanFreq = 0.0;
				channel.chanWidth = 0.0;
				channel.effectiveBW = 0.0;
				channel.resolution = 0.0;
				for(size_t i=0; i!=_freqAvgFactor; ++i)
				{
					const Writer::ChannelInfo& curChannel = channels[ch*_freqAvgFactor + i];
					channel.chanFreq += curChannel.chanFreq;
					channel.chanWidth += curChannel.chanWidth;
					channel.effectiveBW += curChannel.effectiveBW;
					channel.resolution += curChannel.resolution;
				}
				
				channel.chanFreq /= (double) _freqAvgFactor;
				
				avgChannels[ch] = channel;
			}
			
			_writer->WriteBandInfo(name, avgChannels, refFreq, totalBandwidth, flagRow);
			
			if(_antennaCount != 0)
				initBuffers();
		}
		
		virtual void WriteAntennae(const std::vector<Writer::AntennaInfo> &antennae, double time) final override
		{
			_writer->WriteAntennae(antennae, time);
			
			_antennaCount = antennae.size();
			if(_originalChannelCount != 0)
				initBuffers();
		}
		
		virtual void WritePolarizationForLinearPols(bool flagRow) final override
		{
			_writer->WritePolarizationForLinearPols(flagRow);
		}
		
		virtual void WriteSource(const Writer::SourceInfo &source) final override
		{
			_writer->WriteSource(source);
		}
		
		virtual void WriteField(const Writer::FieldInfo& field) final override
		{
			_writer->WriteField(field);
		}
		
		virtual void WriteObservation(const ObservationInfo& observation) final override
		{
			_writer->WriteObservation(observation);
		}
		
		virtual void SetArrayLocation(double x, double y, double z) final override
		{
			_writer->SetArrayLocation(x, y, z);
		}
		
		virtual void AddRows(size_t rowCount) final override
		{
			if(_rowsAdded == 0)
				_writer->AddRows(rowCount);
			_rowsAdded++;
			if(_rowsAdded == _timeAvgFactor)
				_rowsAdded=0;
		}
		
		virtual void WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, const std::complex<float>* data, const bool* flags, const float *weights) final override;
		
		virtual void WriteHistoryItem(const std::string &commandLine, const std::string &application, const std::vector<std::string> &params) final override
		{
			_writer->WriteHistoryItem(commandLine, application, params);
		}
		
		virtual bool IsTimeAligned(size_t antenna1, size_t antenna2) final override {
			const Buffer &buffer = getBuffer(antenna1, antenna2);
			return buffer._rowTimestepCount==0;
		}
		
		virtual bool AreAntennaPositionsLocal() const final override
		{
			return _writer->AreAntennaPositionsLocal();
		}
		virtual bool CanWriteStatistics() const final override
		{
			return _writer->CanWriteStatistics();
		}
	private:
		struct Buffer
		{
			Buffer(size_t avgChannelCount)
			{
				_rowData = new std::complex<float>[avgChannelCount*4];
				_flaggedAndUnflaggedData = new std::complex<float>[avgChannelCount*4];
				_rowFlags = new bool[avgChannelCount*4];
				_rowWeights = new float[avgChannelCount*4];
				_rowCounts = new size_t[avgChannelCount*4];
				initZero(avgChannelCount);
			}
			
			~Buffer()
			{
				delete[] _rowData;
				delete[] _flaggedAndUnflaggedData;
				delete[] _rowFlags;
				delete[] _rowWeights;
				delete[] _rowCounts;
			}
			
			void initZero(size_t avgChannelCount)
			{
				_rowTime = 0.0;
				_rowTimestepCount = 0;
				_interval = 0.0;
				for(size_t ch=0; ch!=avgChannelCount*4; ++ch)
				{
					_rowData[ch] = 0.0;
					_flaggedAndUnflaggedData[ch] = 0.0;
					_rowFlags[ch] = false;
					_rowWeights[ch] = 0.0;
					_rowCounts[ch] = 0;
				}
			}
			
			double _rowTime;
			size_t _rowTimestepCount;
			double _interval;
			std::complex<float> *_rowData, *_flaggedAndUnflaggedData;
			bool *_rowFlags;
			float *_rowWeights;
			size_t *_rowCounts;
		};
		
		void writeCurrentTimestep(size_t antenna1, size_t antenna2)
		{
			Buffer& buffer = getBuffer(antenna1, antenna2);
			double time = buffer._rowTime / buffer._rowTimestepCount;
			double u, v, w;
			_uvwCalculater.CalculateUVW(time, antenna1, antenna2, u, v, w);
			
			for(size_t ch=0;ch!=_avgChannelCount*4;++ch)
			{
				if(buffer._rowCounts[ch]==0)
				{
					buffer._rowData[ch] = std::complex<float>(
						buffer._flaggedAndUnflaggedData[ch].real() / (buffer._rowTimestepCount*_freqAvgFactor),
						buffer._flaggedAndUnflaggedData[ch].imag() / (buffer._rowTimestepCount*_freqAvgFactor));
					buffer._rowFlags[ch] = true;
				} else {
					buffer._rowData[ch] = std::complex<float>(
						buffer._rowData[ch].real()/buffer._rowWeights[ch],
						buffer._rowData[ch].imag()/buffer._rowWeights[ch]);
					buffer._rowFlags[ch] = false;
				}
			}
			
			_writer->WriteRow(time, time, antenna1, antenna2, u, v, w, buffer._interval, buffer._rowData, buffer._rowFlags, buffer._rowWeights);
			
			buffer.initZero(_avgChannelCount);
		}
		
		Buffer &getBuffer(size_t antenna1, size_t antenna2)
		{
			return *_buffers[antenna1*_antennaCount + antenna2];
		}
		
		void setBuffer(size_t antenna1, size_t antenna2, Buffer *buffer)
		{
			_buffers[antenna1*_antennaCount + antenna2] = buffer;
		}
		
		void initBuffers()
		{
			destroyBuffers();
			_buffers.resize(_antennaCount*_antennaCount);
			for(size_t antenna1=0; antenna1!=_antennaCount; ++antenna1)
			{
				for(size_t antenna2=0; antenna2!=antenna1; ++antenna2)
					setBuffer(antenna1, antenna2, 0);
				
				for(size_t antenna2=antenna1; antenna2!=_antennaCount; ++antenna2)
				{
					Buffer *buffer = new Buffer(_avgChannelCount);
					setBuffer(antenna1, antenna2, buffer);
				}
			}
		}
		
		void destroyBuffers()
		{
			if(!_buffers.empty())
			{
				for(size_t antenna1=0; antenna1!=_antennaCount; ++antenna1)
				{
					for(size_t antenna2=antenna1; antenna2!=_antennaCount; ++antenna2)
						delete &getBuffer(antenna1, antenna2);
				}
			}
			_buffers.clear();
		}
		
		std::unique_ptr<Writer> _writer;
		size_t _timeAvgFactor, _freqAvgFactor, _rowsAdded;
		size_t _originalChannelCount, _avgChannelCount, _antennaCount;
		UVWCalculater& _uvwCalculater;
		std::vector<Buffer*> _buffers;
};

#endif
