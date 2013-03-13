#ifndef DYNAMIC_GAUS_ENCODER_H
#define DYNAMIC_GAUS_ENCODER_H

#include "gausencoder.h"

namespace offringastman
{
	
/**
 * Encodes gaussian values with different RMS values, that are grouped
 * into values with similar RMS (such as interferometric baselines).
 * Internally, it uses the GausEncoder for encoding the values. Refer to
 * the GausEncoder class description for more detailed information about the encoder.
 * @author André Offringa
 */
template<typename ValueType=float>
class DynamicGausEncoder
{
	public:
		/**
		 * Templated type used for representing floating point values that
		 * are to be encoded.
		 */
		typedef typename GausEncoder<ValueType>::value_t value_t;
		/**
		 * Unsigned integer type used for representing encoded symbols.
		 */
		typedef typename GausEncoder<ValueType>::symbol_t symbol_t;
		
		/**
		 * Construct a dynamic Gaussian encoder with given number of
		 * quantizations. This constructor initializes the GausEncoder,
		 * whose constructor computes the lookup table. This constructor
		 * is therefore quite slow.
		 * @param quantCount The number of quantization levels, i.e., the dictionary
		 * size.
		 */
		DynamicGausEncoder(size_t quantCount) :
			_encoder(quantCount, 1.0)
		{
		}
		
		/**
		 * Encode an array of values with equal RMS. Will normalize the values and
		 * call GausEncoder::Encode() on each value in the array.
		 * @param rms The estimated RMS of the values to be encoded.
		 * @param dest Destination buffer for encoded symbols of at least @p symbolCount size.
		 * @param input Input floating point values to be encoded of size @p symbolCount.
		 * @param symbolCount Number of samples in input array.
		 */
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
		
		/**
		 * Decode an array of values with equal RMS. Will call GausEncoder::Decode() on
		 * each value in the array and scale the values back to their original values.
		 * @param rms The RMS that was used during encoding of these values.
		 * @param dest Destination buffer for decoded symbols of at least @p symbolCount size.
		 * @param input Input symbols to be decoded, of size @p symbolCount.
		 * @param symbolCount Number of samples in input array.
		 */
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
