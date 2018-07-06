#ifndef THREADED_WRITER_H
#define THREADED_WRITER_H

#include "forwardingwriter.h"

#include <string.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

class ThreadedWriter : public ForwardingWriter
{
	public:
		ThreadedWriter(std::unique_ptr<Writer>&& parentWriter);
		
		virtual ~ThreadedWriter() final override;
		
		virtual void WriteBandInfo(const std::string &name, const std::vector<Writer::ChannelInfo> &channels, double refFreq, double totalBandwidth, bool flagRow) final override;
		
		virtual void AddRows(size_t rowCount) final override;
		
		virtual void WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, const std::complex<float>* data, const bool* flags, const float *weights) final override;
		
	private:
		std::condition_variable _bufferChangeCondition;
		std::mutex _mutex;
		bool _isWriterReady, _isBufferReady, _isFinishing;
		
		size_t _arraySize;
		double _bufferedTime, _bufferedTimeCentroid;
		size_t _bufferedAntenna1, _bufferedAntenna2;
		double _bufferedU, _bufferedV, _bufferedW;
		double _bufferedInterval;
		std::complex<float> *_bufferedData;
		bool *_bufferedFlags;
		float *_bufferedWeights;
		
		// Last property, because it needs to be constructed after fields have been initialized
		std::thread _thread;
		
		void writerThreadFunc();
};

#endif
