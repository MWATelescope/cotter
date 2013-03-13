#ifndef DYNAMIC_GAUS_ENCODER_H
#define DYNAMIC_GAUS_ENCODER_H

#include "gausencoder.h"

namespace offringastman
{
	
template<typename ValueType=float>
class DynamicGausEncoder
{
	public:
		typedef typename GausEncoder<ValueType>::value_t value_t;
		typedef typename GausEncoder<ValueType>::symbol_t symbol_t;
		
		DynamicGausEncoder(size_t quantCount) :
			_encoder(quantCount, 1.0)
		{
		}
		
		void Encode(value_t rms, symbol_t *dest, const value_t *input, size_t symbolCount) const
		{
			value_t fact = 1.0 / rms;
			
			// TODO this is easily vectorizable :-)
			for(size_t i=0; i!=symbolCount; ++i)
			{
				value_t val = input[i] * fact;
				dest[i] = _encoder.Encode(val);
			}
		}
		
		void Decode(value_t rms, value_t *dest, const symbol_t *input, size_t symbolCount) const
		{
			// TODO this is easily vectorizable :-)
			for(size_t i=0; i!=symbolCount; ++i)
			{
				dest[i] = _encoder.Decode(input[i]) * rms;
			}
		}
		
	private:
		GausEncoder<ValueType> _encoder;
};

} // end of namespace

#endif
