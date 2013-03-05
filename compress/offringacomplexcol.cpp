#include "offringacomplexcol.h"

#include "offringastman.h"

#include "gausencoder.h"

#include "thread.h"

const unsigned OffringaComplexColumn::MAX_CACHE_SIZE = 64;

OffringaComplexColumn::~OffringaComplexColumn()
{
	ZMutex::scoped_lock lock(_mutex);
	
	// Don't stop threads before cache is empty
	while(!_cache.empty())
		_cacheChangedCondition.wait(lock);
	
	// Signal threads to stop
	_destruct = true;
	_cacheChangedCondition.notify_all();
	
	// Wait for threads to end
	lock.unlock();
	_threadGroup.join_all();
	delete _encoder;
}

void OffringaComplexColumn::setShapeColumn(const casa::IPosition& shape)
{
	_shape = shape;
	_symbolsPerCell = 2;
	for(casa::IPosition::const_iterator i = _shape.begin(); i!=_shape.end(); ++i)
		_symbolsPerCell *= *i;
	delete[] _symbolReadBuffer;
	_symbolReadBuffer = new unsigned char[Stride()];
	parent().RecalculateStride();
	std::cout << "setShapeColumn() : symbols per cell=" << _symbolsPerCell << '\n';
}

void OffringaComplexColumn::getArrayComplexV(casa::uInt rowNr, casa::Array<casa::Complex>* dataPtr)
{
	ZMutex::scoped_lock lock(_mutex);
	cache_t::const_iterator cacheItemPtr = _cache.find(rowNr);
	if(cacheItemPtr == _cache.end())
	{
		lock.unlock();
		// Not in cache
		if(rowNr >= parent().NRowInFile())
		{
			for(casa::Array<casa::Complex>::contiter i=dataPtr->cbegin(); i!=dataPtr->cend(); ++i)
				*i = casa::Complex(0.0, 0.0);
		} else {
			unsigned char *bufferPtr = _symbolReadBuffer;
			parent().ReadCompressedData(rowNr, this, bufferPtr);
			for(casa::Array<casa::Complex>::contiter i=dataPtr->cbegin(); i!=dataPtr->cend(); ++i)
			{
				i->real() = _encoder->Decode(*bufferPtr);
				++bufferPtr;
				i->imag() = _encoder->Decode(*bufferPtr);
				++bufferPtr;
			}
		}
	} else {
		// In cache. Do not release lock, otherwise might be removed from cache during reading.
		const float *cacheBuffer = cacheItemPtr->second->buffer;
		for(casa::Array<casa::Complex>::contiter i=dataPtr->cbegin(); i!=dataPtr->cend(); ++i)
		{
			i->real() = *cacheBuffer;
			++cacheBuffer;
			i->imag() = *cacheBuffer;
			++cacheBuffer;
		}
	}
}
	
void OffringaComplexColumn::putArrayComplexV(casa::uInt rowNr, const casa::Array<casa::Complex>* dataPtr)
{
	CacheItem *item = new CacheItem(Stride());
	float *cacheBuffer = item->buffer;
	for(casa::Array<casa::Complex>::const_contiter i=dataPtr->cbegin(); i!=dataPtr->cend(); ++i)
	{
		*cacheBuffer = i->real();
		++cacheBuffer;
		*cacheBuffer = i->imag();
		++cacheBuffer;
	}
	
	ZMutex::scoped_lock lock(_mutex);
	
	// Wait until there is space available AND the row to be written is not in the cache
	cache_t::iterator cacheItemPtr = _cache.find(rowNr);
	while(_cache.size() >= MAX_CACHE_SIZE || cacheItemPtr != _cache.end())
	{
		_cacheChangedCondition.wait(lock);
		cacheItemPtr = _cache.find(rowNr);
	}
	
	_cache.insert(cache_t::value_type(rowNr, item));
	_cacheChangedCondition.notify_all();
}

void OffringaComplexColumn::encodeAndWrite(uint64_t rowIndex, const float* buffer, unsigned char* symbolBuffer)
{
	for(size_t i=0; i!=_symbolsPerCell; ++i)
	{
		if(std::isfinite(buffer[i]))
			symbolBuffer[i] = _encoder->Encode(buffer[i]);
		else
			symbolBuffer[i] = _encoder->Encode(0.0);
	}
	parent().WriteCompressedData(rowIndex, this, symbolBuffer);
}

void OffringaComplexColumn::Prepare()
{
	delete _encoder;
	_encoder = new GausEncoder<float>(256, parent().GlobalStddev());
	long cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
	EncodingThreadFunctor functor;
	functor.parent = this;
	for(long i=0;i!=cpuCount;++i)
		_threadGroup.create_thread(functor);
	
}

// Continuously write items from the cache into the measurement
// set untill asked to quit.
void OffringaComplexColumn::EncodingThreadFunctor::operator()()
{
	ZMutex::scoped_lock lock(parent->_mutex);
	std::vector<unsigned char> symbolBuffer(parent->Stride());
	cache_t &cache = parent->_cache;
	
	while(!parent->_destruct)
	{
		cache_t::iterator i;
		bool isItemAvailable = parent->isWriteItemAvailable(i);
		while(!isItemAvailable && !parent->_destruct)
		{
			parent->_cacheChangedCondition.wait(lock);
			isItemAvailable = parent->isWriteItemAvailable(i);
		}
		
		if(isItemAvailable)
		{
			size_t rowNr = i->first;
			CacheItem &item = *i->second;
			item.isBeingWritten = true;
			
			lock.unlock();
			parent->encodeAndWrite(rowNr, item.buffer, &symbolBuffer[0]);
			
			lock.lock();
			delete &item;
			cache.erase(i);
			parent->_cacheChangedCondition.notify_all();
		}
	}
}

// This function should only be called with a locked mutex
bool OffringaComplexColumn::isWriteItemAvailable(cache_t::iterator& i)
{
	i = _cache.begin();
	while(i!=_cache.end() && i->second->isBeingWritten)
		++i;
	return (i != _cache.end());
}
