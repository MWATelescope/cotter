#include "fitsreader.h"
#include "fitswriter.h"
#include "matrix2x2.h"

#include <boost/filesystem/operations.hpp>

#include <iostream>

void read(const FitsReader& templateReader, const std::string& filename, std::vector<double>& data, double* weightPtr)
{
	if(boost::filesystem::exists(filename))
	{
		FitsReader inpReader(filename);
		size_t
			width = inpReader.ImageWidth(),
			height = inpReader.ImageHeight();
			
		if(width != templateReader.ImageWidth() || height != templateReader.ImageHeight())
			throw std::runtime_error("Not all images had same size");
		data.resize(width * height);
		inpReader.Read<double>(&data[0]);
		if(weightPtr != 0)
		{
			*weightPtr = 0.0;
			if(!inpReader.ReadDoubleKeyIfExists("WSCIMGWG", *weightPtr))
				std::cout << "Warning: file '" << filename << "' did not have WSCIMGWG field: will have zero weight.\n";
		}
	}
	else {
		std::cout << "Warning: file '" << filename << "' did not exist, assuming it is zero!\n";
		data.resize(templateReader.ImageWidth() * templateReader.ImageHeight(), 0.0);
	}
}

void performWeighting(const std::string& imagePrefix, const std::string& imagePostfix, const std::string& beamPrefix, const std::string& psPrefix)
{
	std::string
		inpFilenames[4] =
			{ 
				imagePrefix + "-XX-" + imagePostfix, imagePrefix + "-XY-" + imagePostfix,
				imagePrefix + "-XYi-" + imagePostfix, imagePrefix + "-YY-" + imagePostfix
			},
		beamFilename[8] =
			{
				beamPrefix+"-xxr.fits", beamPrefix+"-xxi.fits", beamPrefix+"-xyr.fits", beamPrefix+"-xyi.fits",
				beamPrefix+"-yxr.fits", beamPrefix+"-yxi.fits", beamPrefix+"-yyr.fits", beamPrefix+"-yyi.fits"
			},
		outFilenames[4] =
			{ psPrefix+"-XX-weighted.fits", psPrefix+"-XY-weighted.fits",
				psPrefix+"-XYi-weighted.fits", psPrefix+"-YY-weighted.fits" };
	FitsReader firstImage(inpFilenames[0]);
	std::vector<double> inputData[4], beamData[8], inputWeights(4);
	for(size_t i=0; i!=4; ++i)
		read(firstImage, inpFilenames[i], inputData[i], &inputWeights[i]);
	for(size_t i=0; i!=8; ++i)
		read(firstImage, beamFilename[i], beamData[i], 0);
	
	const size_t 
		width = firstImage.ImageWidth(),
		height = firstImage.ImageHeight(),
		imgSize = width * height;
	for(size_t i=0; i!=imgSize; ++i)
	{
		std::complex<double> beamValues[4];
		beamValues[0] = std::complex<double>(beamData[0][i], beamData[1][i]);
		beamValues[1] = std::complex<double>(beamData[2][i], beamData[3][i]);
		beamValues[2] = std::complex<double>(beamData[4][i], beamData[5][i]);
		beamValues[3] = std::complex<double>(beamData[6][i], beamData[7][i]);
		
		std::complex<double> imgValues[4];
		imgValues[0] = inputData[0][i];
		imgValues[1] = std::complex<double>(inputData[1][i], inputData[2][i]);
		imgValues[2] = std::conj(imgValues[1]);
		imgValues[3] = inputData[3][i];
		
		std::complex<double> tempValues[4];
		Matrix2x2::ATimesB(tempValues, beamValues, imgValues);
		Matrix2x2::ATimesHermB(imgValues, tempValues, beamValues);
		inputData[0][i] = inputWeights[0] * imgValues[0].real();
		inputData[1][i] = inputWeights[1] * 0.5 * (imgValues[1].real() + imgValues[2].real());
		inputData[2][i] = inputWeights[2] * 0.5 * (imgValues[1].imag() - imgValues[2].imag());
		inputData[3][i] = inputWeights[3] * imgValues[3].real();
	}
	
	PolarizationEnum
		linPols[4] = { Polarization::XX, Polarization::XY, Polarization::XY, Polarization::YY };
	
	FitsWriter writer(firstImage);
	
	for(size_t p=0; p!=4; ++p)
	{
		writer.SetPolarization(linPols[p]);
		writer.Write<double>(outFilenames[p], &inputData[p][0]);
	}
}

int main(int argc, char* argv[])
{
	if(argc < 5)
		std::cout << "Syntax: psweight <image-prefix> <image-postfix> <beam-prefix> <ps prefix>\n";
	else {
		size_t argi = 1;
		const std::string
			imagePrefix = argv[argi], imagePostfix = argv[argi+1],
			beamPrefix = argv[argi+2], psPrefix = argv[argi+3];
		performWeighting(imagePrefix, imagePostfix, beamPrefix, psPrefix);
	}
}
