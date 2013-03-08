#ifndef OFFRINGA_STORAGE_MANAGER_H
#define OFFRINGA_STORAGE_MANAGER_H

#include <tables/Tables/DataManager.h>
#include <tables/Tables/DataManError.h>

#include <casa/Containers/Record.h>

#include <fstream>
#include <vector>

#include <auto_ptr.h>
#include <stdint.h>

#include "thread.h"

class OffringaStManColumn;

/**
 * A storage manager for the compression technique to be described in
 * Offringa et al (2013).
 * 
 * TODO describe file format here.
 * 
 * The method prototypes and some documentation were copied from the
 * LOFAR storage manager written by Ger van Diepen.
 */
class OffringaStMan : public casa::DataManager
{
public:
	/**
	 * TODO add quantization count?
	 * TODO add dithering method
	 */
	explicit OffringaStMan(double globalStddev, const casa::String& name = "OffringaStMan");

	OffringaStMan(const casa::String& name, const casa::Record& spec);

	OffringaStMan(const OffringaStMan& source);
	 
	~OffringaStMan();

	// Clone this object.
	virtual casa::DataManager* clone() const { return new OffringaStMan(*this); }

	// Get the type name of the data manager (i.e. OffringaStMan).
	virtual casa::String dataManagerType() const { return "OffringaStMan"; }

	virtual casa::String dataManagerName() const { return _name; }

	// Record a record containing data manager specifications.
	virtual casa::Record dataManagerSpec() const;

	// Get the number of rows in this storage manager.
	uint getNRow() const { return _nRow; }

	virtual casa::Bool canAddRow() const { return true; }

	virtual casa::Bool canRemoveRow() const { return true; }

	virtual casa::Bool canAddColumn() const { return true; }

	virtual casa::Bool canRemoveColumn() const { return true; }

	// Get data manager properties that can be modified.
	// It is a subset of the data manager specification.
	// The default implementation returns an empty record.
	virtual casa::Record getProperties() const;

	// Modify data manager properties given in record fields. Only the
	// properties as returned by getProperties are used, others are ignored.
	// The default implementation does nothing.
	virtual void setProperties(const casa::Record& spec);
	
	// Create an object as specified by the type name string.
	// This methods gets registered in the DataManager "constructor" map.
	// The caller has to delete the object.
	static casa::DataManager* makeObject(const casa::String& aDataManType,
																				const casa::Record& spec)
	{
		return new OffringaStMan(aDataManType, spec);
	}

	// Register the class name and the static makeObject factory method.
	// This will make the engine known to the table system.
	static void registerClass();
	
	void ReadCompressedData(size_t rowIndex, const OffringaStManColumn *column, unsigned char *dest);
	
	void WriteCompressedData(size_t rowIndex, const OffringaStManColumn *column, const unsigned char *data);

	uint64_t NRowInFile() const { return _nRowInFile; }
	
	double GlobalStddev() const { return _globalStddev; }
	
	void RecalculateStride();
	
	const static unsigned short VERSION_MAJOR, VERSION_MINOR;
	
private:
	struct Header
	{
		/** Size of the total header, including column subheaders */
		unsigned headerSize;
		/** Start offset of the column headers */
		unsigned columnHeaderOffset;
		/** Number of columns and column headers */
		unsigned columnCount;
		/** File version number */
		unsigned short versionMajor, versionMinor;
		/** Global stddev, to be used if no more info available */
		double globalStddev;
		// the column headers start here
	};
	
	OffringaStMan &operator=(const OffringaStMan& source);
	
	// Flush and optionally fsync the data.
	// The AipsIO stream represents the main table file and can be
	// used by virtual column engines to store SMALL amounts of data.
  virtual casa::Bool flush(casa::AipsIO&, casa::Bool doFsync);
  
  // Let the storage manager create files as needed for a new table.
  // This allows a column with an indirect array to create its file.
  virtual void create(casa::uInt nRow);
	
  // Open the storage manager file for an existing table.
  // Return the number of rows in the data file.
  virtual void open(casa::uInt nRow, casa::AipsIO&);
	
	// Create a column in the storage manager on behalf of a table column.
	// The caller will NOT delete the newly created object.
	// Create a scalar column.
	virtual casa::DataManagerColumn* makeScalarColumn(const casa::String& name,
		int dataType, const casa::String& dataTypeID);

	// Create a direct array column.
	virtual casa::DataManagerColumn* makeDirArrColumn(const casa::String& name,
		int dataType, const casa::String& dataTypeID);

	// Create an indirect array column.
	virtual casa::DataManagerColumn* makeIndArrColumn(const casa::String& name,
		int dataType, const casa::String& dataTypeID);

	virtual void resync(casa::uInt nRow);

	virtual void deleteManager();

	// Prepare the columns, let the data manager initialize itself further.
	// Prepare is called after create/open has been called for all
	// columns. In this way one can be sure that referenced columns
	// are read back and partly initialized.
	virtual void prepare();
	
	// Reopen the storage manager files for read/write.
	virtual void reopenRW();
	
	// Add rows to the storage manager.
	virtual void addRow(casa::uInt nrrow);

	// Delete a row from all columns.
	virtual void removeRow(casa::uInt rowNr);

	// Do the final addition of a column.
	virtual void addColumn(casa::DataManagerColumn*);

	// Remove a column from the data file.
	virtual void removeColumn(casa::DataManagerColumn*);
	
	void makeEmpty();
	
	uint64_t _nRow, _nRowInFile;
	unsigned _rowStride, _headerSize;
	double _globalStddev;
	ZMutex _mutex;
	
	std::string _name;
	std::auto_ptr<std::fstream> _fStream;
	std::vector<OffringaStManColumn*> _columns;
	casa::Record _spec;
};

#endif
