#include "fft2d.h"
#include "fitswriter.h"
#include "../aocommon/uvector.h"

FFT2D::FFT2D(size_t width, size_t height, size_t cpuCount) :
	_tasks(cpuCount), _width(width), _height(height),
	_nComplex((_width/2+1) * _height)
{
	for(size_t i=0; i!=_tasks.capacity(); ++i)
	{
		double* inputData = reinterpret_cast<double*>(fftw_malloc(_width*_height * sizeof(double)));
		_inputBuffers.push_back(inputData);
		std::complex<double>* fftData = reinterpret_cast<std::complex<double>*>(fftw_malloc(_nComplex * sizeof(fftw_complex)));
		_outputBuffers.push_back(fftData);
	}
	
	_fftPlan = fftw_plan_dft_r2c_2d(_height, _width, _inputBuffers.front(), reinterpret_cast<fftw_complex*>(_outputBuffers.front()), FFTW_ESTIMATE);
}

FFT2D::~FFT2D()
{
	for(size_t i=0; i!=_tasks.capacity(); ++i)
	{
		fftw_free(_outputBuffers[i]);
		fftw_free(_inputBuffers[i]);
	}
	Finish();
}

void FFT2D::runThread(size_t cpuIndex)
{
	double* inpBuffer = _inputBuffers[cpuIndex];
	std::complex<double>* outBuffer = _outputBuffers[cpuIndex];
	Task task;
	while(_tasks.read(task))
	{
		//std::cout << "FFT::runThread(" << cpuIndex << ") -- " << _width << " x " << _height << "...\n";
		double powerBefore = Power(task.input);
		double normFactor = 1.0 / sqrt(_width * _height);
		for(size_t i=0; i!=_width*_height; ++i)
		{
			inpBuffer[i] = task.input[i] * normFactor;
		}
		fftw_execute_dft_r2c(_fftPlan, inpBuffer, reinterpret_cast<fftw_complex*>(outBuffer));
		memcpy(task.output, outBuffer, sizeof(std::complex<double>)*_nComplex);
		double powerAfter = PowerFromWrapped(task.output);
		//std::cout <<"FFT::runThread(" << cpuIndex << ") -- " << powerBefore << " Jy -> " << powerAfter << " Jy\n";
	}
}

void FFT2D::Unwrap(const std::complex<double>* uvIn, std::complex<double>* out)
{
	const size_t midY = _height/2, midX = _width/2;
	for(size_t y=0; y!=_height; ++y)
	{
		size_t sourceY = y;
		if(y > midY)
			sourceY = _height-y;
		size_t destY = y + midY;
		if(destY >= _height) destY -= _height;
		const std::complex<double>* rowIn = &uvIn[sourceY*(_width/2+1)];
		std::complex<double>* rowOut = &out[destY*_width];
		for(size_t x=0; x!=_width; ++x)
		{
			size_t sourceX = x;
			if(x > midX)
				sourceX = _width-x;
			size_t destX = x + midX;
			if(destX >= _width) destX -= _width;
			rowOut[destX] = rowIn[sourceX];
		}
	}
}

void FFT2D::SaveUV(const std::complex<double>* uv, const std::string& filename)
{
	ao::uvector<std::complex<double>> unwrapped(_width * _height);
	Unwrap(uv, unwrapped.data());
	ao::uvector<double> image(_width * _height);
	for(size_t i=0; i!=_width*_height; ++i)
		image[i] = std::norm(unwrapped[i]);
	FitsWriter writer;
	writer.SetImageDimensions(_width, _height);
	writer.Write(filename, image.data());
}
