#ifndef OFFRINGA_STORAGE_MAN_COLUMN_H
#define OFFRINGA_STORAGE_MAN_COLUMN_H

#include <tables/Tables/StManColumn.h>

#include <casa/Arrays/IPosition.h>

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>

#include <map>

#include <stdint.h>

namespace offringastman {
	
class OffringaStMan;

/**
 * Base class for columns of the OffringaStMan.
 * @author André Offringa
 */
class OffringaStManColumn : public casa::StManColumn
{
public:
	/**
	 * Constructor, to be overloaded by subclass.
	 * @param parent The parent stman to which this column belongs.
	 * @param dtype The column's type as defined by Casacore.
	 */
  explicit OffringaStManColumn(OffringaStMan* parent, int dtype) :
		casa::StManColumn(dtype),
		_rowOffset(0),
		_parent(parent)
	{
	}
  
  /** Destructor */
  virtual ~OffringaStManColumn() { }
	
	/**
	 * Whether this column is writable
	 * @returns @c true
	 */
  virtual casa::Bool isWritable() const { return true; }
	
	/**
	 * Prepare this column for reading/writing.
	 * This function will be called to get the stman ready for
	 * reading and writing.
	 */
	virtual void Prepare() = 0;

	/**
	 * Get the number of bytes that this column will store for each row.
	 * @returns The stride.
	 * @see OffringaStMan::recalculateStride()
	 */
	virtual unsigned Stride() const = 0;

	/**
	 * Get the starting point within a row at which this column needs to
	 * write/read his data. Data for this column for a given row will be stored
	 * at: RowOffset() + Stride() * rowindex + size of headers
	 * @returns The offset
	 */
	unsigned RowOffset() const { return _rowOffset; }
	
	/**
	 * Set the row offset. To be used by the parent to store the offset
	 * once the offset is known.
	 */
	void SetRowOffset(unsigned offset) {
		_rowOffset = offset; 
	}
	
	/**
	 * Get number of bytes needed for column header of this column. This is
	 * excluding the generic column header.
	 * @returns Size of column header
	 */
	virtual size_t ExtraHeaderSize() const { return 0; }

protected:
	/** Get the storage manager for this column */
	OffringaStMan &parent() const { return *_parent; }
	/**
	 * Read a row of compressed data from the stman file.
	 * @param rowIndex The index of the row to read.
	 * @param dest The destination buffer, should be at least of size Stride().
	 */
	void readCompressedData(size_t rowIndex, unsigned char* dest);
	/**
	 * Write a row of compressed data to the stman file.
	 * @param rowIndex The index of the row to write.
	 * @param data The data buffer containing Stride() bytes.
	 */
	void writeCompressedData(size_t rowIndex, const unsigned char* data);
	/**
	 * Get the actual number of rows in the file.
	 * @see nRow()
	 * @returns The actual stored amount of rows.
	 */
	uint64_t nRowInFile() const;
	/**
	 * Get the RMS table from the parent.
	 * @returns The RMS table that maps baseline to RMS value.
	 */
	const class RMSTable &rmsTable() const;
	
private:
	OffringaStManColumn(const OffringaStManColumn &source) : casa::StManColumn(0), _isRemoved(false) { }
	void operator=(const OffringaStManColumn &source) { }
	
	bool _isRemoved;
	unsigned _rowOffset;
  OffringaStMan *_parent;
};

} // end of namespace

#include "offringastman.h"

namespace offringastman {
	
inline void OffringaStManColumn::readCompressedData(size_t rowIndex, unsigned char* dest)
{
	_parent->readCompressedData(rowIndex, this, dest);
}

inline void OffringaStManColumn::writeCompressedData(size_t rowIndex, const unsigned char* data)
{
	_parent->writeCompressedData(rowIndex, this, data);
}

inline uint64_t OffringaStManColumn::nRowInFile() const
{
	return _parent->nRowInFile();
}

inline const RMSTable& OffringaStManColumn::rmsTable() const
{
	return _parent->GetRMSTable();
}

} // end of namespace

#endif
