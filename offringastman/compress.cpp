#include "gausencoder.h"
#include "weightencoder.h"
#include "stmanmodifier.h"
#include "offringastman.h"

#include <ms/MeasurementSets/MeasurementSet.h>

#include <tables/Tables/ArrColDesc.h>
#include <tables/Tables/ArrayColumn.h>
#include <tables/Tables/ScalarColumn.h>

#include <auto_ptr.h>

using namespace offringastman;

casa::Complex encode(const GausEncoder<float> &encoder, casa::Complex val)
{
	casa::Complex e;
	e.real() = encoder.Decode(encoder.Encode(val.real()));
	e.imag() = encoder.Decode(encoder.Encode(val.imag()));
	return e;
}

float encode(const GausEncoder<float> &encoder, float val)
{
	return encoder.Decode(encoder.Encode(val));
}

void weightEncode(const WeightEncoder<float> &encoder, std::vector<casa::Complex> &vals, std::vector<unsigned> &temp)
{
	float scaleVal;
	std::vector<float> part(vals.size());
	for(size_t i=0; i!=vals.size(); ++i)
	{
		part[i] = fabs(vals[i].real());
	}
	encoder.Encode(scaleVal, temp, part);
	encoder.Decode(part, scaleVal, temp);
	for(size_t i=0; i!=vals.size(); ++i)
	{
		vals[i].real() = part[i];
		part[i] = fabs(vals[i].imag());
	}
	encoder.Encode(scaleVal, temp, part);
	encoder.Decode(part, scaleVal, temp);
	for(size_t i=0; i!=vals.size(); ++i)
		vals[i].imag() = part[i];
}

void weightEncode(const WeightEncoder<float> &encoder, std::vector<float> &vals, std::vector<unsigned> &temp)
{
	float scaleVal;
	encoder.Encode(scaleVal, temp, vals);
	encoder.Decode(vals, scaleVal, temp);
}

long double getSqErr(casa::Complex valA, casa::Complex valB)
{
	long double dr = valA.real() - valB.real();
	long double di = valA.imag() - valB.imag();
	return dr*dr + di*di;
}

long double getSqErr(float valA, float valB)
{
	return (valA - valB) * (valA - valB);
}

long double getAbsErr(casa::Complex valA, casa::Complex valB)
{
	return (fabsl((long double) valA.real() - valB.real()) +
		fabsl((long double) valA.imag() - valB.imag()));
}

long double getAbsErr(float valA, float valB)
{
	return fabsl((long double) valA - valB);
}
long double getManhDist(float valA, float valB)
{
	return valA - valB;
}
long double getManhDist(casa::Complex valA, casa::Complex valB)
{
	casa::Complex v = valA - valB;
	return v.real() + v.imag();
}
long double getEuclDist(float valA, float valB)
{
	return valA - valB;
}
long double getEuclDist(casa::Complex valA, casa::Complex valB)
{
	casa::Complex v = valA - valB;
	return sqrt(v.real()*v.real() + v.imag()*v.imag());
}

bool isGood(casa::Complex val) { return std::isfinite(val.real()) && std::isfinite(val.imag()); }
bool isGood(float val) { return std::isfinite(val); }
unsigned getCount(casa::Complex) { return 2; }
unsigned getCount(float) { return 1; }

template<typename T>
void MeasureError(casa::MeasurementSet &ms, const std::string &columnName, double stddev, unsigned quantCount)
{
	std::cout << "Calculating...\n";
	const GausEncoder<float> gEncoder(quantCount, stddev);
	const WeightEncoder<float> wEncoder(quantCount);
	long double gausSqErr = 0.0, gausAbsErr = 0.0, maxGausErr = 0.0;
	long double wSqErr = 0.0, wAbsErr = 0.0, maxWErr = 0.0;
	long double rms = 0.0, avgAbs = 0.0;
	long double avg = 0.0;
	size_t count = 0;
	
	casa::ArrayColumn<T> column(ms, columnName);
	casa::IPosition dataShape = column.shape(0);
	casa::Array<T> data(dataShape);
	const size_t elemSize = dataShape[0] * dataShape[1];
	
	casa::ArrayColumn<bool> flagColumn(ms, "FLAG");
	casa::Array<bool> flags(flagColumn.shape(0));
	
	std::vector<T> tempData(elemSize);
	std::vector<unsigned> weightEncodedSymbols(elemSize);
	
	casa::ScalarColumn<int> ant1Column(ms, "ANTENNA1"), ant2Column(ms, "ANTENNA2");
	
	for(size_t row=0; row!=ms.nrow(); ++row)
	{
		if(ant1Column(row) != ant2Column(row))
		{
			column.get(row, data);
			flagColumn.get(row, flags);
			
			casa::Array<bool>::const_contiter f=flags.cbegin();
			typename std::vector<T>::iterator tempDataIter = tempData.begin();
			for(typename casa::Array<T>::const_contiter i=data.cbegin(); i!=data.cend(); ++i)
			{
				if((!(*f)) && isGood(*i))
				{
					count += getCount(*i);
					
					*tempDataIter = *i;
					
					T eVal = encode(gEncoder, *i);
					
					gausSqErr += getSqErr(eVal, *i);
					gausAbsErr += getAbsErr(eVal, *i);
					
					rms += getSqErr(T(0.0), *i);
					avgAbs += getAbsErr(T(0.0), *i);
					
					avg = getManhDist(*i, T(0.0));
					if(getEuclDist(*i, eVal) > maxGausErr)
						maxGausErr = getEuclDist(*i, eVal);
				} else {
					*tempDataIter = T(0.0);
				}
				++f;
				++tempDataIter;
			}
			
			std::vector<T> originals(tempData);
			weightEncode(wEncoder, tempData, weightEncodedSymbols);
			for(size_t i=0; i!=tempData.size(); ++i)
			{
				wSqErr += getSqErr(originals[i], tempData[i]);
				wAbsErr += getAbsErr(originals[i], tempData[i]);
				if(getEuclDist(originals[i], tempData[i]) > maxWErr)
					maxWErr = getEuclDist(originals[i], tempData[i]);
			}
		}
	}
	std::cout
		<< "Encoded " << columnName << " with stddev " << stddev << '\n'
		<< "Sample count: " << count << '\n'
		<< "Avg: " << (avg/count) << '\n'
		<< "RMS: " << sqrt(rms/count) << '\n'
		<< "Average abs: " << avgAbs / count << '\n'
		<< "Root mean square error: " << sqrt(gausSqErr/count) << '\n'
		<< "Average absolute error: " << gausAbsErr / count << '\n'
		<< "Max error: " << maxGausErr << '\n'
		<< "Root mean square error (using weight encoder): " << sqrt(wSqErr/count) << '\n'
		<< "Average absolute error (using weight encoder): " << wAbsErr / count << '\n'
		<< "Max error (using weight encoder): " << maxWErr << '\n';
}

/**
 * Compress a given measurement set.
 * @param argc Command line parameter count
 * @param argv Command line parameters.
 */
int main(int argc, char *argv[])
{
	register_offringastman();
	
	if(argc < 2)
	{
		std::cout <<
			"Usage: compress [-meas <column>] <ms> [<bits per data val> [<bits per weight>]]\n"
			"Defaults: \n"
			"\tbits per data val = 8\n"
			"\tbits per weight = 6\n";
		return 0;
	}
	
	std::string columnName;
	int argi = 1;
	bool measure = false;
	
	if(strcmp(argv[argi], "-meas") == 0)
	{
		measure = true;
		columnName = argv[argi+1];
		argi+=2;
	}
	std::string msPath = argv[argi];
	unsigned bitsPerComplex=8, bitsPerWeight=6;
	if(argi+1 < argc) {
		bitsPerComplex = atoi(argv[argi+1]);
		if(argi+2 < argc)
			bitsPerWeight = atoi(argv[argi+2]);
	}
	
	std::cout << "Opening ms...\n";
	std::auto_ptr<casa::MeasurementSet> ms(new casa::MeasurementSet(msPath, casa::Table::Update));
	
	if(measure)
	{
		size_t quantCount = 1<<bitsPerComplex;
		if(columnName == "WEIGHT_SPECTRUM")
			MeasureError<float>(*ms, columnName, 1.0, quantCount);
		else
			MeasureError<casa::Complex>(*ms, columnName, 1.0, quantCount);
	} else {
		StManModifier modifier(*ms);
		
		bool isDataReplaced = modifier.InitComplexColumnWithOffringaStMan("DATA", bitsPerComplex, bitsPerWeight);
		bool isWeightReplaced = modifier.InitWeightColumnWithOffringaStMan("WEIGHT_SPECTRUM", bitsPerComplex, bitsPerWeight);
		
		if(isDataReplaced)
			modifier.MoveColumnData<casa::Complex>("DATA");
		if(isWeightReplaced)
			modifier.MoveColumnData<float>("WEIGHT_SPECTRUM");

		if(isDataReplaced || isWeightReplaced)
		{
			StManModifier::Reorder(ms, msPath);
		}
		
		std::cout << "Finished.\n";
	}
	
	return 0;
}
