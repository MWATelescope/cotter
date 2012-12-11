#ifndef METAFITSFILE_H
#define METAFITSFILE_H

#include "fitsuser.h"

#include <fitsio.h>

#include <vector>

class MWAHeader;
class MWAHeaderExt;

class MetaFitsFile : private FitsUser
{
public:
	MetaFitsFile(const char *filename);
	
	void ReadHeader(MWAHeader &header, MWAHeaderExt &headerExt);
	void ReadTiles(std::vector<class MWAInput> &inputs, std::vector<class MWAAntenna> &antennae);
private:
	void parseKeyword(MWAHeader &header, MWAHeaderExt &headerExt, const char *keyName, const char *keyValue);
	std::string parseFitsString(const char *valueStr);
	void parseFitsDate(const char *valueStr, int &year, int &month, int &day, int &hour, int &min, double &sec);
	double parseFitsDateToMJD(const char *valueStr);
	void parseIntArray(const char* valueStr, int *delays, size_t count);
	bool parseBool(const char *valueStr);
	
	fitsfile *_fptr;
};

#endif
