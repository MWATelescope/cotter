#include "offringacomplexcol.h"

#include "offringastman.h"

#include "dynamicgausencoder.h"

#include "thread.h"

#include <tables/Tables/ScalarColumn.h>

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
	delete _ant1Col;
	delete _ant2Col;
	delete _fieldCol;
}

void OffringaComplexColumn::setShapeColumn(const casa::IPosition& shape)
{
	_shape = shape;
	_symbolsPerCell = 2;
	for(casa::IPosition::const_iterator i = _shape.begin(); i!=_shape.end(); ++i)
		_symbolsPerCell *= *i;
	delete[] _symbolReadBuffer;
	_symbolReadBuffer = new unsigned char[Stride()];
	recalculateStride();
	std::cout << "setShapeColumn() : symbols per cell=" << _symbolsPerCell << '\n';
}

void OffringaComplexColumn::getArrayComplexV(casa::uInt rowNr, casa::Array<casa::Complex>* dataPtr)
{
	int ant1 = (*_ant1Col)(rowNr), ant2 = (*_ant2Col)(rowNr), field = (*_fieldCol)(rowNr);
	
	ZMutex::scoped_lock lock(_mutex);
	cache_t::const_iterator cacheItemPtr = _cache.find(rowNr);
	if(cacheItemPtr == _cache.end()) // Not in cache?
	{
		lock.unlock();
		if(rowNr >= nRowInFile())
		{
			for(casa::Array<casa::Complex>::contiter i=dataPtr->cbegin(); i!=dataPtr->cend(); ++i)
				*i = casa::Complex(0.0, 0.0);
		} else {
			unsigned char *bufferPtr = _symbolReadBuffer;
			readCompressedData(rowNr, bufferPtr);
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
	item->antenna1 = (*_ant1Col)(rowNr);
	item->antenna2 = (*_ant2Col)(rowNr);
	item->fieldId = (*_fieldCol)(rowNr);
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
	writeCompressedData(rowIndex, symbolBuffer);
}

void OffringaComplexColumn::Prepare()
{
	delete _encoder;
	delete _ant1Col;
	delete _ant2Col;
	delete _fieldCol;
	
	_encoder = new GausEncoder<float>(256, 1.0);
	
	casa::Table &table = parent().table();
	_ant1Col = new casa::ROScalarColumn<int>(table, "ANTENNA1");
	_ant2Col = new casa::ROScalarColumn<int>(table, "ANTENNA2");
	_fieldCol = new casa::ROScalarColumn<int>(table, "FIELD_ID");
	
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
	// (note that the parent of the functor is the column, not the stman)
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
