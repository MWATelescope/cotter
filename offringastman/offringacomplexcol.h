#ifndef OFFRINGA_COMPLEX_COLUMN_H
#define OFFRINGA_COMPLEX_COLUMN_H

#include <tables/Tables/DataManError.h>

#include <casa/Arrays/IPosition.h>

#include <map>

#include <stdint.h>

#include "offringastmancol.h"
#include "thread.h"

namespace casa {
	template <typename T> class ROScalarColumn;
}

namespace offringastman {

class OffringaStMan;

template <typename T> class DynamicGausEncoder;

/**
 * A column for storing compressed complex values with an approximate Gaussian distribution.
 * @author André Offringa
 */
class OffringaComplexColumn : public OffringaStManColumn
{
public:
	/**
	 * Create a new column. Internally called by OffringaStMan when creating a
	 * new column.
	 */
  OffringaComplexColumn(OffringaStMan* parent, int dtype) :
		OffringaStManColumn(parent, dtype),
		_bitsPerSymbol(0),
		_symbolsPerCell(0),
		_encoder(0),
		_ant1Col(0),
		_ant2Col(0),
		_fieldCol(0),
		_packedSymbolReadBuffer(0),
		_unpackedSymbolReadBuffer(0),
		_destruct(false)
	{
	}
  
  /** Destructor. */
  virtual ~OffringaComplexColumn();
	
	/** Set the dimensions of values in this column. */
  virtual void setShapeColumn(const casa::IPosition& shape);
	
	/** Get the dimensions of the values in a particular row.
	 * @param rownr The row to get the shape for. */
	virtual casa::IPosition shape(casa::uInt rownr) { return _shape; }
	
	/**
	 * Read the values for a particular row. This will read the required
	 * data and decode it.
	 * @param rowNr The row number to get the values for.
	 * @param dataPtr The array of values, which should be a contiguous array.
	 */
	virtual void getArrayComplexV(casa::uInt rowNr, casa::Array<casa::Complex>* dataPtr);
	
	/**
	 * Write values into a particular row. This will add the values into the cache
	 * and returns immediately afterwards. A pool of threads will encode the items
	 * in the cache and write them to disk.
	 * @param rowNr The row number to write the values to.
	 * @param dataPtr The data pointer, which should be a contiguous array.
	 */
	virtual void putArrayComplexV(casa::uInt rowNr, const casa::Array<casa::Complex>* dataPtr);
	
	/**
	 * Number of bytes per row that this column occupies in the stman file.
	 * @returns Number of bytes.
	 */
	virtual unsigned Stride() const { return (_symbolsPerCell * _bitsPerSymbol + 7) / 8; }

	/**
	 * Prepare this column for reading/writing. Used internally by the stman.
	 */
	virtual void Prepare();
	
	/**
	 * Set the bits per symbol. Should only be called by OffringaStMan.
	 * @param bitsPerSymbol New number of bits per symbol.
	 */
	void SetBitsPerSymbol(unsigned bitsPerSymbol) { _bitsPerSymbol = bitsPerSymbol; }

private:
	OffringaComplexColumn(const OffringaComplexColumn &source) : OffringaStManColumn(0,0) { }
	void operator=(const OffringaComplexColumn &source) { }
	
	struct CacheItem
	{
		CacheItem(unsigned symbolsPerCell) : isBeingWritten(false)
		{
			buffer = new float[symbolsPerCell];
		}
		~CacheItem()
		{
			delete[] buffer;
		}
		
		float *buffer;
		bool isBeingWritten;
		int antenna1, antenna2, fieldId;
	};
	
	struct EncodingThreadFunctor
	{
		void operator()();
		OffringaComplexColumn *parent;
	};
	
	typedef std::map<size_t, CacheItem*> cache_t;
	
	void encodeAndWrite(uint64_t rowIndex, const CacheItem &item, unsigned char* packedSymbolBuffer, unsigned int* unpackedSymbolBuffer);
	bool isWriteItemAvailable(cache_t::iterator &i);
	
	unsigned _bitsPerSymbol, _symbolsPerCell;
	casa::IPosition _shape;
	DynamicGausEncoder<float> *_encoder;
	casa::ROScalarColumn<int> *_ant1Col, *_ant2Col, *_fieldCol;
	unsigned char *_packedSymbolReadBuffer;
	unsigned int *_unpackedSymbolReadBuffer;
	cache_t _cache;
	bool _destruct;
	altthread::mutex _mutex;
	altthread::threadgroup _threadGroup;
	altthread::condition _cacheChangedCondition;
	
	static const unsigned MAX_CACHE_SIZE;
};

} // end of namespace

#endif
