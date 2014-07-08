#include <iostream>
#include <map>

#include <unistd.h>

#include "fft2d.h"
#include "fitsreader.h"
#include "fitswriter.h"

#include "aocommon/uvector.h"
#include "universe.h"
#include "windowfunction.h"

fftw_plan fftPlan;

const double SPEED_OF_LIGHT = 299792458.0;
const double BOLTZMANN = 1.3806488e-23;

const size_t gridSizeX = 1024, gridSizeY = 768;
const bool convertToTemperature = true;

ao::uvector<std::complex<double>*> dataCube;
ao::uvector<double> weightsPerChannel;
ao::uvector<double> frequencies;
ao::uvector<double> beamOmegas;
ao::uvector<double*> psData;
ao::uvector<size_t> psDataCount;
std::map<double, size_t> perpGrid;
double weightSum, windowSum;

ao::lane<size_t> transformTasks;

double Power(size_t pixelIndex)
{
	double power = 0.0;
	for(size_t i=0; i!=dataCube.size(); ++i)
		power += std::norm(dataCube[i][pixelIndex]);
	return power;
}

void transformParThread(size_t n)
{
	std::complex<double>* inputData = reinterpret_cast<std::complex<double>*>(fftw_malloc(n * sizeof(std::complex<double>)));
	std::complex<double>* outputData = reinterpret_cast<std::complex<double>*>(fftw_malloc(n * sizeof(std::complex<double>)));
	size_t pixelIndex;
	const double factor = 1.0 / sqrt(dataCube.size());
	while(transformTasks.read(pixelIndex))
	{
		//double powerBefore = 0.0;
		//if(pixelIndex%133700==0)
		//	powerBefore = Power(pixelIndex);
		for(size_t i=0; i!=dataCube.size(); ++i)
			inputData[i] = dataCube[i][pixelIndex];
		fftw_execute_dft(fftPlan, reinterpret_cast<fftw_complex*>(inputData), reinterpret_cast<fftw_complex*>(outputData));
		for(size_t i=0; i!=dataCube.size(); ++i)
			dataCube[i][pixelIndex] = outputData[i] * factor;
		//if(pixelIndex%133700==0)
		//	std::cout << pixelIndex << " -- " << powerBefore << " Jy --> " << Power(pixelIndex) << " Jy\n";
	}
	fftw_free(outputData);
	fftw_free(inputData);
}

void averageAnuli(FFT2D* fft)
{
	const size_t
		height = fft->Height(), midY = height/2,
		width = fft->Width(), midX = width/2;
	size_t channelIndex;
	ao::uvector<size_t> weights(perpGrid.size());
	ao::uvector<double> sums(perpGrid.size());
	
	while(transformTasks.read(channelIndex))
	{
		std::complex<double>* input = dataCube[channelIndex];
		double* output = new double[perpGrid.size()];
		weights.assign(perpGrid.size(), 0);
		sums.assign(perpGrid.size(), 0.0);
		psData[channelIndex] = output;
		
		for(size_t y=0; y!=height; ++y)
		{
			size_t sourceY = y;
			if(y > midY)
				sourceY = height-y;
			int destY = y;
			if(destY >= int(midY)) destY -= height;
			const std::complex<double>* rowIn = &input[sourceY*(width/2+1)];
			for(size_t x=0; x!=width; ++x)
			{
				size_t sourceX = x;
				if(x > midX)
					sourceX = width-x;
				int destX = x;
				if(destX > int(midX)) destX -= width;
				double distance = sqrt(destY*destY + destX*destX);
				std::map<double, size_t>::const_iterator perpIndex = perpGrid.lower_bound(distance);
				if(perpIndex != perpGrid.begin())
				{
					std::map<double, size_t>::const_iterator before = perpIndex;
					--before;
					if(distance - before->first < perpIndex->first - distance)
						perpIndex = before;
				}
				size_t perpIntIndex = perpIndex->second;
				sums[perpIntIndex] += std::norm(rowIn[sourceX]);
				++weights[perpIntIndex];
			}
		}
		for(size_t i=0; i!=perpGrid.size(); ++i)
		{
			output[i] = sums[i] / weights[i];
			psDataCount[i] += weights[i];
		}
		
		delete[] input;
	}
}

void applyWindow(size_t nComplex)
{
	size_t channelIndex;
	
	while(transformTasks.read(channelIndex))
	{
		std::complex<double>* input = dataCube[channelIndex];
		double windowValue = WindowFunction::blackmanNutallWindow(dataCube.size(), channelIndex);
		if(convertToTemperature) {
			double f = frequencies[channelIndex];
			double jToT = 1e-23 * SPEED_OF_LIGHT*SPEED_OF_LIGHT / (2.0 * BOLTZMANN * f * f * beamOmegas[channelIndex]);
			//std::cout << jToT << '\n';
			windowValue *= jToT * weightsPerChannel[channelIndex] / (weightSum * windowSum);
		}
		else
			windowValue *= weightsPerChannel[channelIndex] / (weightSum * windowSum);
		for(size_t i=0; i!=nComplex; ++i)
			input[i] *= windowValue;
	}
}

void makePerpGrid(size_t width)
{
	double maxVal = double(width)*0.5*M_SQRT2;
	double minVal = 4e-3 * maxVal;
	double nmin = 0, nmax = gridSizeX;
	double eRange = (log(maxVal)-log(minVal))/(nmax-nmin);
	perpGrid.insert(std::make_pair(0.0, 0));
	for(size_t dist=0; dist!=gridSizeX-1; ++dist)
	{
		double x = dist;
		double logVal = minVal * exp(eRange * x) - nmin; //dist*maxVal/perGridSize
		perpGrid.insert(std::make_pair(logVal, dist+1));
	}
}

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		std::cout << "syntax: pstransform <weighted cube list>\n";
		return 0;
	}
	
	size_t cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
	FitsReader firstImage(argv[1]);
	size_t
		width = firstImage.ImageWidth(),
		height = firstImage.ImageHeight();
	FFT2D fft(width, height, cpuCount);
	fft.Start();
	double frequency = 0;
	std::cout << "Reading and Fourier transforming images...\n";
	for(int i=1; i!=argc; ++i)
	{
		FitsReader reader(argv[i]);
		if(reader.Frequency() <= frequency)
			throw std::runtime_error("Files are not sorted on frequency!");
		frequency = reader.Frequency();
		double weight = 0.0;
		if(!reader.ReadDoubleKeyIfExists("WSCIMGWG", weight))
			std::cout << "Warning: file '" << argv[i] << "' did not have WSCIMGWG field: will have zero weight.\n";
		double beam = 0.0;
		if(!reader.ReadDoubleKeyIfExists("BMAJ", beam))
			std::cout << "Warning: file '" << argv[i] << "' did not have BMAJ field: will have zero beam.\n";
		weightSum += weight;
		double* input = new double[width*height];
		reader.Read(input);
		std::complex<double>* output = new std::complex<double>[fft.NComplex()];
		dataCube.push_back(output);
		weightsPerChannel.push_back(weight);
		frequencies.push_back(frequency);
		beam *= M_PI/180.0;
		beamOmegas.push_back(M_PI*beam*beam/(4.0*M_LN2));
		fft.AddTask(input, output);
		//Testing
		//for(size_t j=0; j!=fft.NComplex(); ++j) output[j] = 0.0;
		//for(size_t j=0; j!=width/2; ++j) output[j] = width/2-j;
	}
	fft.Finish();
	fft.SaveUV(dataCube.front(), "UV-first-image.fits");

	double redshiftLo = Universe::HIRedshift(frequencies.back()), redshiftHi = Universe::HIRedshift(frequencies.front());
	std::cout << "- Redshift range: " << redshiftLo << " - " << redshiftHi << '\n';
	std::cout << "- Distance: " << Universe::ComovingDistanceInMPC(redshiftLo) << " - " << Universe::ComovingDistanceInMPC(redshiftHi) << " Mpc\n";
	double distanceRange = (Universe::ComovingDistanceInMPC(redshiftHi)-Universe::ComovingDistanceInMPC(redshiftLo))/dataCube.size();
	std::cout << "- Grid step: " << distanceRange << " Mpc\n";
	
	std::complex<double>* inputData = reinterpret_cast<std::complex<double>*>(fftw_malloc(dataCube.size() * sizeof(std::complex<double>)));
	std::complex<double>* outputData = reinterpret_cast<std::complex<double>*>(fftw_malloc(dataCube.size() * sizeof(std::complex<double>)));
	fftPlan = fftw_plan_dft_1d(dataCube.size(), reinterpret_cast<fftw_complex*>(inputData), reinterpret_cast<fftw_complex*>(outputData), FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_free(outputData);
	fftw_free(inputData);
	
	std::cout << "Making image with total uv power...\n";
	ao::uvector<std::complex<double>> sumImage(fft.NComplex(), 0.0);
	for(size_t ch=0; ch!=dataCube.size(); ++ch)
	{
		for(size_t i=0; i!=fft.NComplex(); ++i)
			sumImage[i] += dataCube[ch][i];
	}
	FitsWriter sumWriter;
	fft.SaveUV(sumImage.data(), "uv-total.fits");
	
	std::cout << "Applying window function... " << std::flush;
	windowSum = 0.0;
	for(size_t channelIndex=0; channelIndex!=dataCube.size(); ++channelIndex)
		windowSum += WindowFunction::blackmanNutallWindow(dataCube.size(), channelIndex);
	transformTasks.resize(cpuCount);
	std::vector<std::thread> threads;
	for(size_t i=0; i!=cpuCount; ++i)
		threads.push_back(std::thread(&applyWindow, fft.NComplex()));
	for(size_t i=0; i!=dataCube.size(); ++i)
		transformTasks.write(i);
	transformTasks.write_end();
	for(std::vector<std::thread>::iterator i=threads.begin(); i!=threads.end(); ++i)
		i->join();
	transformTasks.clear();
	threads.clear();
	std::cout << "Average weight=" << (windowSum/dataCube.size()) << '\n';
	
	std::cout << "Transforming in parallel direction...\n";
	for(size_t i=0; i!=cpuCount; ++i)
		threads.push_back(std::thread(&transformParThread, dataCube.size()));
	for(size_t i=0; i!=fft.NComplex(); ++i)
		transformTasks.write(i);
	transformTasks.write_end();
	for(std::vector<std::thread>::iterator i=threads.begin(); i!=threads.end(); ++i)
		i->join();
	transformTasks.clear();
	threads.clear();
	
	makePerpGrid(width);
	
	psData.resize(dataCube.size());
	psDataCount.assign(perpGrid.size(), 0);
	
	std::cout << "Averaging anuli...\n";
	for(size_t i=0; i!=cpuCount; ++i)
		threads.push_back(std::thread(&averageAnuli, &fft));
	for(size_t i=0; i!=dataCube.size(); ++i)
		transformTasks.write(i);
	transformTasks.write_end();
	for(std::vector<std::thread>::iterator i=threads.begin(); i!=threads.end(); ++i)
		i->join();
	transformTasks.clear();
	threads.clear();

	std::cout << "Interpolating in perpendicular direction...\n";
	for(size_t x=0; x!=perpGrid.size(); ++x)
	{
		if(psDataCount[x] == 0)
		{
			int xRight = x, xLeft = x;
			while(xRight < int(perpGrid.size()) && psDataCount[xRight]==0) ++xRight;
			while(xLeft >= 0 && psDataCount[xLeft]==0) --xLeft;
			size_t nearestX;
			if(xRight<int(perpGrid.size()) && xRight - x < x - xLeft)
				nearestX = xRight;
			else
				nearestX = xLeft;
			for(size_t y=0; y!=dataCube.size(); ++y)
				psData[y][x] = psData[y][nearestX];
		}
	}
		
	std::cout << "Making image with linear scale for parallel direction...\n";
	ao::uvector<double> psLinearImage(perpGrid.size() * dataCube.size());
	double* psImagePtr = psLinearImage.data();
	for(size_t y=0; y!=dataCube.size(); ++y)
	{
		size_t sourceY = y+dataCube.size()/2;
		if(sourceY >= dataCube.size()) sourceY -= dataCube.size();
		for(size_t x=0; x!=perpGrid.size(); ++x)
		{
			*psImagePtr = psData[sourceY][x];
			++psImagePtr;
		}
	}
	FitsWriter writer;
	writer.SetExtraKeyword("PSXMIN", perpGrid.begin()->first);
	writer.SetExtraKeyword("PSXMAX", perpGrid.rbegin()->first);
	writer.SetExtraKeyword("PSYMIN", 0);
	writer.SetExtraKeyword("PSYMAX", distanceRange);
	writer.SetImageDimensions(perpGrid.size(), dataCube.size());
	writer.Write("ps-linear.fits", psLinearImage.data());
	
	std::cout << "Making log-log power spectrum...\n";
	size_t logGridSize = gridSizeY;
	ao::uvector<double> psLogImage(perpGrid.size() * logGridSize, 0.0);
	ao::uvector<size_t> psLogWeights(perpGrid.size() * logGridSize, 0);
	double minY = dataCube.size()*1e-2, maxY = dataCube.size();
	size_t midY = dataCube.size()/2;
	for(size_t y=0; y!=dataCube.size(); ++y)
	{
		size_t destY;
		if(y >= midY)
			destY = (y-midY)*2;
		else
			destY = (midY-y)*2;
		int yLog = int(round(double(logGridSize)/(log(maxY)-log(minY))*log(destY/minY)));
		if(yLog < 0) yLog = 0;
		else if(yLog >= int(logGridSize)) yLog = logGridSize-1;
		double
			*srcRow = &psLinearImage[y * perpGrid.size()],
			*destRow = &psLogImage[yLog * perpGrid.size()];
		size_t
			*destWeightRow = &psLogWeights[yLog * perpGrid.size()];
		for(size_t x=0; x!=perpGrid.size(); ++x)
		{
			destRow[x] += srcRow[x];
			++destWeightRow[x];
		}
	}
	double* imagePtr = psLogImage.data();
	size_t* weightPtr = psLogWeights.data();
	for(size_t y=0; y!=logGridSize; ++y)
	{
		if(*weightPtr != 0)
		{
			for(size_t x=0; x!=perpGrid.size(); ++x)
			{
				*imagePtr /= *weightPtr;
				++imagePtr;
				++weightPtr;
			}
		}
		else {
			imagePtr += perpGrid.size();
			weightPtr += perpGrid.size();
		}
	}
	for(size_t y=0; y!=logGridSize; ++y)
	{
		if(psLogWeights[y * perpGrid.size()] == 0) {
			// Find nearest row with values
			int yUp = y, yDown = y;
			while(yUp < int(logGridSize) && psLogWeights[yUp*perpGrid.size()]==0) ++yUp;
			while(yDown >= 0 && psLogWeights[yDown*perpGrid.size()]==0) --yDown;
			double
				*row = &psLogImage[y * perpGrid.size()],
				*nearestRowWithData;
			if(yUp<int(logGridSize) && yUp - y < y - yDown)
				nearestRowWithData = psLogImage.data() + yUp*perpGrid.size();
			else
				nearestRowWithData = psLogImage.data() + yDown*perpGrid.size();
			for(size_t x=0; x!=perpGrid.size(); ++x)
				row[x] = nearestRowWithData[x];
		}
	}
	writer.SetImageDimensions(perpGrid.size(), logGridSize);
	// TODO need to fix X axis min so that it is more accurate
	writer.SetExtraKeyword("PSXMIN", (++perpGrid.begin())->first);
	writer.SetExtraKeyword("PSXMAX", perpGrid.rbegin()->first);
	writer.SetExtraKeyword("PSYMIN", (minY/maxY)*distanceRange);
	writer.SetExtraKeyword("PSYMAX", distanceRange);
	writer.Write("ps.fits", psLogImage.data());
}
