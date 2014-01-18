#ifndef IMAGE_COORDINATES_H
#define IMAGE_COORDINATES_H

#include <cmath>

/**
 * This class collects all the LM coordinate transform as defined in
 * Perley (1999)'s "imaging with non-coplaner arrays".
 */
class ImageCoordinates
{
	public:
		template<typename T>
		static void RaDecToLM(T ra, T dec, T phaseCentreRa, T phaseCentreDec, T &destL, T &destM)
		{
			const T
				deltaAlpha = ra - phaseCentreRa,
				sinDeltaAlpha = sin(deltaAlpha),
				cosDeltaAlpha = cos(deltaAlpha),
				sinDec = sin(dec),
				cosDec = cos(dec),
				sinDec0 = sin(phaseCentreDec),
				cosDec0 = cos(phaseCentreDec);
			
			destL = cosDec * sinDeltaAlpha;
			destM = sinDec*cosDec0 - cosDec*sinDec0*cosDeltaAlpha;
		}
		
		template<typename T>
		static T RaDecToN(T ra, T dec, T phaseCentreRa, T phaseCentreDec)
		{
			const T
				cosDeltaAlpha = cos(ra - phaseCentreRa),
				sinDec = sin(dec),
				cosDec = cos(dec),
				sinDec0 = sin(phaseCentreDec),
				cosDec0 = cos(phaseCentreDec);
			
			return sinDec*sinDec0 + cosDec*cosDec0*cosDeltaAlpha;
		}
		
		template<typename T>
		static void LMToRaDec(T l, T m, T phaseCentreRa, T phaseCentreDec, T &destRa, T &destDec)
		{
			const T
				cosDec0 = cos(phaseCentreDec),
				sinDec0 = sin(phaseCentreDec),
				lmTerm = sqrt((T) 1.0 - l*l - m*m),
				deltaAlpha = atan2(l, lmTerm*cosDec0 - m*sinDec0);
				
			destRa = deltaAlpha + phaseCentreRa;
			destDec = asin(m*cosDec0 + lmTerm*sinDec0);
		}
		
		template<typename T>
		static void XYToLM(size_t x, size_t y, T pixelSizeX, T pixelSizeY, size_t width, size_t height, T &l, T &m)
		{
			T midX = (T) width / 2.0, midY = (T) height / 2.0;
			l = (midX - (T) x) * pixelSizeX;
			m = ((T) y - midY) * pixelSizeY;
		}
		
		template<typename T>
		static void LMToXY(T l, T m, T pixelSizeX, T pixelSizeY, size_t width, size_t height, int &x, int &y)
		{
			T midX = (T) width / 2.0, midY = (T) height / 2.0;
			x = round(-l / pixelSizeX) + midX;
			y = round(m / pixelSizeY) + midY;
		}
		
		template<typename T>
		static void LMToXYfloat(T l, T m, T pixelSizeX, T pixelSizeY, size_t width, size_t height, T &x, T &y)
		{
			T midX = (T) width / 2.0, midY = (T) height / 2.0;
			x = -l / pixelSizeX + midX;
			y = m / pixelSizeY + midY;
		}
		
		template<typename T>
		static T AngularDistance(T ra1, T dec1, T ra2, T dec2)
		{
			T sinDec1, sinDec2, cosDec1, cosDec2;
			SinCos(dec1, &sinDec1, &cosDec1);
			SinCos(dec2, &sinDec2, &cosDec2);
			return std::acos(sinDec1*sinDec2 + cosDec1*cosDec2*std::cos(ra1 - ra2));
		}
	private:
		static void SinCos(double angle, double* sinAngle, double* cosAngle)
		{ sincos(angle, sinAngle, cosAngle); }
		
		static void SinCos(long double angle, long double* sinAngle, long double* cosAngle)
		{ sincosl(angle, sinAngle, cosAngle); }
		
		static void SinCos(float angle, float* sinAngle, float* cosAngle)
		{ sincosf(angle, sinAngle, cosAngle); }
		
		ImageCoordinates();
};

#endif
