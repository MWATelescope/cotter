#ifndef OFFRINGA_STORAGE_MAN_COLUMN_H
#define OFFRINGA_STORAGE_MAN_COLUMN_H

#include <tables/Tables/StManColumn.h>

#include <casa/Arrays/IPosition.h>

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>

#include <map>

#include <stdint.h>

class OffringaStMan;

class OffringaStManColumn : public casa::StManColumn
{
public:
  explicit OffringaStManColumn(OffringaStMan* parent, int dtype) :
		casa::StManColumn(dtype),
		_rowOffset(0),
		_parent(parent)
	{
	}
  
  virtual ~OffringaStManColumn() { }
	
  virtual casa::Bool isWritable() const { return true; }
	
	virtual void Prepare() = 0;

	virtual unsigned Stride() const = 0;

	unsigned RowOffset() const { return _rowOffset; }
	
	void SetRowOffset(unsigned offset) {
		_rowOffset = offset; 
		std::cout << columnName() << " offset is now " << offset << '\n';
	}

protected:
	OffringaStMan &parent() const { return *_parent; }
	
private:
	OffringaStManColumn(const OffringaStManColumn &source) : casa::StManColumn(0) { }
	void operator=(const OffringaStManColumn &source) { }
	
	unsigned _rowOffset;
  OffringaStMan *_parent;
};

#endif
