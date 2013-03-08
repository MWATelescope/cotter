#include "gausencoder.h"
#include "weightencoder.h"

#include <ms/MeasurementSets/MeasurementSet.h>
#include <tables/Tables/ArrayColumn.h>
#include <tables/Tables/ScalarColumn.h>

#include <iostream>
#include <stdexcept>

/**
 * @file
 * @brief Contains the main function for the simcompress application.
 * 
 * simcompress is a tool to simulate the effects of compression. It won't change the actual
 * storage manager, but will change all values in a column to the values that they would have
 * if they were compressed and decompressed.
 * @author André Offringa
 */

/**
 * Simulate compressing a given measurement set.
 * @param argc Command line parameter count
 * @param argv Command line parameters.
 */
int main(int argc, char *argv[])
{
	if(argc < 4)
	{
		std::cout <<
			"simcompress is a tool to simulate the effects of compression. It won't change the actual\n"
			"storage manager, but will change all values in a column to the values that they would have\n"
			"if they were compressed and decompressed.\n\n"
			"Syntax: simcompress <measurementset> <column> <quantcount> [<stddev>]\n\n"
			"If the column contains complex values, the Gaussian encoder will be used. If it\n"
			"contains floats, the Weight encoder will be used.\n"
			"stddev will only be used when the Gaussian encoder is used.\n\n"
			"In the normal case, the column specified is either the DATA or the WEIGHT_SPECTRUM\n"
			"column.\n";
			return 0;
	}
	
	casa::MeasurementSet ms(argv[1], casa::Table::Update);
	const std::string columnName = argv[2];
	const unsigned quantCount = atoi(argv[3]);
	
	if(ms.columnDataType(ms.columnType(columnName)) == casa::TpArrayComplex)
	{
		std::cout << "Column " << columnName << " is a Complex column; will use Gaussian encoder.\n";
		if(argc < 5)
			throw std::runtime_error("Can't encode complex column without stddev: please specify the stddev");
		const float stddev = atof(argv[4]);
		GausEncoder<float> encoder(quantCount, stddev);
		casa::ArrayColumn<casa::Complex> column(ms, columnName);
		casa::Array<casa::Complex> array(column.shape(0));
		std::cout << "Encoding with " << quantCount << " quantization levels and standard deviation of " << stddev << "...\n";
		for(size_t row=0; row!=ms.nrow(); ++row)
		{
			column.get(row, array);
			for(casa::Array<casa::Complex>::contiter i=array.cbegin(); i!=array.cend(); ++i)
			{
				i->real() = encoder.Decode(encoder.Encode(i->real()));
				i->imag() = encoder.Decode(encoder.Encode(i->imag()));
			}
			column.put(row, array);
		}
	}
	else if(ms.columnDataType(ms.columnType(columnName)) == casa::TpArrayFloat)
	{
		std::cout << "Column " << columnName << " is a float column; will use Weight encoder.\n";
		casa::ArrayColumn<float> column(ms, columnName);
		WeightEncoder<float> encoder(quantCount);
		casa::Array<float> array(column.shape(0));
		std::vector<float> vals(array.cend() - array.cbegin());
		std::vector<unsigned> encodedData(vals.size());
		std::cout << "Encoding with " << quantCount << " quantization levels...\n";
		for(size_t row=0; row!=ms.nrow(); ++row)
		{
			column.get(row, array);
			
			std::vector<float>::iterator valIter = vals.begin();
			for(casa::Array<float>::contiter i=array.cbegin(); i!=array.cend(); ++i)
			{
				*valIter = *i;
				++valIter;
			}
			
			float scaleVal;
			encoder.Encode(scaleVal, encodedData, vals);
			encoder.Decode(vals, scaleVal, encodedData);
			
			valIter = vals.begin();
			for(casa::Array<float>::contiter i=array.cbegin(); i!=array.cend(); ++i)
			{
				*i = *valIter;
				++valIter;
			}
			
			column.put(row, array);
		}
	}
	
	return 0;
}
