#ifndef WEIGHT_ENCODER_H
#define WEIGHT_ENCODER_H

#include <vector>
#include <cmath>

template<typename T>
class WeightEncoder
{
	public:
		typedef T value_t;
		typedef unsigned symbol_t;
		
		WeightEncoder(unsigned quantCount) : _quantCount(quantCount)
		{
		}
		
		void Encode(value_t &scaleVal, std::vector<symbol_t> &dest, const std::vector<value_t> &input) const
		{
			// Find max value (implicit assumption input is not empty)
			typename std::vector<value_t>::const_iterator i=input.begin();
			scaleVal = *i;
			++i;
			while(i != input.end())
			{
				if(*i > scaleVal) scaleVal = *i;
				++i;
			}
			
			i=input.begin();
			typename std::vector<symbol_t>::iterator d=dest.begin();
			const value_t scaleFact = value_t(_quantCount-1) / scaleVal;
			while(i != input.end())
			{
				*d = roundf(scaleFact * (*i));
				
				++i;
				++d;
			}
		}
		
		void Decode(std::vector<value_t> &dest, value_t scaleVal, const std::vector<symbol_t> &input) const
		{
			typename std::vector<symbol_t>::const_iterator i=input.begin();
			typename std::vector<value_t>::iterator d=dest.begin();
			const value_t scaleFact = scaleVal / value_t(_quantCount-1);
			while(i != input.end())
			{
				*d = (*i) * scaleFact;
				
				++i;
				++d;
			}
		}
		
	private:
		unsigned _quantCount;
};

#endif
