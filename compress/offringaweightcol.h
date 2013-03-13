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

class OffringaWeightColumn : public OffringaStManColumn
{
public:
  explicit OffringaWeightColumn(OffringaStMan* parent, int dtype) :
		OffringaStManColumn(parent, dtype),
		_bitsPerSymbol(6),
		_symbolsPerCell(0),
		_encoder(0),
		_symbolBuffer(),
		_packBuffer(0),
		_dataCopyBuffer()
	{
	}
  
  virtual ~OffringaWeightColumn();
	
  virtual void setShapeColumn(const casa::IPosition& shape);
	
	virtual casa::IPosition shape(casa::uInt rownr) { return _shape; }
	
	virtual void getArrayfloatV(casa::uInt rowNr, casa::Array<float>* dataPtr);
	
	virtual void putArrayfloatV(casa::uInt rowNr, const casa::Array<float>* dataPtr);
	
	virtual unsigned Stride() const { return (_symbolsPerCell * _bitsPerSymbol + 7) / 8 + sizeof(float); }

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
