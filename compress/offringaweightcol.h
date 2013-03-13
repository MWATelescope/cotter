#ifndef OFFRINGA_WEIGHT_COLUMN_H
#define OFFRINGA_WEIGHT_COLUMN_H

#include "offringastmancol.h"

#include <tables/Tables/DataManError.h>

#include <casa/Arrays/IPosition.h>

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>

#include <map>

#include <stdint.h>

namespace offringastman
{

class OffringaStMan;

template <typename T> class WeightEncoder;

/**
 * A column for storing compressed weight values.
 * Weights are stored with a single scale value and are uniformly quantized. See the
 * WeightEncoder for details.
 * @author André Offringa
 */
class OffringaWeightColumn : public OffringaStManColumn
{
public:
	/**
	 * Create a new column. Internally called by OffringaStMan when creating a
	 * new column.
	 */
  OffringaWeightColumn(OffringaStMan* parent, int dtype) :
		OffringaStManColumn(parent, dtype),
		_bitsPerSymbol(6),
		_symbolsPerCell(0),
		_encoder(0),
		_symbolBuffer(),
		_packBuffer(0),
		_dataCopyBuffer()
	{
	}
  
  /** Destructor. */
  virtual ~OffringaWeightColumn();
	
	/** Set the dimensions of values in this column. */
  virtual void setShapeColumn(const casa::IPosition& shape);
	
	/** Get the dimensions of the values in a particular row.
	 * @param rownr The row to get the shape for. */
	virtual casa::IPosition shape(casa::uInt rownr) { return _shape; }
	
	/**
	 * Read the weight values for a particular row. 
	 * @param rowNr The row number to get the values for.
	 * @param dataPtr The array of values, which should be a contiguous array.
	 */
	virtual void getArrayfloatV(casa::uInt rowNr, casa::Array<float>* dataPtr);
	
	/**
	 * Write values into a particular row.
	 * @param rowNr The row number to write the values to.
	 * @param dataPtr The data pointer, which should be a contiguous array.
	 */
	virtual void putArrayfloatV(casa::uInt rowNr, const casa::Array<float>* dataPtr);
	
	/**
	 * Number of bytes per row that this column occupies in the stman file.
	 * @returns Number of bytes.
	 */
	virtual unsigned Stride() const { return (_symbolsPerCell * _bitsPerSymbol + 7) / 8 + sizeof(float); }

	/**
	 * Prepare this column for reading/writing. Used internally by the stman.
	 */
	virtual void Prepare();

private:
	OffringaWeightColumn(const OffringaWeightColumn &source) : OffringaStManColumn(0,0) { }
	void operator=(const OffringaWeightColumn &source) { }
	
	unsigned _bitsPerSymbol, _symbolsPerCell;
	casa::IPosition _shape;
	WeightEncoder<float> *_encoder;
	std::vector<unsigned> _symbolBuffer;
	unsigned char *_packBuffer;
	std::vector<float> _dataCopyBuffer;
};

} // end of namespace

#endif
