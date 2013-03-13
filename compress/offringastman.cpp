#include "offringastman.h"
#include "offringastmancol.h"
#include "offringastmanerror.h"
#include "offringacomplexcol.h"
#include "offringaweightcol.h"

using namespace altthread;

void register_offringastman()
{
	offringastman::OffringaStMan::registerClass();
}

namespace offringastman
{

const unsigned short
	OffringaStMan::VERSION_MAJOR = 1,
	OffringaStMan::VERSION_MINOR = 0;

OffringaStMan::OffringaStMan(const class RMSTable &rmsTable, const casa::String& name) :
	DataManager(),
	_nRow(0),
	_rmsTable(rmsTable),
	_name(name)
{
	// TODO _spec.define("global_stddev", _globalStddev);
}

OffringaStMan::OffringaStMan(const casa::String& name, const casa::Record& spec) :
	DataManager(),
	_nRow(0),
	_rmsTable(),
	_name(name),
	_spec(spec)
{
	setRMSTableFromSpec();
}

OffringaStMan::OffringaStMan(const OffringaStMan& source) :
	DataManager(),
	_nRow(0),
	_rmsTable(source._rmsTable),
	_name(source._name),
	_spec(source._spec)
{
	initializeSpec();
}

OffringaStMan& OffringaStMan::operator=(const OffringaStMan& source)
{
	throw OffringaStManError("operator=() was called on OffringaStMan. OffringaStMan should never be assigned to.");
}

void OffringaStMan::setRMSTableFromSpec()
{
	std::cout << "setRMSTableFromSpec()\n";
	int i = _spec.description().fieldNumber("RMSTable");
	if(i >= 0)
	{
		casa::Array<double> rmsTableArray = _spec.asArrayDouble("RMSTable");
		casa::IPosition shape = rmsTableArray.shape();
		size_t fieldCount = shape[2];
		size_t antennaCount = shape[1];
		_rmsTable = RMSTable(antennaCount, fieldCount);
		casa::Vector<double>::const_contiter rmsIter = rmsTableArray.cbegin();
		for(unsigned f=0; f!=fieldCount; ++f)
		{
			for(unsigned a1=0; a1!=antennaCount; ++a1)
			{
				for(unsigned a2=0; a2!=antennaCount; ++a2)
				{
					_rmsTable.Value(a1, a2, f) = *rmsIter;
					++rmsIter;
				}
			}
		}
	}
}

void OffringaStMan::initializeSpec()
{
	std::cout << "initializeSpec()\n";
	casa::IPosition shape(3);
	shape[0] = _rmsTable.AntennaCount();
	shape[1] = _rmsTable.AntennaCount();
	shape[2] = _rmsTable.FieldCount();
	casa::Array<double> rmsTableVector(shape);
	casa::Array<double>::contiter rmsIter = rmsTableVector.cbegin();
	for(unsigned f=0; f!=_rmsTable.FieldCount(); ++f)
	{
		for(unsigned a1=0; a1!=_rmsTable.AntennaCount(); ++a1)
		{
			for(unsigned a2=0; a2!=_rmsTable.AntennaCount(); ++a2)
			{
				*rmsIter = _rmsTable.Value(a1, a2, f);
				++rmsIter;
			}
		}
	}
	_spec.define("RMSTable", rmsTableVector);
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
	std::cout << "create(" << nRow << ")\n";
	_nRow = nRow;
	_fStream.reset(new std::fstream(fileName().c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::trunc));
	if(_fStream->fail())
		throw OffringaStManError("I/O error: could not create new file '" + fileName() + "'");
	_nRowInFile = 0;
	writeHeader();
}

void OffringaStMan::writeHeader()
{
	_fStream->seekp(0, std::ios_base::beg);
	if(_fStream->fail())
		throw OffringaStManError("I/O error: could not seek to beginning of file before write task");
	
	Header header;
	_headerSize = sizeof(header) + _rmsTable.SizeInBytes();
	header.headerSize = _headerSize;
	header.columnHeaderOffset = _headerSize;
	header.columnCount = 0;
	header.versionMajor = VERSION_MAJOR;
	header.versionMinor = VERSION_MINOR;
	header.rmsTableAntennaCount = _rmsTable.AntennaCount();
	header.rmsTableFieldCount = _rmsTable.FieldCount();
	_fStream->write(reinterpret_cast<char*>(&header), sizeof(header));
	if(_fStream->fail())
		throw OffringaStManError("I/O error: could not write to file");
	_rmsTable.Write(*_fStream);
}

void OffringaStMan::open(casa::uInt nRow, casa::AipsIO& )
{
	std::cout << "open(" << nRow << ")\n";
	_nRow = nRow;
	_fStream.reset(new std::fstream(fileName().c_str()));
	if(_fStream->fail())
		throw OffringaStManError("I/O error: could not open file '" + fileName() + "', which should be an existing file");
	Header header;
	_fStream->read(reinterpret_cast<char*>(&header), sizeof(header));
	_headerSize = header.headerSize;
	_rmsTable = RMSTable(header.rmsTableAntennaCount, header.rmsTableFieldCount);
	_rmsTable.Read(*_fStream);
	_fStream->seekg(0, std::ios_base::end);
	std::streampos size = _fStream->tellg();
	
	recalculateStride();
	
	if(_rowStride==0)
	{
		if(size > 0)
		{
			std::ostringstream s;
			s << "File is non-empty, but casa thinks it is empty (size=" << size << ") : corrupted?";
			throw OffringaStManError(s.str());
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
	throw OffringaStManError(s.str());
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
			throw OffringaStManError(s.str());
		}
	}
	_columns.push_back(col);
	return col;
}

casa::DataManagerColumn* OffringaStMan::makeIndArrColumn(const casa::String& name, int dataType, const casa::String& dataTypeID)
{
	throw OffringaStManError("makeIndArrColumn() called on OffringaStMan. OffringaStMan can only created direct columns!\nUse casa::ColumnDesc::Direct as option in your column desc constructor");
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
	mutex::scoped_lock lock(_mutex);
	std::cout << "prepare()\n";
	
	// At this point, the rms-es *have* to be set, otherwise
	// encoding will fail.
	if(_rmsTable.Empty())
		throw OffringaStManError("The RMS table of the OffringaStMan was not set!\nOffringaStMan was not correctly initialized by your tool.");
	
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
		throw OffringaStManError("Can't add columns while data has been committed to table");
	recalculateStride();
	offCol->Prepare();
}

void OffringaStMan::removeColumn(casa::DataManagerColumn* column)
{
	std::cout << "removeColumn(" << column->columnName() << ")\n";
	if(_nRowInFile != 0)
		throw OffringaStManError("Can't remove columns while data has been committed to table");
}

void OffringaStMan::readCompressedData(size_t rowIndex, const OffringaStManColumn *column, unsigned char *dest)
{
	mutex::scoped_lock lock(_mutex);
	if(_nRowInFile <= rowIndex)
		throw OffringaStManError("Reading past end of file");
	_fStream->seekg(rowIndex * _rowStride + column->RowOffset() + _headerSize, std::ios_base::beg);
	if(_fStream->fail())
		throw OffringaStManError("I/O error: could not seek in file before reading, in file '" + fileName() + "'");
	_fStream->read(reinterpret_cast<char*>(dest), column->Stride());
	if(_fStream->fail())
		throw OffringaStManError("I/O error: error while reading file '" + fileName() + "'");
}

void OffringaStMan::writeCompressedData(size_t rowIndex, const OffringaStManColumn *column, const unsigned char *data)
{
	mutex::scoped_lock lock(_mutex);
	if(_nRowInFile <= rowIndex)
		_nRowInFile = rowIndex + 1;
	_fStream->seekp(rowIndex * _rowStride + column->RowOffset() + _headerSize, std::ios_base::beg);
	if(_fStream->fail())
		throw OffringaStManError("I/O error: could not seek in file before writing, in file '" + fileName() + "'");
	_fStream->write(reinterpret_cast<const char*>(data), column->Stride());
	if(_fStream->fail())
		throw OffringaStManError("I/O error: error while writing file '" + fileName() + "'");
}

void OffringaStMan::recalculateStride()
{
	_rowStride = 0;
	for(std::vector<OffringaStManColumn*>::iterator col=_columns.begin(); col!=_columns.end(); ++col)
	{
		(*col)->SetRowOffset(_rowStride);
		_rowStride += (*col)->Stride();
	}
}

} // end of namespace
