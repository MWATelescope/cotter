#include "gausencoder.h"
#include "stopwatch.h"
#include "bytepacker.h"
#include "weightencoder.h"

#include <iostream>
#include <cmath>

double uniformToGaus(double u1, double u2)
{
	double
		u = u1,
		v = u2;
	return sqrt(-2.0*log(u))*cos(2.0*M_PI*v);
}

long double errorInFloatExp(signed char exponent)
{
	// mantisse has 23 bits significance, so avg distance (=error) is 0.5 / (2^23) = 2^-24
	long double err = powl(2.0L, (long double) exponent - 24.0L);
	return err * err;
}

long double gausProbability(signed char exponent, long double sigma)
{
	long double x = powl(2.0L, (long double) exponent - 1.0L);
	return 1.0 / (sqrtl(M_PI) * sigma) * expl(-x*x / (sigma*sigma));
}

long double gausErrorInFloat(long double sigma)
{
	long double totalErr = 0.0L;
	for(signed int e=-126; e!=128; ++e)
	{
		long double weight = powl(2.0L, (long double) e);
		totalErr += errorInFloatExp(e) * gausProbability(e, sigma) * weight;
	}
	return totalErr;
}

long double uniformErrorInFloat(long double sigma)
{
	long double totalErr = 0.0L;
	for(signed int e=-126; e!=1; ++e)
	{
		long double weight = powl(2.0L, (long double) e);
		totalErr += errorInFloatExp(e) * 1.0 * weight;
	}
	return totalErr;
}

template<typename T>
void assertEqualArray(const T *expected, const T *actual, size_t size, const std::string &msg)
{
	for(size_t i=0; i!=size; ++i)
	{
		if(expected[i] != actual[i])
		{
			std::cout << "Failed assertEqualArray() for test " << msg << ":\nExpected: {" << (int) expected[0];
			for(size_t j=1; j!=size; ++j)
				std::cout << ", " << (int) expected[j];
			std::cout << "}\n  Actual: {" << (int) actual[0];
			for(size_t j=1; j!=size; ++j)
				std::cout << ", " << (int) actual[j];
			std::cout << "}\n";
			return;
		}
	}
}

void performBytePackTests()
{
	size_t bitSizes[5] = {4, 6, 8, 10, 12};
	for(size_t i=0; i!=5; ++i)
	{
		for(size_t s=0; s!=12; ++s)
		{
			unsigned arr[12] = { 1, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31 };
			unsigned expected[13] = {37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37};
			for(size_t x=0;x!=12;++x)
				arr[x] &= (1<<bitSizes[i]) - 1;
	
			unsigned result[13] = { 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37};
			unsigned char packed[24], packedOr[24];
			memset(packed, 39, 24);
			memset(packedOr, 39, 24);
			BytePacker::pack(bitSizes[i], packed, arr, s);
			BytePacker::unpack(bitSizes[i], result, packed, s);
			for(size_t x=0; x!=s; ++x)
				expected[x] = arr[x];
			std::stringstream msg;
			msg<< "result of pack+unpack (l=" << s << ",bits=" << bitSizes[i] << ')';
			assertEqualArray(expected, result, 13, msg.str());
			size_t packedSize = ((s*bitSizes[i]+7)/8);
			assertEqualArray(packedOr, &packed[packedSize], 20-packedSize, "packed not overwritten past length");
		}
		
		unsigned arr2[15];
		for(size_t x=0; x!=15; ++x)
			arr2[x] = (1<<bitSizes[i]) - 1;
		unsigned char packed[30];
		memset(packed, 0, 30);
		unsigned result[15];
		memset(result, 0, 15*sizeof(unsigned));
		BytePacker::pack(bitSizes[i], packed, arr2, 15);
		BytePacker::unpack(bitSizes[i], result, packed, 15);
		std::stringstream msg;
		msg<< "all bits set (bits=" << bitSizes[i] << ')';
		assertEqualArray(arr2, result, 15, msg.str());
	}
	
}

void encodeMBs(const GausEncoder<float> &encoder, size_t mb, unsigned bitCount)
{
	float input[100];
	unsigned encodeBuffer[100];
	unsigned char packed[400];
	for(size_t xi2=0;xi2!=100;++xi2)
	{
		input[xi2] = uniformToGaus(xi2/100.0, (xi2%10)/10.0);
	}
	for(size_t xi=0;xi!=mb*2500;++xi)
	{
		for(size_t xi2=0;xi2!=100;++xi2)
			encodeBuffer[xi2] = encoder.Encode(input[xi2]);
		BytePacker::pack(bitCount, packed, encodeBuffer, 100);
		volatile unsigned char x = packed[47];
	}
}

void decodeMBs(const GausEncoder<float> &encoder, size_t mb, unsigned bitCount)
{
	float output[100];
	unsigned symbolBuffer[100];
	unsigned char packed[400];
	unsigned modulo = (1<<bitCount);
	for(size_t xi2=0;xi2!=400;++xi2)
	{
		packed[xi2] = xi2*7;
	}
	for(size_t xi=0;xi!=mb*2500;++xi)
	{
		BytePacker::unpack(bitCount, symbolBuffer, packed, 100);
		for(size_t xi2=0;xi2!=100;++xi2)
			output[xi2] = encoder.Decode(symbolBuffer[xi2]%modulo);
		volatile float x = output[47];
	}
}

void testGausEncoder()
{
	std::cout << "QuantCount\tAbsError-uniform\tAbsError-Gaussian\tRms-uniform\tRms-Gaussian\tEncSpeed\tDecSpeed\n";
	for(unsigned i=1; i!=21; ++i)
	{
		size_t quantCount = (1<<i);
		GausEncoder<float> encoder(quantCount, 1.0, true);

		long double uniformStddevFact = sqrtl(3.0);
		long double errorUniform = 0.0, errorGaus = 0.0, rmsUniform = 0.0, rmsGaus = 0.0;
		long double uniformStddev = 0.0;
		for(size_t xi=0;xi!=1000000;++xi)
		{
			float x = ((xi * 2) / 1000000.0 - 1.0) * uniformStddevFact;
			uniformStddev += x*x;
			unsigned symbol = encoder.Encode(x);
			float xDecoded = encoder.Decode(symbol);
			errorUniform += fabsl(xDecoded - x);
			rmsUniform += (xDecoded - x) * (xDecoded - x);
			
			x = uniformToGaus((xi%1000+0.5)/1000.0, (xi/1000+0.5)/1000.0);
			symbol = encoder.Encode(x);
			xDecoded = encoder.Decode(symbol);
			//std::cout << x << '\t' << xDecoded << '\n';
			errorGaus += fabsl(xDecoded - x);
			rmsGaus += (xDecoded - x) * (xDecoded - x);
		}
		Stopwatch watchA(true);
		encodeMBs(encoder, 4, i);
		watchA.Pause();
		
		Stopwatch watchB(true);
		decodeMBs(encoder, 32, i);
		watchB.Pause();
		
		std::cout
			<< i << '\t'
			<< (errorUniform / 1000000.0) << '\t'
			<< (errorGaus / 1000000.0) << '\t'
			<< sqrtl(rmsUniform / 1000000.0) << '\t'
			<< sqrtl(rmsGaus / 1000000.0) << '\t'
			<< (4.0/watchA.Seconds()) << '\t'
			<< (32.0/watchB.Seconds()) << '\t'
			<< encoder.Decode(quantCount/2) << '\t'
			<< encoder.Decode(quantCount-1) << '\t'
			<< sqrtl(uniformStddev/1000000.0) << '\n';
	}
}

void testDistribution()
{
	const size_t quantCount = 16;
	GausEncoder<float> encoder(quantCount, 1.0, true);
	unsigned counts[quantCount];
	memset(counts, 0, quantCount*sizeof(unsigned));
	for(size_t xi=0;xi!=1000000;++xi)
	{
		//double
		//	u = (double) rand() / RAND_MAX,
		//	v = (double) rand() / RAND_MAX;
		double u = (xi%1000+0.5)/1000.0, v = (xi/1000+0.5)/1000.0;
		float x = uniformToGaus(u, v);
		counts[encoder.Encode(x)]++;
	}
	for(size_t i=0;i!=quantCount;++i)
	{
		std::cout << i << '\t' << counts[i] << '\t' << encoder.Decode(i) << ',' << encoder.RightBoundary(i) << '\n';
	}
	std::cout << encoder.RightBoundary(quantCount-2)+1.0 << "->" << encoder.Encode(encoder.RightBoundary(quantCount-2)+1.0) << '\n';
	std::cout << "0->" << encoder.Encode(0.0) << '\n';
}

void testWeightEncoder()
{
	std::cout << "QuantCount\tAbsError-uniform\tAbsError-Gaussian\tRms-uniform\tRms-Gaussian\tEncSpeed\tDecSpeed\n";
	for(unsigned i=0; i!=21; ++i)
	{
		size_t quantCount = (1<<i);
		WeightEncoder<float> encoder(quantCount);
		
		long double errorUniform = 0.0, errorGaus = 0.0, rmsUniform = 0.0, rmsGaus = 0.0;
		std::vector<float> uniInp(100), gausInp(100);
		std::vector<float> uniBuffer(100), gausBuffer(100);
		std::vector<unsigned> encBuffer(100);
		for(size_t xi1=0;xi1!=10000;++xi1)
		{
			for(size_t xi2=0;xi2!=100;++xi2)
			{
				size_t xi = xi1 * 100 + xi2;
				float x = (xi * 2) / 1000000.0 - 1.0;
				uniInp[xi2] = x;
				x = uniformToGaus((xi%1000+0.5)/1000.0, (xi/1000+0.5)/1000.0);
				gausInp[xi2] = x;
			}
			float val;
			encoder.Encode(val, encBuffer, uniInp);
			encoder.Decode(uniBuffer, val, encBuffer);
			encoder.Encode(val, encBuffer, gausInp);
			encoder.Decode(gausBuffer, val, encBuffer);
			for(size_t xi2=0;xi2!=100;++xi2)
			{
				errorUniform += fabsl(uniBuffer[xi2] - uniInp[xi2]);
				rmsUniform += (uniBuffer[xi2] - uniInp[xi2]) * (uniBuffer[xi2] - uniInp[xi2]);
				
				errorGaus += fabsl(gausBuffer[xi2] - gausInp[xi2]);
				rmsGaus += (gausBuffer[xi2] - gausInp[xi2]) * (gausBuffer[xi2] - gausInp[xi2]);
			}
		}
		unsigned char packed[100];
		Stopwatch watchA(true);
		for(size_t xi2=0;xi2!=100;++xi2)
		{
			uniInp[xi2] = (xi2*7)%64;
		}
		for(size_t xi=0;xi!=40000;++xi)
		{
			float val;
			encoder.Encode(val, encBuffer, uniInp);
			if(i == 6)
				BytePacker::pack6(packed, &encBuffer[0], 100);
			volatile unsigned char x = packed[47];
		}
		watchA.Pause();
		
		Stopwatch watchB(true);
		for(size_t xi=0;xi!=40000;++xi)
		{
			if(i == 6)
				BytePacker::unpack6(&encBuffer[0], packed, 100);
			encoder.Decode(uniBuffer, (xi/40000.0), encBuffer);
			volatile float x = uniBuffer[49];
		}
		watchB.Pause();
		
		std::cout
			<< i << '\t'
			<< (errorUniform / 1000000.0) << '\t'
			<< (errorGaus / 1000000.0) << '\t'
			<< sqrtl(rmsUniform / 1000000.0) << '\t'
			<< sqrtl(rmsGaus / 1000000.0) << '\t'
			<< (4.0/watchA.Seconds()) << '\t'
			<< (4.0/watchB.Seconds()) << '\n';
	}
}

void makeErrorHistogram(size_t quantCount)
{
	GausEncoder<float> encoder(quantCount, 1.0);
	const size_t histSize = 2000, iterationCount = 10000000;
	const double xRange = 0.5; // stop at xRange sigma
	std::vector<size_t> histogram(histSize);
	for(size_t i=0;i!=iterationCount;++i)
	{
		double
			u = (double) rand() / RAND_MAX,
			v = (double) rand() / RAND_MAX;
		float x = uniformToGaus(u, v);
		float y = encoder.Decode(encoder.Encode(x));
		float err = x - y;
		float errIndex = round(err * histSize/(2.0*xRange) + (histSize/2)); // histogram up to 3 sigma
		if(errIndex >= 0.0 && errIndex <= histSize)
		{
			histogram[(size_t) errIndex]++;
		}
	}
	for(size_t i=0;i!=histSize;++i)
	{
		std::cout << (i-(histSize/2.0))*(xRange*2.0/histSize) << '\t' << (histogram[i]/(long double) iterationCount) << '\n';
	}
}

int main(int argc, char **argv) {
	performBytePackTests();
	
	long double e = gausErrorInFloat(1.0L), f = uniformErrorInFloat(1.0);
	std::cout << "Total float distance for gaus: " << e << '\n';
	std::cout << "Total float distance for uniform: " << f << '\n';
	
	//testDistribution();
	//testWeightEncoder();
	testGausEncoder();
	//makeErrorHistogram(256);
}
