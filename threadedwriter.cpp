#include "threadedwriter.h"

#include <boost/mem_fn.hpp>

ThreadedWriter::ThreadedWriter(std::unique_ptr<Writer>&& parentWriter) :
	ForwardingWriter(std::move(parentWriter)),
	_isWriterReady(false),
	_isBufferReady(false),
	_isFinishing(false),
	_bufferedData(0),
	_bufferedFlags(0),
	_bufferedWeights(0),
	_thread(&ThreadedWriter::writerThreadFunc, this)
{
}

ThreadedWriter::~ThreadedWriter()
{
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_isFinishing = true;
	}
	
	_bufferChangeCondition.notify_all();
	_thread.join();
	
	delete[] _bufferedData;
	delete[] _bufferedFlags;
	delete[] _bufferedWeights;
}

void ThreadedWriter::WriteBandInfo(const std::string &name, const std::vector<Writer::ChannelInfo> &channels, double refFreq, double totalBandwidth, bool flagRow)
{
	_arraySize = channels.size() * 4;
	_bufferedData = new std::complex<float>[_arraySize];
	_bufferedFlags = new bool[_arraySize];
	_bufferedWeights = new float[_arraySize];
	
	ForwardingWriter::WriteBandInfo(name, channels, refFreq, totalBandwidth, flagRow);
}

void ThreadedWriter::AddRows(size_t rowCount)
{
	std::unique_lock<std::mutex> lock(_mutex);
	
	// Wait until the writer is ready AND the buffer is empty
	while(!_isWriterReady || _isBufferReady)
		_bufferChangeCondition.wait(lock);
	
	// Just keep mutex locked (might take time, but this method is not called so often...)
	ParentWriter().AddRows(rowCount);
}

void ThreadedWriter::WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, const std::complex<float>* data, const bool* flags, const float *weights)
{
	std::unique_lock<std::mutex> lock(_mutex);
	
	// Wait until the writer is ready AND the buffer is empty (=not ready)
	while(!_isWriterReady || _isBufferReady)
		_bufferChangeCondition.wait(lock);
	
	_bufferedTime = time;
	_bufferedTimeCentroid = timeCentroid;
	_bufferedAntenna1 = antenna1;
	_bufferedAntenna2 = antenna2;
	_bufferedU = u;
	_bufferedV = v;
	_bufferedW = w;
	_bufferedInterval = interval;
	memcpy(_bufferedData, data, _arraySize * sizeof(std::complex<float>));
	memcpy(_bufferedFlags, flags, _arraySize * sizeof(bool));
	memcpy(_bufferedWeights, weights, _arraySize * sizeof(float));
	
	_isBufferReady = true;
	_bufferChangeCondition.notify_all();
}

void ThreadedWriter::writerThreadFunc()
{
	std::unique_lock<std::mutex> lock(_mutex);
	
	while(!_isFinishing)
	{
		_isWriterReady = true;
		_bufferChangeCondition.notify_all();

		// Wait until a buffer is ready OR the writer is shutting down
		while(!_isBufferReady && !_isFinishing)
			_bufferChangeCondition.wait(lock);
		
		_isWriterReady = false;
		if(_isBufferReady)
		{
			lock.unlock();
			
			ParentWriter().WriteRow(_bufferedTime, _bufferedTimeCentroid, _bufferedAntenna1, _bufferedAntenna2, _bufferedU, _bufferedV, _bufferedW, _bufferedInterval, _bufferedData, _bufferedFlags, _bufferedWeights);
			
			lock.lock();
			_isBufferReady = false;
		}
	}
}
