#ifndef GAUS_ENCODER_H
#define GAUS_ENCODER_H

#include <vector>
#include <cstring>
#include <algorithm>

namespace offringastman
{
	
/**
 * Lossy encoder for Gaussian values.
 * 
 * This encoder can encode a numeric value represented by a floating point
 * number (float, double, ...) into an integer value with a given limit.
 * It is the least-square error quantization for Gaussian distributed values,
 * and remains fairly good for near-Gaussian (e.g. uniform) values.
 * 
 * Encoding and decoding have asymetric time complexity / speeds, as decoding
 * is easier than encoding. Decoding is a single indexing into an array, thus
 * extremely fast and with constant time complexity. Encoding is a binary
 * search through the quantizaton values, thus takes O(log quantcount).
 * Typical performance of encoding is 100 MB/s.
 * 
 * If the values are encoded into a number of bits which are not divisible by
 * eight, the BytePacker class can be used to pack the values.
 * 
 * @author André Offringa (offringa@gmail.com)
 */
template<typename ValueType=float>
class GausEncoder
{
	public:
		/**
		 * Construct encoder for given dictionary size and Gaussian stddev.
		 * @param quantCount The number of quantization values, i.e., the dictionary
		 * size.
		 * @param stddev The standard deviation of the data. The closer this value is
		 * to the real stddev, the more accurate the encoder will be.
		 * @param gaussianMapping Used for testing with non-gaussian distributions.
		 */
		GausEncoder(size_t quantCount, ValueType stddev, bool gaussianMapping = true);
		
		/**
		 * Unsigned integer type used for representing encoded symbols.
		 */
		typedef unsigned symbol_t;
		
		/**
		 * Template type used for representing floating point values.
		 */
		typedef ValueType value_t;
		
		/**
		 * Get the quantized symbol for the given floating point value.
		 * This method is implemented with a binary search, so takes
		 * O(log N), with N the dictionary size (2^bitcount).
		 * Use Decode() on the returned symbol to get an estimate of
		 * the unquantized value.
		 * @param value Floating point value to be encoded.
		 */
		symbol_t Encode(ValueType value) const
		{
			return _encDictionary.symbol(_encDictionary.lower_bound(value));
		}
		
		/**
		 * Will return the right boundary of the given symbol.
		 * The right boundary is the smallest value that would not be
		 * quantized to the given symbol anymore. If no such boundary
		 * exists, 0.0 is returned.
		 */
		value_t RightBoundary(symbol_t symbol) const
		{
			if(symbol != _encDictionary.size())
				return _encDictionary.value(symbol);
			else
				return 0.0;
		}
		
		/*
		symbol_t Encode2(ValueType value) const
		{
			typename Dictionary::const_iterator iter =
				_dictionary.lower_bound(value);
			
			// if all keys are smaller, return largest symbol.
			if(iter == _dictionary.end())
				return _dictionary.largest_symbol();
			// if smaller than first item return first item
			else if(iter == _dictionary.begin())
				return _dictionary.symbol(iter);
			
			// requested value is between two symbols: round to nearest.
				
			typename Dictionary::const_iterator prev = iter-1;
			if(value - _dictionary.value(prev) < _dictionary.value(iter) - value)
				return _dictionary.symbol(prev);
			else
				return _dictionary.symbol(iter);
		}*/
		
		/**
		 * Get the centroid value that belongs to the given symbol.
		 * @param symbol Symbol to be decoded
		 * @returns The best estimate of the original value.
		 */
		ValueType Decode(symbol_t symbol) const
		{
			return _decDictionary.value(symbol);
		}
		
	private:
		class DictionarySlow
		{
			public:
				typedef typename std::vector<value_t>::iterator iterator;
				typedef typename std::vector<value_t>::const_iterator const_iterator;
				
				DictionarySlow(size_t size) : _values(size) { }
				const_iterator lower_bound(value_t val) const
				{
					return std::lower_bound(_values.begin(), _values.end(), val);
				}
				iterator begin() { return _values.begin(); } 
				const_iterator begin() const { return _values.begin(); } 
				const_iterator end() const { return _values.end(); }
				symbol_t symbol(const_iterator iter) const { return (iter - _values.begin()); }
				symbol_t largest_symbol() const { return _values.size()-1; }
				value_t value(const_iterator iter) const { return *iter; }
				value_t value(symbol_t sym) const { return _values[sym]; }
			private:
				std::vector<value_t> _values;
		};

		class Dictionary
		{
			public:
				typedef value_t* iterator;
				typedef const value_t* const_iterator;
				
				Dictionary(size_t size) : _values(size) { }
				
				/**
				 * This implementation turns out to be slightly faster than the
				 * STL implementation. It performs 10.7 MB/s, vs. 9.0 MB/s for the
				 * STL. 18% faster. Using "unsigned" instead of "size_t" is 5% slower.
				 * (It's not a fair STL comparison, because this implementation
				 * does not check for empty vector).
				 */
				const_iterator lower_bound(value_t val) const
				{
					size_t p = 0, q = _values.size();
					while(p+1 != q)
					{
						size_t m = (p + q) / 2;
						if(_values[m] <= val) p=m;
						else q=m;
					}
					return (_values[p] < val) ? (&_values[q]) : (&_values[p]);
				}
				/**
				* Below is the first failed result of an attempt to beat the STL in performance.
				* It turns out to be 13% slower for larger dictionaries,
				* compared to the STL implementation that is used in the class
				* above. It performs 7.9 MB/s. 26% compared to the 'fastest' lower_bound.
				*/
				const_iterator lower_bound_slow(value_t val) const
				{
					const value_t *p = &*_values.begin(), *q = p + _values.size();
					while(p+1 != q)
					{
						// This is a bit inefficient, but (p + q)/2 was not allowed, because
						// operator+(ptr,ptr) is not allowed.
						const value_t *m = p + (q - p)/2;
						if(*m <= val) p=m;
						else q=m;
					}
					return p;
				}
				/**
				 * By using this function instead of lower_bound, the performance of the
				 * Encode() function is boosted to 17.4 MB/s :).
				 * (unfortunately, we no longer round to nearest -- initially I did, but
				 * we now round using the centroid. So this function is no longer used.)
				 */
				const_iterator nearest(value_t val) const
				{
					if(val <= _values[0]) return &_values[0];
					size_t p = 0, q = _values.size()-1;
					if(val >= _values[q]) return &_values[q];
					while(p+1 != q)
					{
						size_t m = (p + q) / 2;
						if(_values[m] <= val) p=m;
						else q=m;
					}
					return (val - _values[p] < _values[q] - val) ? (&_values[p]) : (&_values[q]);
				}
				iterator begin() { return &*_values.begin(); } 
				const_iterator begin() const { return &*_values.begin(); } 
				const_iterator end() const { return &*_values.end(); }
				symbol_t symbol(const_iterator iter) const { return (iter - begin()); }
				symbol_t largest_symbol() const { return _values.size()-1; }
				value_t value(const_iterator iter) const { return *iter; }
				value_t value(symbol_t sym) const { return _values[sym]; }
				size_t size() const { return _values.size(); }
			private:
				std::vector<value_t> _values;
		};
		
		typedef long double num_t;
		
		static num_t cumulative(num_t x);
		static num_t invCumulative(num_t c, num_t err = num_t(1e-13));
		
		Dictionary _encDictionary;
		Dictionary _decDictionary;
};

} // end of namespace

#endif
