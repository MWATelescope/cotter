#include <iostream>
#include <vector>

#include "uvector.h"
#include "stopwatch.h"

volatile int *ptr;

template<class Tp>
void doConstruct()
{
	for(std::size_t i=0; i!=2500000; ++i) {
		Tp tp(1000);
		ptr = &tp[0];
	}
}

template<class Tp>
void doConstructFill()
{
	for(std::size_t i=0; i!=1000000; ++i) {
		Tp tp(1000, 0);
		ptr = &tp[0];
	}
}

template<class Tp>
void doCopyConstruct()
{
	Tp vec(1000, 1);
	for(std::size_t i=0; i!=1000000; ++i) {
		Tp a(vec);
		ptr = &a[0];
	}
}

template<class Tp>
void doAssignCopy()
{
	Tp vec(1000, 1), a;
	for(std::size_t i=0; i!=1000000; ++i) {
		a = vec;
		ptr = &a[0];
	}
}

template<class Tp>
void doAssignMove()
{
	Tp vec(1000, 1), a;
	for(std::size_t i=0; i!=1000000; ++i) {
		a = std::move(vec);
		ptr = &a[0];
		vec = std::move(a);
		ptr = &vec[0];
	}
}

template<class Tp>
void doPushback()
{
	for(size_t iter=0; iter!=100000; ++iter)
	{
		Tp vec;
		for(std::size_t i=0; i!=1000; ++i)
			vec.push_back(i);
		ptr = &vec[0];
	}
}

template<class Tp>
void doInsert()
{
	for(size_t iter=0; iter!=10000; ++iter)
	{
		Tp vec;
		for(std::size_t i=0; i!=100; ++i)
			vec.insert(vec.begin() + vec.size()/2, i);
		ptr = &vec[0];
	}
}

template<class Tp>
void doInsertInitialized()
{
	for(size_t iter=0; iter!=10000; ++iter)
	{
		Tp vec;
		for(std::size_t i=0; i!=100; ++i)
			vec.insert(vec.begin() + vec.size()/2, 100, typename Tp::value_type());
		ptr = &vec[0];
	}
}

template<class Tp>
void doInsertUninitialized()
{
	for(size_t iter=0; iter!=10000; ++iter)
	{
		Tp vec;
		for(std::size_t i=0; i!=100; ++i)
			vec.insert_uninitialized(vec.begin() + vec.size()/2, 100);
		ptr = &vec[0];
	}
}

template<class Tp>
void doErase()
{
	for(size_t iter=0; iter!=10000; ++iter)
	{
		Tp vec(1000, 0);
		for(std::size_t i=0; i!=1000; ++i)
			vec.erase(vec.begin() + vec.size()/2);
		ptr = &vec[0];
	}
}

template<class Tp>
void doIterate()
{
	Tp vec(1000, 1);
	for(size_t iter=0; iter!=100000; ++iter)
	{
		volatile size_t sum = 0;
		for(typename Tp::const_iterator i=vec.begin(); i!=vec.end(); ++i)
			sum += *i;
	}
}

template<class Tp>
void doEquality()
{
	Tp vecA(1000, 1), vecB(1000, 1);
	for(size_t iter=0; iter!=100000; ++iter)
	{
		volatile bool x = (vecA == vecB);
	}
}

template<class Tp>
void doSmallerThan()
{
	Tp vecA(1000, 1), vecB(1000, 1);
	for(size_t iter=0; iter!=100000; ++iter)
	{
		volatile bool x = (vecA < vecB);
	}
}

template<class Tp>
void doSmallerEqualThan()
{
	Tp vecA(1000, 1), vecB(1000, 1);
	for(size_t iter=0; iter!=100000; ++iter)
	{
		volatile bool x = (vecA <= vecB);
	}
}

template<void Func()>
double test()
{
	Stopwatch watch(true);
	Func();
	return watch.Seconds();
}

struct Measurement
{
	const char *name;
	double uvecMean, uvecStdDev;
	double stdvecMean, stdvecStdDev;
};

template<void UvecFunc(), void StdvecFunc()>
void measure(struct Measurement& m, const char* name)
{
	m.name = name;
	const size_t NITERATIONS = 5;
	double
		uvecTimeSum = 0.0, uvecTimeSumSq = 0.0,
		stdvecTimeSum = 0.0, stdvecTimeSumSq = 0.0;
	std::cout << "uvector vs std::vector (" << m.name << ")\n";
	(std::cout << std::fixed).precision(0);
	for(size_t i=0; i!=NITERATIONS; ++i)
	{
		double
			uvecTime = test<UvecFunc>()*1000.0,
			stdvecTime = test<StdvecFunc>()*1000.0;
		std::cout << uvecTime << '\t' << stdvecTime << '\n';
		uvecTimeSum += uvecTime;
		uvecTimeSumSq += uvecTime * uvecTime;
		stdvecTimeSum += stdvecTime;
		stdvecTimeSumSq += stdvecTime * stdvecTime;
	}
	m.uvecMean = uvecTimeSum/NITERATIONS,
	m.uvecStdDev = sqrt(uvecTimeSumSq / NITERATIONS - m.uvecMean*m.uvecMean),
	m.stdvecMean = stdvecTimeSum/NITERATIONS,
	m.stdvecStdDev = sqrt(stdvecTimeSumSq / NITERATIONS - m.stdvecMean*m.stdvecMean);
	std::cout.precision(2);
	std::cout << "Results:\n" << std::fixed <<
		"  uvector: " << m.uvecMean   << " ± " << m.uvecStdDev   << " ms\n"
		"stdvector: " << m.stdvecMean << " ± " << m.stdvecStdDev << " ms\n";
}

void reportMeasurement(const Measurement& m)
{
	std::cout.precision(2);
	std::cout << std::fixed
		<< m.uvecMean   << " ± " << m.uvecStdDev << '\t'
		<< m.stdvecMean << " ± " << m.stdvecStdDev << '\t'
		<< round((m.uvecMean / m.stdvecMean) * 1000.0)/10.0 << "%\t"
		<< m.name << '\n';
}

int main(int argc, char* argv[])
{
	uvector<Measurement> measurements(13);
	measure<doConstruct<uvector<int>>, doConstruct<std::vector<int>>>(measurements[0], "constructor");
	measure<doConstructFill<uvector<int>>, doConstructFill<std::vector<int>>>(measurements[1], "constructor fill");
	measure<doConstruct<uvector<int>>, doCopyConstruct<std::vector<int>>>(measurements[2], "copy constructor");
	measure<doAssignCopy<uvector<int>>, doAssignCopy<std::vector<int>>>(measurements[3], "assign copy");
	measure<doAssignMove<uvector<int>>, doAssignMove<std::vector<int>>>(measurements[4], "assign move");
	measure<doPushback<uvector<int>>, doPushback<std::vector<int>>>(measurements[5], "push_back");
	measure<doInsert<uvector<int>>, doInsert<std::vector<int>>>(measurements[6], "insert");
	measure<doInsertUninitialized<uvector<int>>, doInsertInitialized<std::vector<int>>>(measurements[7], "insert_uninitialized");
	measure<doErase<uvector<int>>, doErase<std::vector<int>>>(measurements[8], "erase");
	measure<doIterate<uvector<int>>, doIterate<std::vector<int>>>(measurements[9], "iterate");
	measure<doEquality<uvector<int>>, doEquality<std::vector<int>>>(measurements[10], "equality");
	measure<doSmallerThan<uvector<int>>, doSmallerThan<std::vector<int>>>(measurements[11], "smaller than");
	measure<doSmallerEqualThan<uvector<int>>, doSmallerEqualThan<std::vector<int>>>(measurements[12], "smaller or equal than");
	
	for(uvector<Measurement>::const_iterator m=measurements.begin(); m!=measurements.end(); ++m)
		reportMeasurement(*m);
	
	std::cout << "\n\nReverse:\n";
	for(uvector<Measurement>::const_reverse_iterator m=measurements.rbegin(); m!=measurements.rend(); ++m)
		reportMeasurement(*m);
}
