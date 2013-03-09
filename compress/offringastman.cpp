#include "offringastman.h"
#include "offringastmancol.h"
#include "offringacomplexcol.h"
#include "offringaweightcol.h"

const unsigned short
	OffringaStMan::VERSION_MAJOR = 1,
	OffringaStMan::VERSION_MINOR = 0;

OffringaStMan::OffringaStMan(double globalStddev, const casa::String& name) :
	DataManager(),
	_nRow(0),
	_globalStddev(globalStddev),
	_name(name)
{
	_spec.define("global_stddev", _globalStddev);
}

OffringaStMan::OffringaStMan(const casa::String& name, const casa::Record& spec) :
	DataManager(),
	_nRow(0),
	_globalStddev(0.0),
	_name(name),
	_spec(spec)
{
	int i = spec.description().fieldNumber("global_stddev");
	if(i >= 0)
		_globalStddev = spec.asDouble("global_stddev");
}

OffringaStMan::OffringaStMan(const OffringaStMan& source) :
	DataManager(),
	_nRow(0),
	_globalStddev(source._globalStddev),
	_name(source._name),
	_spec(source._spec)
{
	_spec.define("global_stddev", _globalStddev);
}

OffringaStMan& OffringaStMan::operator=(const OffringaStMan& source)
{
	throw casa::DataManError("operator=() was called on OffringaStMan. OffringaStMan should never be assigned to.");
}

void OffringaStMan::makeEmpty()
{
	for(std::vector<OffringaStManColumn*>::iterator i = _columns.begin(); i!=_columns.end(); ++i)
		delete *i;
}

OffringaStMan::~OffringaStMan()
{
	makeEmpty();
	std::cout << "~OffringaStMan()\n";
}

casa::Record OffringaStMan::dataManagerSpec() const 
{
	std::cout << "dataManagerSpec()\n";
	return _spec;
}

casa::Record OffringaStMan::getProperties() const
{
	std::cout << "getProperties()\n";
	return _spec;
}

void OffringaStMan::setProperties(const casa::Record& spec)
{
	std::cout << "setProperties()\n";
}

void OffringaStMan::registerClass()
{
	DataManager::registerCtor("OffringaStMan", makeObject);
}

casa::Bool OffringaStMan::flush(casa::AipsIO& , casa::Bool doFsync)
{
	std::cout << "flush()\n";
	return false;
}

void OffringaStMan::create(casa::uInt nRow)
{
	ZMutex::scoped_lock lock(_mutex);
	std::cout << "create(" << nRow << ")\n";
	_nRow = nRow;
	_fStream.reset(new std::fstream(fileName().c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::trunc));
	if(_fStream->fail())
		throw casa::DataManError("I/O error: could not create new file '" + fileName() + "'");
	_fStream->clear();
	_fStream->seekp(0, std::ios_base::beg);
	if(_fStream->fail())
		throw casa::DataManError("I/O error: could not seek to beginning of file before write task");
	Header header;
	_headerSize = sizeof(header);
	header.headerSize = sizeof(header);
	header.columnHeaderOffset = sizeof(header);
	header.columnCount = 0;
	header.versionMajor = VERSION_MAJOR;
	header.versionMinor = VERSION_MINOR;
	header.globalStddev = _globalStddev;
	_fStream->write(reinterpret_cast<char*>(&header), sizeof(header));
	if(_fStream->fail())
		throw casa::DataManError("I/O error: could not write to file");
	_nRowInFile = 0;
}

void OffringaStMan::open(casa::uInt nRow, casa::AipsIO& )
{
	ZMutex::scoped_lock lock(_mutex);
	std::cout << "open(" << nRow << ")\n";
	_nRow = nRow;
	_fStream.reset(new std::fstream(fileName().c_str()));
	if(_fStream->fail())
		throw casa::DataManError("I/O error: could not open file '" + fileName() + "', which should be an existing file");
	Header header;
	_fStream->read(reinterpret_cast<char*>(&header), sizeof(header));
	_headerSize = header.headerSize;
	_globalStddev = header.globalStddev;
	_fStream->seekg(0, std::ios_base::end);
	std::streampos size = _fStream->tellg();
	
	RecalculateStride();
	
	if(_rowStride==0)
	{
		if(size > 0)
		{
			std::ostringstream s;
			s << "File is non-empty, but casa thinks it is empty (size=" << size << ") : corrupted?";
			throw casa::DataManError(s.str());
		}
		_nRowInFile = 0;
	} else {
		if(size > _headerSize)
			_nRowInFile = ((size_t) size - _headerSize) / _rowStride;
		else
			_nRowInFile = 0;
	}
}

casa::DataManagerColumn* OffringaStMan::makeScalarColumn(const casa::String& name, int dataType, const casa::String& dataTypeID)
{
	std::ostringstream s;
	s << "Cant create scalar columns with OffringaStMan! (requested datatype: '" << dataTypeID << "' (" << dataType << ")";
	throw casa::DataManError(s.str());
}

casa::DataManagerColumn* OffringaStMan::makeDirArrColumn(const casa::String& name, int dataType, const casa::String& dataTypeID)
{
	// TODO
	std::cout << "makeDirArrColumn(" << name << "," << dataType << ',' << dataTypeID << ")\n";
	
	OffringaStManColumn *col = 0;
	switch(dataType)
	{
		case casa::TpFloat:
			col = new OffringaWeightColumn(this, dataType);
			break;
		case casa::TpComplex:
			col = new OffringaComplexColumn(this, dataType);
			break;
		default: {
			std::ostringstream s;
			s << "Can't create datatype '" << dataTypeID << "' (" << dataType << ") with OffringaStMan storage manager";
			throw casa::DataManError(s.str());
		}
	}
	_columns.push_back(col);
	return col;
}

casa::DataManagerColumn* OffringaStMan::makeIndArrColumn(const casa::String& name, int dataType, const casa::String& dataTypeID)
{
	throw casa::DataManError("makeIndArrColumn() called on OffringaStMan. OffringaStMan can only created direct columns!\nUse casa::ColumnDesc::Direct as option in your column desc constructor");
}

void OffringaStMan::resync(casa::uInt nRow)
{
	std::cout << "resync()\n";
}

void OffringaStMan::deleteManager()
{
	std::cout << "deleteManager()\n";
	// TODO
}

void OffringaStMan::prepare()
{
	ZMutex::scoped_lock lock(_mutex);
	std::cout << "prepare()\n";
	
	// At this point, the stddev *has* to be set, otherwise
	// encoding will fail.
	if(_globalStddev == 0.0)
		throw casa::DataManError("The global stddev parameter of the OffringaStMan was not set!\nOffringaStMan was not correctly initialized by your tool.");
	
	for(std::vector<OffringaStManColumn*>::iterator col=_columns.begin(); col!=_columns.end(); ++col)
		(*col)->Prepare();
}

void OffringaStMan::reopenRW()
{
}

void OffringaStMan::addRow(casa::uInt nrrow)
{
	std::cout << "addRow()\n";
}

void OffringaStMan::removeRow(casa::uInt rowNr)
{
	std::cout << "removeRow()\n";
}

void OffringaStMan::addColumn(casa::DataManagerColumn* column)
{
	std::cout << "addColumn(" << column->columnName() << ")\n";
	OffringaComplexColumn *offCol = static_cast<OffringaComplexColumn*>(column);
	if(_nRowInFile != 0)
		throw casa::DataManError("Can't add columns while data has been committed to table");
	RecalculateStride();
	offCol->Prepare();
}

void OffringaStMan::removeColumn(casa::DataManagerColumn* column)
{
	std::cout << "removeColumn(" << column->columnName() << ")\n";
	if(_nRowInFile != 0)
		throw casa::DataManError("Can't remove columns while data has been committed to table");
}

void OffringaStMan::ReadCompressedData(size_t rowIndex, const OffringaStManColumn *column, unsigned char *dest)
{
	ZMutex::scoped_lock lock(_mutex);
	if(_nRowInFile <= rowIndex)
		throw casa::DataManError("Reading past end of file");
	_fStream->seekg(rowIndex * _rowStride + column->RowOffset() + _headerSize, std::ios_base::beg);
	if(_fStream->fail())
		throw casa::DataManError("I/O error: could not seek in file before reading, in file '" + fileName() + "'");
	_fStream->read(reinterpret_cast<char*>(dest), column->Stride());
	if(_fStream->fail())
		throw casa::DataManError("I/O error: error while reading file '" + fileName() + "'");
}

void OffringaStMan::WriteCompressedData(size_t rowIndex, const OffringaStManColumn *column, const unsigned char *data)
{
	ZMutex::scoped_lock lock(_mutex);
	if(_nRowInFile <= rowIndex)
		_nRowInFile = rowIndex + 1;
	_fStream->seekp(rowIndex * _rowStride + column->RowOffset() + _headerSize, std::ios_base::beg);
	if(_fStream->fail())
		throw casa::DataManError("I/O error: could not seek in file before writing, in file '" + fileName() + "'");
	_fStream->write(reinterpret_cast<const char*>(data), column->Stride());
	if(_fStream->fail())
		throw casa::DataManError("I/O error: error while writing file '" + fileName() + "'");
}

void OffringaStMan::RecalculateStride()
{
	_rowStride = 0;
	for(std::vector<OffringaStManColumn*>::iterator col=_columns.begin(); col!=_columns.end(); ++col)
	{
		(*col)->SetRowOffset(_rowStride);
		_rowStride += (*col)->Stride();
	}
}
