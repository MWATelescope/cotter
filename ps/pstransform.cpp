#include <iostream>
#include <map>

#include <unistd.h>

#include "fft2d.h"
#include "fitsreader.h"
#include "fitswriter.h"

#include "aocommon/uvector.h"
#include "windowfunction.h"

fftw_plan fftPlan;

ao::uvector<std::complex<double>*> dataCube;
ao::uvector<double> weightsPerChannel;
ao::uvector<double*> psData;
std::map<double, size_t> perpGrid;
double weightSum;

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
		double powerBefore = 0.0;
		if(pixelIndex%133700==0)
			powerBefore = Power(pixelIndex);
		for(size_t i=0; i!=dataCube.size(); ++i)
			inputData[i] = dataCube[i][pixelIndex];
		fftw_execute_dft(fftPlan, reinterpret_cast<fftw_complex*>(inputData), reinterpret_cast<fftw_complex*>(outputData));
		for(size_t i=0; i!=dataCube.size(); ++i)
			dataCube[i][pixelIndex] = outputData[i] * factor;
		if(pixelIndex%133700==0)
			std::cout << pixelIndex << " -- " << powerBefore << " Jy --> " << Power(pixelIndex) << " Jy\n";
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
	ao::uvector<double> weights(perpGrid.size());
	ao::uvector<std::complex<double>> sums(perpGrid.size());
	
	while(transformTasks.read(channelIndex))
	{
		std::complex<double>* input = dataCube[channelIndex];
		double* output = new double[perpGrid.size()];
		weights.assign(perpGrid.size(), 0.0);
		sums.assign(perpGrid.size(), 0.0);
		psData[channelIndex] = output;
		
		for(size_t y=0; y!=fft->Height(); ++y)
		{
			size_t sourceY = y;
			if(y > midY)
				sourceY = height-y;
			int destY = y;
			if(destY >= int(midY)) destY -= height;
			const std::complex<double>* rowIn = &input[sourceY*(width/2+1)];
			for(size_t x=0; x!=midX+1; ++x)
			{
				double distance = sqrt(destY*destY + x*x);
				std::map<double, size_t>::const_iterator perpIndex = perpGrid.lower_bound(distance);
				if(perpIndex != perpGrid.begin())
				{
					std::map<double, size_t>::const_iterator before = perpIndex;
					--before;
					if(distance - before->first < perpIndex->first - distance)
						perpIndex = before;
				}
				size_t perpIntIndex = perpIndex->second;
				sums[perpIntIndex] += rowIn[x];
				++weights[perpIntIndex];
			}
		}
		for(size_t i=0; i!=perpGrid.size(); ++i)
		{
			output[i] = std::norm(sums[i]) / weights[i];
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
		windowValue *= weightsPerChannel[channelIndex] * dataCube.size() / weightSum;
		for(size_t i=0; i!=nComplex; ++i)
			input[i] *= windowValue;
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
	for(int i=1; i!=argc; ++i)
	{
		FitsReader reader(argv[i]);
		if(reader.Frequency() <= frequency)
			throw std::runtime_error("Files are not sorted on frequency!");
		frequency = reader.Frequency();
		double weight = 0.0;
		if(!reader.ReadDoubleKeyIfExists("WSCIMGWG", weight))
			std::cout << "Warning: file '" << argv[i] << "' did not have WSCIMGWG field: will have zero weight.\n";
		weightSum += weight;
		double* input = new double[width*height];
		reader.Read(input);
		std::complex<double>* output = new std::complex<double>[fft.NComplex()];
		dataCube.push_back(output);
		weightsPerChannel.push_back(weight);
		fft.AddTask(input, output);
	}
	fft.Finish();
	fft.SaveUV(dataCube.front(), "UV-first-image.fits");
	
	std::complex<double>* inputData = reinterpret_cast<std::complex<double>*>(fftw_malloc(dataCube.size() * sizeof(std::complex<double>)));
	std::complex<double>* outputData = reinterpret_cast<std::complex<double>*>(fftw_malloc(dataCube.size() * sizeof(std::complex<double>)));
	fftPlan = fftw_plan_dft_1d(dataCube.size(), reinterpret_cast<fftw_complex*>(inputData), reinterpret_cast<fftw_complex*>(outputData), FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_free(outputData);
	fftw_free(inputData);
	
	std::cout << "Applying window function...\n";
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
	
	size_t perGridSize = 250;
	for(size_t dist=0; dist!=perGridSize; ++dist)
	{
		perpGrid.insert(std::make_pair(double(dist*width)*0.5*M_SQRT2/perGridSize, dist));
	}
	psData.resize(dataCube.size());
	
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
	
	std::cout << "Making final image...\n";
	ao::uvector<double> psImage(perpGrid.size() * dataCube.size());
	double* psImagePtr = psImage.data();
	for(size_t y=0; y!=dataCube.size(); ++y)
	{
		for(size_t x=0; x!=perpGrid.size(); ++x)
		{
			*psImagePtr = psData[y][x];
			++psImagePtr;
		}
	}
	FitsWriter writer;
	writer.SetImageDimensions(perpGrid.size(), dataCube.size());
	writer.Write("ps.fits", psImage.data());
}
