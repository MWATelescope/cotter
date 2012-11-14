#include "averagingmswriter.h"

#include <xmmintrin.h>

#define USE_SSE

void AveragingMSWriter::WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, size_t scanNumber, const std::complex<float>* data, const bool* flags, const float *weights)
{
	Buffer &buffer = getBuffer(antenna1, antenna2);
	size_t srcIndex = 0;
	for(size_t ch=0; ch!=_avgChannelCount*_freqAvgFactor; ++ch)
	{
#ifndef USE_SSE
		for(size_t p=0; p!=4; ++p)
		{
			const size_t destIndex = (ch / _freqAvgFactor) * 4 + p;
			buffer._flaggedAndUnflaggedData[destIndex] += data[srcIndex];
			if(!flags[srcIndex])
			{
				buffer._rowData[destIndex] += data[srcIndex] * weights[srcIndex];
				buffer._rowWeights[destIndex] += weights[srcIndex];
				buffer._rowCounts[destIndex]++;
			}
			++srcIndex;
		}
#else
		const size_t destIndex = (ch / _freqAvgFactor) * 4;
		const __m128 dataValA = _mm_load_ps((float*) &data[srcIndex]);
		const __m128 dataValB = _mm_load_ps((float*) &data[srcIndex+2]);
		std::complex<float> *allDataPtr = &buffer._flaggedAndUnflaggedData[destIndex];
		
		// Perform *allDataPtr += dataVal
		_mm_store_ps((float*) allDataPtr, _mm_add_ps(_mm_load_ps((float*) allDataPtr), dataValA));
		_mm_store_ps((float*) (allDataPtr+2), _mm_add_ps(_mm_load_ps((float*) (allDataPtr+2)), dataValB));
		
		// Note that if one polarization is flagged, all are flagged
		if(!flags[srcIndex])
		{
			const __m128 weightsA = _mm_set_ps(weights[srcIndex+1], weights[srcIndex+1], weights[srcIndex], weights[srcIndex]);
			const __m128 weightsB = _mm_set_ps(weights[srcIndex+3], weights[srcIndex+3], weights[srcIndex+2], weights[srcIndex+2]);
			std::complex<float> *destPtr = &buffer._rowData[destIndex];
			
			// Perform *destPtr += dataVal * weights
			_mm_store_ps((float*) destPtr,     _mm_add_ps(_mm_load_ps((float*) destPtr), _mm_mul_ps(dataValA, weightsA)));
			_mm_store_ps((float*) (destPtr+2), _mm_add_ps(_mm_load_ps((float*) (destPtr+2)), _mm_mul_ps(dataValB, weightsB)));
			
			// Perform buf.weight += weights
			_mm_store_ps(&buffer._rowWeights[destIndex], _mm_add_ps(_mm_load_ps(&weights[srcIndex]), _mm_load_ps(&buffer._rowWeights[destIndex])));
			
			buffer._rowCounts[destIndex]++;
			buffer._rowCounts[destIndex+1]++;
			buffer._rowCounts[destIndex+2]++;
			buffer._rowCounts[destIndex+3]++;
		}
		
		srcIndex += 4;
#endif
	}
	buffer._rowTime += time;
	buffer._rowTimestepCount++;
	buffer._scanNumber += scanNumber;
	buffer._interval += interval;
	
	if(buffer._rowTimestepCount == _timeAvgFactor)
		writeCurrentTimestep(antenna1, antenna2);
}
