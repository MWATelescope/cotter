#ifndef FITSUSER_H
#define FITSUSER_H

#include <string>

class FitsUser
{
	protected:
		static void checkStatus(int status);
		static void throwError(int status, const std::string &msg = std::string());
};

#endif
