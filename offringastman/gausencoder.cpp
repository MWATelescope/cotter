#include "gausencoder.h"

#include <gsl/gsl_sf_erf.h>

#include <limits>
#include <cmath>

namespace offringastman
{

template<typename ValueType>
inline typename GausEncoder<ValueType>::num_t GausEncoder<ValueType>::cumulative(num_t x)
{
	return num_t(0.5) + num_t(0.5) * gsl_sf_erf(x/num_t(M_SQRT2l));
}

template<typename ValueType>
typename GausEncoder<ValueType>::num_t GausEncoder<ValueType>::invCumulative(num_t c, num_t err)
{
	if(c < 0.5) return(-invCumulative(1.0 - c, err));
	else if(c == 0.5) return 0.0;
	else if(c == 1.0) return std::numeric_limits<num_t>::infinity();
	else if(c > 1.0) return std::numeric_limits<num_t>::quiet_NaN();
	
	num_t x = 1.0;
	num_t fx = cumulative(x);
	num_t xLow, xHi;
	if(fx < c)
	{
		do {
			x *= 2.0;
			fx = cumulative(x);
		} while(fx < c);
		xLow = x * 0.5;
		xHi = x;
	} else {
		xLow = 0.0;
		xHi = 1.0;
	}
	num_t error = xHi;
	int notConverging = 0;
	do {
		x = (xLow + xHi) * 0.5;
		fx = cumulative(x);
		if(fx > c)
			xHi = x;
		else
			xLow = x;
		num_t currErr = std::fabs(fx - c);
		if(currErr >= error)
		{
			++notConverging;
			// not converging anymore; stop.
			if(notConverging > 10)
				return x;
		} else
			notConverging = 0;
		error = currErr;
	} while(error > err);
	return x;
}

/*
 * Old functon; initialize without explicit borders
 */
/*
template<typename ValueType>
GausEncoder<ValueType>::GausEncoder(size_t quantCount, ValueType stddev, bool gaussianMapping) :
	_dictionary(quantCount)
{
	typename Dictionary::iterator item = _dictionary.begin();
	if(gaussianMapping)
	{
		num_t previousVal = 0.0;
		for(size_t i=0; i!=quantCount; ++i)
		{
			num_t val = ((num_t) i + num_t(0.5)) / (num_t) (quantCount);
			*item = stddev * invCumulative(val);
			//item.roundingLimit = (val + previousVal) / 2.0;
			//item.symbol = i;
			previousVal = val;
			++item;
		}
	} else {
		for(size_t i=0; i!=quantCount; ++i)
		{
			num_t val = -1.0 + 2.0*((num_t) i + num_t(0.5)) / (num_t) (quantCount);
			*item = stddev * val;
			// item.symbol = i;
			++item;
		}
	}
}*/

template<typename ValueType>
GausEncoder<ValueType>::GausEncoder(size_t quantCount, ValueType stddev, bool gaussianMapping) :
	_encDictionary(quantCount-1), _decDictionary(quantCount)
{
	typename Dictionary::iterator encItem = _encDictionary.begin();
	typename Dictionary::iterator decItem = _decDictionary.begin();
	if(gaussianMapping)
	{
		num_t previousVal = 0.0;
		for(size_t i=0; i!=quantCount; ++i)
		{
			if(i != 0)
			{
				num_t rightBoundary = ((num_t) i) / (num_t) (quantCount);
				*encItem = stddev * invCumulative(rightBoundary);
				++encItem;
			}
			
			num_t val = ((num_t) i + num_t(0.5)) / (num_t) (quantCount);
			*decItem = stddev * invCumulative(val);
			previousVal = val;
			++decItem;
		}
	} else {
		for(size_t i=0; i!=quantCount; ++i)
		{
			if(i != 0)
			{
				num_t rightBoundary = -1.0 + 2.0*((num_t) i) / (num_t) (quantCount);
				*encItem = stddev * rightBoundary;
				++encItem;
			}
			
			num_t val = -1.0 + 2.0*((num_t) i + num_t(0.5)) / (num_t) (quantCount);
			*decItem = stddev * val;
			// item.symbol = i;
			++decItem;
		}
	}
}

template class GausEncoder<float>;

} // end of namespace
