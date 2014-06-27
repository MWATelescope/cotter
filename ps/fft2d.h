#ifndef FFT2D_H
#define FFT2D_H

#include <fftw3.h>

#include "aocommon/lane.h"

#include <thread>
#include <vector>
#include <complex>

class FFT2D
{
public:
	FFT2D(size_t width, size_t height, size_t cpuCount);
	
	~FFT2D();
	
	size_t NComplex() const { return _nComplex; }
	
	void AddTask(double* input, std::complex<double>* output)
	{
		Task task;
		task.input = input;
		task.output = output;
		_tasks.write(task);
	}
	
	void Start()
	{
		for(size_t i=0; i!=_tasks.capacity(); ++i)
		{
			_threads.push_back(std::thread(&FFT2D::runThread, this, i));
		}
	}
	
	void Finish()
	{
		_tasks.write_end();
		for(std::vector<std::thread>::iterator i=_threads.begin(); i!=_threads.end(); ++i)
			i->join();
		_threads.clear();
		_tasks.clear();
	}
	
	void RunSingle(double* input, std::complex<double>* output)
	{
		AddTask(input, output);
		_tasks.write_end();
		runThread(0);
		_tasks.clear();
	}
	
	void Unwrap(const std::complex<double>* uvIn, std::complex<double>* out);
	void SaveUV(const std::complex<double>* uv, const std::string& filename);

	double Power(const double* data) const
	{
		double total = 0.0;
		for(size_t i=0; i!=_width*_height; ++i)
			total += data[i]*data[i];
		return total;
	}
	
	double PowerFromWrapped(const std::complex<double>* data) const
	{
		double total = 0.0;
		for(size_t y=0; y!=_height; ++y)
		{
			total += std::norm(*data);
			++data;
			for(size_t x=0; x!=_width/2; ++x)
			{
				total += std::norm(*data)*2;
				++data;
			}
		}
		return total;
	}
	
	size_t Width() const { return _width; }
	size_t Height() const { return _height; }
private:
	struct Task {
		double* input;
		std::complex<double>* output;
	};
	
	ao::lane<Task> _tasks;
	std::vector<std::thread> _threads;
	size_t _width, _height, _nComplex;
	fftw_plan _fftPlan;
	
	void runThread(size_t cpuIndex);
	
	std::vector<double*> _inputBuffers;
	std::vector<std::complex<double>*> _outputBuffers;
};

#endif
