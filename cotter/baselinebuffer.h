#ifndef BASELINE_BUFFER_H
#define BASELINE_BUFFER_H

#include <cstring>

class BaselineBuffer
{
	public:
		BaselineBuffer() :
			realXX(0), imagXX(0),
			realXY(0), imagXY(0),
			realYX(0), imagYX(0),
			realYY(0), imagYY(0),
			nElementsPerRow(0)
		{
		}
		
		float
			*realXX, *imagXX,
			*realXY, *imagXY,
			*realYX, *imagYX,
			*realYY, *imagYY;
		size_t nElementsPerRow;
};

#endif
