#include "fitsuser.h"

#include <fitsio.h>

#include <stdexcept>
#include <sstream>

void FitsUser::checkStatus(int status)
{
	if(status != 0)
	{
		throwError(status);
	}
}

void FitsUser::throwError(int status, const std::string &msg)
{
	std::stringstream msgStr;
	
	if(!msg.empty()) msgStr << msg << ". ";
	
	char statusStr[FLEN_STATUS], errMsg[FLEN_ERRMSG];
	fits_get_errstatus(status, statusStr);
	msgStr << "CFitsio reported an error while accessing fits files. Status = " << status << ": " << statusStr;

	if(fits_read_errmsg(errMsg))
	{
		msgStr << ". Error message stack: " << errMsg << '.';

		while( fits_read_errmsg(errMsg) )
			msgStr << ' ' << errMsg << '.';
	}
	throw std::runtime_error(msgStr.str());
}

