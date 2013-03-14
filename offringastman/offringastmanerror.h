#ifndef OFFRINGA_STMAN_ERROR_H
#define OFFRINGA_STMAN_ERROR_H

#include <tables/Tables/DataManError.h>

namespace offringastman
{

/**
 * Represents a runtime exception that occured within the OffringaStMan.
 */
class OffringaStManError : public casa::DataManError
{
public:
	/** Constructor */
	OffringaStManError() : casa::DataManError()
	{	}
	/** Construct with message.
	 * @param message The exception message. */
	OffringaStManError(const std::string& message) : casa::DataManError(message + " -- Error occured inside the OffringaStMan")
	{ }
};

} // end of namespace

#endif
