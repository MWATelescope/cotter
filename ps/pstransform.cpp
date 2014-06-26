#include <iostream>

#include <unistd.h>

#include "fft2d.h"
#include "fitsreader.h"

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		std::cout << "syntax: pstransform <weighted cube list>\n";
		return 0;
	}
	
	FitsReader firstImage(argv[1]);
	size_t
		width = firstImage.ImageWidth(),
		height = firstImage.ImageHeight();
	FFT2D fft(width, height, sysconf(_SC_NPROCESSORS_ONLN));
	fft.Start();
	std::vector<std::complex<double>*> data;
	for(int i=1; i!=argc; ++i)
	{
		FitsReader reader(argv[i]);
		double* input = new double[width*height];
		reader.Read(input);
		std::complex<double>* output = new std::complex<double>[fft.NComplex()];
		data.push_back(output);
		fft.AddTask(input, output);
	}
	fft.Finish();
	fft.SaveUV(data.front(), "out.fits");
}
