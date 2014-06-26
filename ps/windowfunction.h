#ifndef WINDOW_FUNCTION_H
#define WINDOW_FUNCTION_H

#include <cmath>
#include <cstring>

class WindowFunction
{
public:
	static double blackmanNutallWindow(size_t n, size_t i)
	{
		const static double
			a0=0.3635819, a1=0.4891775, a2=0.1365995, a3=0.0106411;
		const double
			id = double(i) * 2.0 * M_PI, nd = n-1;
		return
			a0 -
			a1 * cos( (1.0 * id)/nd ) +
			a2 * cos( (2.0 * id)/nd ) -
			a3 * cos( (3.0 * id)/nd );
	}
};

#endif
