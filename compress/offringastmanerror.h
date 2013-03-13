#ifndef OFFRINGA_STMAN_ERROR_H
#define OFFRINGA_STMAN_ERROR_H

#include <tables/Tables/DataManError.h>

namespace offringastman
{
	
class OffringaStManError : public casa::DataManError
{
public:
	OffringaStManError() : casa::DataManError()
	{	}
	OffringaStManError(const std::string& message) : casa::DataManError(message + " -- Error occured inside the OffringaStMan")
	{ }
};

} // end of namespace

#endif
