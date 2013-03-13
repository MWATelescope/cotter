#ifndef OFFRINGA_STORAGE_MANAGER_H
#define OFFRINGA_STORAGE_MANAGER_H

#include <tables/Tables/DataManager.h>

#include <casa/Containers/Record.h>

#include <fstream>
#include <vector>

#include <auto_ptr.h>
#include <stdint.h>

#include "rmstable.h"
#include "thread.h"

/**
* @file
* Contains OffringaStMan and its global register function
* register_offringastman().
* 
* @defgroup Globals Global functions
* Contains the register_offringastman() function.
*/

#ifndef DOXYGEN_SHOULD_SKIP_THIS
extern "C" {
#endif
	/**
	* This function makes the OffringaStMan known to casacore. The function
	* is necessary for loading the storage manager from a shared library.
	* When Casa encounters a set with the offringastman, it will look
	* for a library 'liboffringastman', which should contain a global function
	* that has this specific name ("register_" + storage manager's name in
	* lowercase) to be able to be automatically called when the library is
	* loaded.
	* @ingroup offringastman
	* @see OffringaStMan::registerClass()
	*/
	void register_offringastman();
#ifndef DOXYGEN_SHOULD_SKIP_THIS
}
#endif

/**
 * "Offringa storage manager" for compressing CASA Measurement sets.
 * Contains the main OffringaStMan class and several required subclasses.
 * 
 * @author André Offringa
 */
namespace offringastman {

class OffringaStManColumn;

/**
* A storage manager for the compression technique to be described in
* Offringa et al (2013).
* 
* TODO describe file format here.
* 
* The procedure used by Casa when initializing a new OffringaStMan:
* 1. A OffringaStMan is constructed by a client program and passed to Casacore.
* 2. makeDirArrColumn() is called
* 3. OffringaStManColumn::setShapeColumn() is called on each column
* 4. create() is called.
* 5. prepare() is called.
* 6. Data can be read/written using the column.
* 
* Procedure when initializing the manager for set with an existing
* OffringaStMan:
* 1. (optionally) when the stman is not loaded yet, Casa will load the library and
*    call register_offringastman().
* 2. A OffringaStMan is constructed by Casacore
* 3. makeDirArrColumn() is called for all existing OffringaStMan columns
* 4. OffringaStManColumn::setShapeColumn() is called
* 5. when all existing columns are added, open() is called
* 6. prepare() is called.
* 7. Data can be read/written
* 
* Finally, if a new column is created on a set with already existing
* columns, the procedure is:
* 1. makeDirArrColumn() is called
* 2. OffringaStManColumn::setShapeColumn() is called
* 3. addColumn() is called
* 4. Data can be read/written
* Note that no prepare() is called, so addColumn should prepare everything
* is necessary for reading/writing.
* 
* Two different types can be stored with this manager:
* - Complex gaussian arrays, stored with the OffringaComplexCol class
* - Weight arrays, stored with the OffringaWeightCol class
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
	* TODO stddev should actually be RMS
	*/
	explicit OffringaStMan(const RMSTable &rmsTable, const casa::String& name = "OffringaStMan");

	/**
	* This constructor is called by Casa when it needs to create a LofarStMan.
	* Casa will call makeObject() that will call this constructor.
	* When it loads an OffringaStMan for an existing MS, the "spec" parameter
	* will be empty, thus the class should initialize its properties
	* by reading them from the file.
	* The @p spec is used to make a new storage manager with specs similar to
	* another one.
	* @param name Name of this storage manager.
	* @param spec Specs to initialize this class with.
	*/
	OffringaStMan(const casa::String& name, const casa::Record& spec);

	/**
	* Copy constructor that initializes a storage manager with similar specs.
	* The columns are not copied: the new manager will be empty.
	*/
	OffringaStMan(const OffringaStMan& source);
	
	/** Destructor. */
	~OffringaStMan();

	/** Polymorphical copy constructor, equal to OffringaStMan(const OffringaStMan&).
	* @returns Empty manager with specs as the source.
	*/
	virtual casa::DataManager* clone() const { return new OffringaStMan(*this); }

	/** Type of manager
	* @returns "OffringaStMan". */
	virtual casa::String dataManagerType() const { return "OffringaStMan"; }

	/** Returns the name of this manager as specified during construction. */
	virtual casa::String dataManagerName() const { return _name; }

	/** Get manager specifications. Includes method settings, etc. Can be used
	* to make a second storage manager with 
	* @returns Record containing data manager specifications.
	*/
	virtual casa::Record dataManagerSpec() const;

	/**
	* Get the number of rows in the measurement set.
	* This is not necessary equal to the actual number of rows stored in the
	* file. In case a new OffringaStMan is added to an existing measurement set
	* with some rows, no rows will yet be stored, while this function will
	* return the number of rows in the entire measurement set.
	* @returns Number of rows in the measurement set.
	*/
	uint getNRow() const { return _nRow; }

	/**
	* Whether rows can be added.
	* @returns @c true
	*/
	virtual casa::Bool canAddRow() const { return true; }

	/**
	* Whether rows can be removed.
	* @returns @c true (but only rows at the end can actually be removed)
	*/
	virtual casa::Bool canRemoveRow() const { return true; }

	/**
	* Whether columns can be added.
	* @returns @c true (but restrictions apply; columns can only be added as long
	* as no writes have been performed on the set).
	*/
	virtual casa::Bool canAddColumn() const { return true; }

	/**
	* Whether columns can be removed.
	* @return @c true (but restrictions apply -- still to be checked)
	* @todo Describe restrictons
	*/
	virtual casa::Bool canRemoveColumn() const { return true; }

	/**
	* Create an object with given name and spec.
	* This methods gets registered in the DataManager "constructor" map.
	* The caller has to delete the object. New class will be
	* initialized via @ref OffringaStMan(const casa::String& name, const casa::Record& spec).
	* @returns An OffringaStMan with given specs.
	*/
	static casa::DataManager* makeObject(const casa::String& name,
																				const casa::Record& spec)
	{
		return new OffringaStMan(name, spec);
	}

	/**
	* This function makes the OffringaStMan known to casacore. The function
	* is necessary for loading the storage manager from a shared library. It
	* should have this specific name ("register_" + storage manager's name in
	* lowercase) to be able to be automatically called when the library is
	* loaded. That function will forward the
	* call here.
	*/
	static void registerClass();
	
	/**
	 * Retrieve the RMS table. These values are used for encoding.
	 * @returns The RMS table that contains the RMS values per baseline.
	 */
	const RMSTable &GetRMSTable() const { return _rmsTable; }
	
protected:
	/**
	* The number of rows that are actually stored in the file.
	*/
	uint64_t nRowInFile() const { return _nRowInFile; }
	
	/**
	* Will recount the number of bytes that each column will use
	* to determine to total number of bytes that a row will take.
	* The stride of a row is the total number of bytes/per row.
	* This method should be called after each change to the structure,
	* i.e., by adding/changing columns.
	*/
	void recalculateStride();
	
private:
	friend class OffringaStManColumn;
	
	const static unsigned short VERSION_MAJOR, VERSION_MINOR;
	
#ifndef DOXYGEN_SHOULD_SKIP_THIS
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
		
		unsigned rmsTableAntennaCount, rmsTableFieldCount;
		
		// the RMS table is here
		
		// the column headers start here
	};
#endif
	
	void readCompressedData(size_t rowIndex, const OffringaStManColumn *column, unsigned char *dest);
	
	void writeCompressedData(size_t rowIndex, const OffringaStManColumn *column, const unsigned char *data);
	
	void writeHeader();

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
	
	void setRMSTableFromSpec();
	
	void initializeSpec();
	
	uint64_t _nRow, _nRowInFile;
	unsigned _rowStride, _headerSize;
	RMSTable _rmsTable;
	altthread::mutex _mutex;
	
	std::string _name;
	std::auto_ptr<std::fstream> _fStream;
	std::vector<OffringaStManColumn*> _columns;
	casa::Record _spec;
};

} // end of namespace

#endif
