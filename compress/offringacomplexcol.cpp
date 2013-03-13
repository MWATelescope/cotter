#include "offringacomplexcol.h"

#include "offringastman.h"

#include "dynamicgausencoder.h"

#include "thread.h"
#include "bytepacker.h"

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
	delete[] _packedSymbolReadBuffer;
	_packedSymbolReadBuffer = new unsigned char[Stride()];
	delete[] _unpackedSymbolReadBuffer;
	_unpackedSymbolReadBuffer = new unsigned int[Stride()];
	recalculateStride();
	std::cout << "setShapeColumn() : symbols per cell=" << _symbolsPerCell << '\n';
}

void OffringaComplexColumn::getArrayComplexV(casa::uInt rowNr, casa::Array<casa::Complex>* dataPtr)
{
	const int ant1 = (*_ant1Col)(rowNr), ant2 = (*_ant2Col)(rowNr), fieldId = (*_fieldCol)(rowNr);
	const RMSTable::rms_t rms = rmsTable().Value(ant1, ant2, fieldId);
	
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
			readCompressedData(rowNr, _packedSymbolReadBuffer);
			BytePacker::unpack(_bitsPerSymbol, _unpackedSymbolReadBuffer, _packedSymbolReadBuffer, _symbolsPerCell);
			_encoder->Decode(rms, reinterpret_cast<casa::Complex::value_type*>(dataPtr->data()), _unpackedSymbolReadBuffer, _symbolsPerCell);
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

void OffringaComplexColumn::encodeAndWrite(uint64_t rowIndex, const CacheItem &item, unsigned char* packedSymbolBuffer, unsigned int* unpackedSymbolBuffer)
{
	const RMSTable::rms_t rms = rmsTable().Value(item.antenna1, item.antenna2, item.fieldId);
	for(size_t i=0; i!=_symbolsPerCell; ++i)
	{
		if(!std::isfinite(item.buffer[i]))
			item.buffer[i] = 0.0;
	}
	_encoder->Encode(rms, unpackedSymbolBuffer, item.buffer, _symbolsPerCell);
	BytePacker::pack(_bitsPerSymbol, packedSymbolBuffer, unpackedSymbolBuffer, _symbolsPerCell);
	writeCompressedData(rowIndex, packedSymbolBuffer);
}

void OffringaComplexColumn::Prepare()
{
	delete _encoder;
	delete _ant1Col;
	delete _ant2Col;
	delete _fieldCol;
	
	_encoder = new DynamicGausEncoder<float>(1 << _bitsPerSymbol);
	
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
	std::vector<unsigned char> packedSymbolBuffer(parent->Stride());
	std::vector<unsigned> unpackedSymbolBuffer(parent->_symbolsPerCell);
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
			parent->encodeAndWrite(rowNr, item, &packedSymbolBuffer[0], &unpackedSymbolBuffer[0]);
			
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
