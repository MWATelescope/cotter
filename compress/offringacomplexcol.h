#ifndef OFFRINGA_COMPLEX_COLUMN_H
#define OFFRINGA_COMPLEX_COLUMN_H

#include <tables/Tables/DataManError.h>

#include <casa/Arrays/IPosition.h>

#include <map>

#include <stdint.h>

#include "offringastmancol.h"
#include "thread.h"

class OffringaStMan;

template <typename T> class GausEncoder;

class OffringaComplexColumn : public OffringaStManColumn
{
public:
  explicit OffringaComplexColumn(OffringaStMan* parent, int dtype) :
		OffringaStManColumn(parent, dtype),
		_bitsPerSymbol(8),
		_symbolsPerCell(0),
		_encoder(0),
		_symbolReadBuffer(0),
		_destruct(false)
	{
	}
  
  virtual ~OffringaComplexColumn();
	
  virtual casa::Bool isWritable() const { return true; }
	
  virtual void setShapeColumn(const casa::IPosition& shape);
	
	virtual casa::IPosition shape(casa::uInt rownr) { return _shape; }
	
	virtual void getArrayComplexV(casa::uInt rowNr, casa::Array<casa::Complex>* dataPtr);
	
	virtual void putArrayComplexV(casa::uInt rowNr, const casa::Array<casa::Complex>* dataPtr);
	
	virtual unsigned Stride() const { return (_symbolsPerCell * _bitsPerSymbol + 7) / 8; }

	virtual void Prepare();

private:
	OffringaComplexColumn(const OffringaComplexColumn &source) : OffringaStManColumn(0,0) { }
	void operator=(const OffringaComplexColumn &source) { }
	
	struct CacheItem
	{
		CacheItem(unsigned stride) : isBeingWritten(false)
		{
			buffer = new float[stride];
		}
		~CacheItem()
		{
			delete[] buffer;
		}
		
		float *buffer;
		bool isBeingWritten;
	};
	
	struct EncodingThreadFunctor
	{
		void operator()();
		OffringaComplexColumn *parent;
	};
	
	typedef std::map<size_t, CacheItem*> cache_t;
	
	void encodeAndWrite(uint64_t rowIndex, const float* buffer, unsigned char* symbolBuffer);
	bool isWriteItemAvailable(cache_t::iterator &i);
	
	unsigned _bitsPerSymbol, _symbolsPerCell;
	casa::IPosition _shape;
	GausEncoder<float> *_encoder;
	unsigned char *_symbolReadBuffer;
	cache_t _cache;
	bool _destruct;
	ZMutex _mutex;
	ZThreadGroup _threadGroup;
	ZCondition _cacheChangedCondition;
	
	static const unsigned MAX_CACHE_SIZE;
};

#endif
