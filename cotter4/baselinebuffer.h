#ifndef BASELINE_BUFFER_H
#define BASELINE_BUFFER_H

#include <cstring>

class BaselineBuffer
{
	public:
		BaselineBuffer() :
			nElementsPerRow(0)
		{
			for(size_t p=0; p!=4; ++p)
			{
				real[p] = 0; imag[p] = 0;
			}
		}
		
		BaselineBuffer(const BaselineBuffer &source) :
			nElementsPerRow(source.nElementsPerRow)
		{
			for(size_t p=0; p!=4; ++p)
			{
				real[p] = source.real[p];
				imag[p] = source.imag[p];
			}
		}
		
		BaselineBuffer& operator=(const BaselineBuffer &source)
		{
			nElementsPerRow = source.nElementsPerRow;
			for(size_t p=0; p!=4; ++p)
			{
				real[p] = source.real[p];
				imag[p] = source.imag[p];
			}
			return *this;
		}
		
		float *real[4], *imag[4];
		size_t nElementsPerRow;
};

#endif
