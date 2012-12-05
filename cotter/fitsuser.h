#ifndef FITSUSER_H
#define FITSUSER_H

#include <string>

class FitsUser
{
	protected:
		void checkStatus(int status);
		void throwError(int status, const std::string &msg = std::string());
};

#endif
