#include "mwafits.h"
#include "fitsuser.h"

#include <sstream>

#include <fitsio.h>

struct MWAFitsData : private FitsUser
{
	MWAFitsData(const std::string& filename)
	{
		int status = 0;
		fits_open_file(&_fitsPtr, filename.c_str(), READWRITE, &status);
		checkStatus(status);
	}
	
	~MWAFitsData()
	{
		int status = 0;
		fits_close_file(_fitsPtr, &status);
		checkStatus(status);
	}
	
	void writeKey(const char* name, const char* value, const char* comment)
	{
		int status = 0;
		fits_write_key(_fitsPtr, TSTRING, name, (void*) value, comment, &status);
		checkStatus(status);
	}
	
	void writeKey(const char* name, const std::string& value, const char* comment)
	{
		writeKey(name, value.c_str(), comment);
	}
	
	void writeKey(const char* name, double value, const char* comment)
	{
		std::ostringstream str;
		str << value;
		writeKey(name, str.str(), comment);
	}
	
private:
	fitsfile *_fitsPtr;
};

const char *MWAFits::_keywordNames[] = {
	"FIBRFACT", "METAVER",
	"MWAPYVER",
	"COTVER", "COTVDATE"
};

const char *MWAFits::_keywordComments[] = {
	"Fiber velocity factor", "METAFITS version number",
	"MWAPY version number",
	"Cotter version", "Cotter version date"
};

MWAFits::MWAFits(const std::string& filename) : _filename(filename),
	_data(new MWAFitsData(filename))
{
}

MWAFits::~MWAFits()
{
	delete _data;
}

template<typename T>
void MWAFits::writeKey(MWAFitsEnums::MWAKeywords keyword, const T& value)
{
	_data->writeKey(keywordName(keyword), value, keywordComment(keyword));
}

void MWAFits::WriteMWAKeywords(const std::string& metaDataVersion, const std::string& mwaPyVersion, const std::string& cotterVersion, const std::string& cotterVersionDate)
{
	//writeKey(MWAFitsEnums::MWA_FIBER_VEL_FACTOR, fibreVelFactor);
	writeKey(MWAFitsEnums::MWA_METADATA_VERSION, metaDataVersion);
	writeKey(MWAFitsEnums::MWA_MWAPY_VERSION, mwaPyVersion);
	writeKey(MWAFitsEnums::MWA_COTTER_VERSION, cotterVersion);
	writeKey(MWAFitsEnums::MWA_COTTER_VERSION_DATE, cotterVersionDate);
}
