#include <iostream>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <stdexcept>
#include <limits>

#include <gsl/gsl_vector.h>
#include <gsl/gsl_multifit_nlin.h>

bool fixDW = false, fixConstant = false;

double sinZA(double za)
{
	return (2900.0*sin(za*(M_PI/180.0)) + 5.5*cos(za*(M_PI/180.0)));
}

double fovFact(double fov)
{
	double fovRad = 0.5 * fov * (M_PI/180.0); // this is the max l
	double fov2 = fovRad * fovRad;
	return fov2 < 1.0 ? (1.0 - sqrt(1.0 - fov2)) : 1.0;
}

struct TimingRecord {
	double average, stddev;
	size_t n;
	double sum, sumSq;
	
	void Recalculate()
	{
		average = sum / double(n);
		double sumMeanSquared = sum * sum / n;
		stddev = sqrt((sumSq - sumMeanSquared) / n);
	}
	
	std::string ToLine(const std::string& description, const double val)
	{
		std::ostringstream newLine;
		newLine.precision(11);
		newLine << description << '\t' << val
			<< '\t' << average << '\t' << stddev
			<< '\t' << n << '\t' << sum << '\t' << sumSq;
		return newLine.str();
	}
};

bool parseLine(const std::string& line, std::string& description, double& value, TimingRecord& record)
{
	std::string tokens[7];
	size_t curPos = 0;
	for(size_t i=0; i!=6; ++i)
	{
		size_t delimitor = line.find('\t', curPos);
		if(delimitor == std::string::npos)
			return false;
		tokens[i] = line.substr(curPos, delimitor - curPos);
		curPos = delimitor+1;
	}
	tokens[6] = line.substr(curPos);
	
	description = tokens[0];
	value = atof(tokens[1].c_str());
	record.average = atof(tokens[2].c_str());
	record.stddev = atof(tokens[3].c_str());
	record.n = atof(tokens[4].c_str());
	record.sum = atof(tokens[5].c_str());
	record.sumSq = atof(tokens[6].c_str());
	if(!std::isfinite(record.stddev))
		record.stddev = 0.0;
	return true;
}

enum FittingFunc
{
	NFunc,
	NSqLogNFunc2,
	NSqLogNFunc3,
	NSqLogNFunc4,
	NZAFunc,
	NZASqFunc2,
	NZASqFunc3,
	WSCleanFunc,
	WSSCleanFunc,
	CASAFunc
};

struct Sample
{
	double time, weight, za, nPix, nVis, fov, nChan;
	
	bool sameConfiguration(const Sample& s) const { return s.za==za && s.nPix==nPix && s.nVis==nVis && s.fov==fov && s.nChan==nChan; }
};

struct FittingInfo
{
	FittingFunc func;
	size_t n;
	double *xs, *ys, *ws;
	Sample *samples;
};

double eval(FittingFunc func, double za, double nVis, double nPix, double fov, double nChan, double a, double b, double c, double d, double e, double f)
{
	if(func == WSCleanFunc) {
		double we = sinZA(za) * fovFact(fov);
		return nChan * (a * (we+b)*nPix*nPix*log2(nPix) + c * nVis/nChan) + d;
	}
	else if(func == WSSCleanFunc) {
		return nChan * (a *nPix*nPix*log2(nPix) + b * nVis/nChan) + c;
	}
	else {
		double we = (sinZA(za) * fovFact(fov) + d);
		return nChan * (a * nPix*nPix*log2(nPix) + b * nVis/nChan * we * we) + f;
	}
}

double eval(FittingFunc func, double za, double nVis, double nPix, double fov, double nChan)
{
	switch(func)
	{
		case WSCleanFunc:
			return eval(func, za, nVis, nPix, fov, nChan, 0.00013703, 0.000432297, 0.241851, 0.0, 0.0, 0.0);
		case CASAFunc:
			return eval(func, za, nVis, nPix, fov, nChan, 1.97854e-07, 47825.5, 0.0, 0.000473665, 0.0, 235.922);
		default:
			return 0.0;
	}
}

double evalWE(FittingFunc func, double we, double nVis, double nPix, double nChan, double a, double b, double c, double d, double e, double f)
{
	if(func == WSCleanFunc) {
		return nChan * (a * (we+b)*nPix*nPix*log2(nPix) + c * nVis/nChan) + d;
	}
	else if(func == WSSCleanFunc) {
		return nChan * (a *nPix*nPix*log2(nPix) + b * nVis/nChan) + c;
	}
	else {
		we = we + d;
		return nChan * (a * nPix*nPix*log2(nPix) + b * nVis/nChan * we * we) + f;
	}
}

int fitFunc(const gsl_vector *xvec, void *data, gsl_vector *f)
{
	const FittingInfo &fittingInfo = *reinterpret_cast<FittingInfo*>(data);
	const double
		a = gsl_vector_get(xvec, 0),
		b = gsl_vector_get(xvec, 1),
		c = gsl_vector_get(xvec, 2),
		d = gsl_vector_get(xvec, 3);
	
	for(size_t i=0; i!=fittingInfo.n; ++i)
	{
		double value, y, w;
		if(fittingInfo.func == WSCleanFunc || fittingInfo.func == WSSCleanFunc || fittingInfo.func == CASAFunc)
		{
			const double e = gsl_vector_get(xvec, 4), f = gsl_vector_get(xvec, 5);
			y = fittingInfo.samples[i].time;
			w = fittingInfo.samples[i].weight;
			const double
				za = fittingInfo.samples[i].za,
				nVis = fittingInfo.samples[i].nVis,
				nPix = fittingInfo.samples[i].nPix,
				fov = fittingInfo.samples[i].fov,
				nChan = fittingInfo.samples[i].nChan;
			value = eval(fittingInfo.func, za, nVis, nPix, fov, nChan, a, b, c, d, e, f);
		}
		else {
			double x = fittingInfo.xs[i], za = x*(M_PI/180.0);
			y = fittingInfo.ys[i], w = fittingInfo.ws[i];
			switch(fittingInfo.func)
			{
				case NFunc:
					value = c*x + d;
					break;
				case NSqLogNFunc2:
					value = a*x*x*log2(x) + d;
					break;
				case NSqLogNFunc3:
					value = a*x*x*log2(x) + b*x*x + d;
					break;
				case NSqLogNFunc4:
					value = a*x*x*log2(x) + b*x*x + c*x + d;
					break;
				case NZAFunc:
					value = (a / 2900.0) * (2900.0*sin(za) + 5.5*cos(za)) + b;
					break;
				case NZASqFunc2:
					value = (2900.0*sin(za) + 5.5*cos(za)) / 2900.0;
					value = a * value * value + c;
					break;
				case NZASqFunc3:
					value = (2900.0*sin(za) + 5.5*cos(za)) / 2900.0;
					value = a * value * value + b * value + c;
					break;
				default:
					value = 0.0;
					break;
			}
		}
		gsl_vector_set(f, i, (value - y) * w);
	}
		
	return GSL_SUCCESS;
}

int fitFuncDeriv(const gsl_vector *xvec, void *data, gsl_matrix *J)
{
	const FittingInfo &fittingInfo = *reinterpret_cast<FittingInfo*>(data);
	const double
		a = gsl_vector_get(xvec, 0),
		b = gsl_vector_get(xvec, 1),
		c = gsl_vector_get(xvec, 2),
		d = gsl_vector_get(xvec, 3);
	
	for(size_t i=0; i!=fittingInfo.n; ++i)
	{
		double da = 0.0, db = 0.0, dc = 0.0, dd = 0.0, de = 0.0, df = 0.0;
		double value, y, w;
		if(fittingInfo.func == WSCleanFunc || fittingInfo.func == CASAFunc || fittingInfo.func == WSSCleanFunc)
		{
			y = fittingInfo.samples[i].time;
			w = fittingInfo.samples[i].weight;
			const double
				za = fittingInfo.samples[i].za,
				nVis = fittingInfo.samples[i].nVis,
				nPix = fittingInfo.samples[i].nPix,
				fov = fittingInfo.samples[i].fov,
				nChan = fittingInfo.samples[i].nChan;
			
			if(fittingInfo.func == WSCleanFunc)
			{ // nChan * (a * (we+b)*nPix*nPix*log2(nPix) + c * nVis) + d;
				double we = sinZA(za) * fovFact(fov);
				da = nChan * (we+b)*nPix*nPix*log2(nPix);
				db = fixDW ? 0.0 : nChan * a*nPix*nPix*log2(nPix);
				dc = nVis;
				dd = fixConstant ? 0.0 : 1.0;
			}
			else if(fittingInfo.func == WSSCleanFunc)
			{ // nChan * (a *nPix*nPix*log2(nPix) + b * nVis/nChan) + c;
				da = nChan * nPix*nPix*log2(nPix);
				db = nVis;
				dc = fixConstant ? 0.0 : 1.0;
			}
			else { // nChan * (a * nPix*nPix*log2(nPix) + b * nVis/nChan * we * we) + f;
				double we = sinZA(za) * fovFact(fov) + d;
				da = nChan * nPix*nPix*log2(nPix);
				db = nChan * 2.0 * we * we;
				dc = 0.0;
				dd = fixDW ? 0.0 : nChan * b * 2.0 * (d + sinZA(za) * fovFact(fov));
				de = 0.0;
				df = fixConstant ? 0.0 : 1.0;
			}
			gsl_matrix_set(J, i, 4, de*w);
			gsl_matrix_set(J, i, 5, df*w);
		}
		else {
			double x = fittingInfo.xs[i], za = x*(M_PI/180.0);
			y = fittingInfo.ys[i], w = fittingInfo.ws[i];
			switch(fittingInfo.func)
			{
				case NFunc:
					dc = x;
					dd = 1.0;
					break;
				case NSqLogNFunc2:
					da = x*x*log2(x);
					dd = 1.0;
					break;
				case NSqLogNFunc3:
					da = x*x*log2(x);
					db = x*x;
					dd = 1.0;
					break;
				case NSqLogNFunc4:
					da = x*x*log2(x);
					db = x*x;
					dc = x;
					dd = 1.0;
					break;
				case NZAFunc:
					da = (2900.0*sin(za) + 5.5*cos(za)) / 2900.0;
					db = 1.0;
					break;
				case NZASqFunc2:
					da = (2900.0*sin(za) + 5.5*cos(za)) / 2900.0;
					da = da*da;
					dc = 1.0;
					break;
				case NZASqFunc3:
					db = (2900.0*sin(za) + 5.5*cos(za)) / 2900.0;
					da = db*db;
					dc = 1.0;
					break;
				default:
					break;
			}
		}
		gsl_matrix_set(J, i, 0, da*w);
		gsl_matrix_set(J, i, 1, db*w);
		gsl_matrix_set(J, i, 2, dc*w);
		gsl_matrix_set(J, i, 3, dd*w);
	}
	return GSL_SUCCESS;
}

int fitFuncBoth(const gsl_vector *x, void *data, gsl_vector *f, gsl_matrix *J)
{
	fitFunc(x, data, f);
	fitFuncDeriv(x, data, J);
	return GSL_SUCCESS;
}

void fit(FittingInfo& info, double& a, double& b, double& c, double& d, double& e, double& f)
{
	size_t p = 6;
	if(info.n < 6)
		p = 4;
	const gsl_multifit_fdfsolver_type *T = gsl_multifit_fdfsolver_lmsder;
	gsl_multifit_fdfsolver *solver = gsl_multifit_fdfsolver_alloc(T, info.n, p);
	gsl_multifit_function_fdf fdf;
	fdf.f = &fitFunc;
	fdf.df = &fitFuncDeriv;
	fdf.fdf = &fitFuncBoth;
	fdf.n = info.n;
	fdf.p = p;
	fdf.params = &info;

	gsl_vector_view initialVals;
	double initialValsArray[5];
	initialValsArray[0] = a;
	initialValsArray[1] = b;
	initialValsArray[2] = c;
	initialValsArray[3] = d;
	if(p == 6)
	{
		initialValsArray[4] = e;
		initialValsArray[5] = f;
		initialVals = gsl_vector_view_array (initialValsArray, 6);
	}
	else {
		initialVals = gsl_vector_view_array (initialValsArray, 4);
	}
	gsl_multifit_fdfsolver_set (solver, &fdf, &initialVals.vector);

	int status;
	size_t iter = 0;
	do {
		iter++;
		status = gsl_multifit_fdfsolver_iterate(solver);
		
		if(status)
			break;
		
		status = gsl_multifit_test_delta(solver->dx, solver->x, 1e-9, 1e-9);
		
	} while (status == GSL_CONTINUE && iter < 1000);
	std::cout << gsl_strerror(status) << " after " << iter << " iteratons.\n";
	
	a = gsl_vector_get (solver->x, 0);
	b = gsl_vector_get (solver->x, 1);
	c = gsl_vector_get (solver->x, 2);
	d = gsl_vector_get (solver->x, 3);
	if(p == 6) {
		e = gsl_vector_get (solver->x, 4);
		f = gsl_vector_get (solver->x, 5);
	}
	gsl_multifit_fdfsolver_free(solver);
}

void fit(FittingInfo& info, double& a, double& b, double& c, double& d)
{
	double e = 0.0, f = 0.0;
	fit(info, a, b, c, d, e, f);
}

void readFile(std::vector<std::pair<double, TimingRecord>>& valueArray, const std::string& filename)
{
	std::cout << "Reading " << filename << ".\n";
	std::ifstream file(filename);
	std::string line;
	if(file.bad())
		throw std::runtime_error("Could not read file " + filename);
	while(file.good())
	{
		std::getline(file, line);
		std::string desc;
		double val;
		TimingRecord time;
		if(parseLine(line, desc, val, time))
		{
			valueArray.push_back(std::make_pair(val, time));
		}
	}
}

void addSample(std::vector<Sample>& samples, const Sample& sample)
{
	if(!std::isfinite(sample.weight) || sample.weight==0.0)
		std::cout << "Removing sample with 0 stddev\n";
	else {
		samples.push_back(sample);
		samples.back().nPix *= 0.001;
	}
}

void readNVisFile(std::vector<Sample>& samples, const std::string& filename, double za, double nPix, double fov, double nChan, double w)
{
	std::vector<std::pair<double, TimingRecord>> valueArray;
	readFile(valueArray, filename);
	
	size_t n = valueArray.size();
	for(size_t i=0; i!=n; ++i)
	{
		Sample sample;
		sample.za = za;
		sample.nPix = nPix;
		sample.nVis = valueArray[i].first;
		sample.fov = fov;
		sample.nChan = nChan;
		const TimingRecord& record = valueArray[i].second;
		sample.time = record.average;
		sample.weight = w / record.stddev;
		addSample(samples, sample);
	}
}

void readNPixFile(std::vector<Sample>& samples, const std::string& filename, double za, double nVis, double fov, double nChan, double w)
{
	std::vector<std::pair<double, TimingRecord>> valueArray;
	readFile(valueArray, filename);
	
	size_t n = valueArray.size();
	for(size_t i=0; i!=n; ++i)
	{
		Sample sample;
		sample.za = za;
		sample.nPix = valueArray[i].first;
		sample.nVis = nVis;
		sample.fov = fov;
		sample.nChan = nChan;
		const TimingRecord& record = valueArray[i].second;
		sample.time = record.average;
		sample.weight = w / record.stddev;
		addSample(samples, sample);
	}
}

void readZAFile(std::vector<Sample>& samples, const std::string& filename, double nPix, double nVis, double fov, double nChan, double w)
{
	std::vector<std::pair<double, TimingRecord>> valueArray;
	readFile(valueArray, filename);
	
	size_t n = valueArray.size();
	for(size_t i=0; i!=n; ++i)
	{
		Sample sample;
		sample.za = valueArray[i].first;
		sample.nPix = nPix;
		sample.nVis = nVis;
		sample.fov = fov;
		sample.nChan = nChan;
		const TimingRecord& record = valueArray[i].second;
		sample.time = record.average;
		sample.weight = w / record.stddev;
		addSample(samples, sample);
	}
}

void readFOVFile(std::vector<Sample>& samples, const std::string& filename, double za, double nPix, double nVis, double nChan, double w)
{
	std::vector<std::pair<double, TimingRecord>> valueArray;
	readFile(valueArray, filename);
	
	size_t n = valueArray.size();
	for(size_t i=0; i!=n; ++i)
	{
		Sample sample;
		sample.za = za;
		sample.nPix = nPix;
		sample.nVis = nVis;
		sample.fov = valueArray[i].first;
		sample.nChan = nChan;
		const TimingRecord& record = valueArray[i].second;
		sample.time = record.average;
		sample.weight = w / record.stddev;
		addSample(samples, sample);
	}
}

void readNChanFile(std::vector<Sample>& samples, const std::string& filename, double za, double nPix, double nVis, double fov, double w)
{
	std::vector<std::pair<double, TimingRecord>> valueArray;
	readFile(valueArray, filename);
	
	size_t n = valueArray.size();
	for(size_t i=0; i!=n; ++i)
	{
		Sample sample;
		sample.za = za;
		sample.nPix = nPix;
		sample.nVis = nVis;
		sample.fov = fov;
		sample.nChan = valueArray[i].first;
		const TimingRecord& record = valueArray[i].second;
		sample.time = record.average;
		sample.weight = w / record.stddev;
		addSample(samples, sample);
	}
}

void writeNVisFit(const std::string& filename, FittingFunc func, double a, double b, double c, double d, double e, double f, double za, double nPix, double fov, double nChan)
{
	std::ofstream file(filename);
	double xStart = 25, xEnd = 1400.0;
	double nVis=xStart / 1.01;
	while(nVis < xEnd)
	{
		nVis *= 1.01;
		file << nVis << '\t' << eval(func, za, nVis, nPix, fov, nChan, a, b, c, d, e, f) << '\n';
	}
}

void writeNPixFit(const std::string& filename, FittingFunc func, double a, double b, double c, double d, double e, double f, double za, double nVis, double fov, double nChan)
{
	std::ofstream file(filename);
	double xStart = 1024, xEnd = 12800.0;
	double nPix=xStart / 1.01;
	while(nPix < xEnd)
	{
		nPix *= 1.01;
		file << nPix << '\t' << eval(func, za, nVis, nPix/1000.0, fov, nChan, a, b, c, d, e, f) << '\n';
	}
}

void writeZAFit(const std::string& filename, FittingFunc func, double a, double b, double c, double d, double e, double f, double nPix, double nVis, double fov, double nChan)
{
	std::ofstream file(filename);
	double zaStart = 0.0, zaEnd = 90.0;
	double za=zaStart - 0.1;
	while(za < zaEnd)
	{
		za += 0.1;
		file << za << '\t' << eval(func, za, nVis, nPix, fov, nChan, a, b, c, d, e, f) << '\n';
	}
}

void writeFOVFit(const std::string& filename, FittingFunc func, double a, double b, double c, double d, double e, double f, double za, double nPix, double nVis, double nChan)
{
	std::ofstream file(filename);
	double fovStart = 4.608, fovEnd = 147.456;
	double fov=fovStart - 0.1;
	while(fov < fovEnd)
	{
		fov += 0.1;
		file << fov << '\t' << eval(func, za, nVis, nPix, fov, nChan, a, b, c, d, e, f) << '\n';
	}
}

void writeNChanFit(const std::string& filename, FittingFunc func, double a, double b, double c, double d, double e, double f, double za, double nPix, double nVis, double fov)
{
	std::ofstream file(filename);
	double nChanStart = 1, nChanEnd = 768;
	double nChan=nChanStart - 0.1;
	while(nChan < nChanEnd)
	{
		nChan += 0.1;
		file << nChan << '\t' << eval(func, za, nVis, nPix, fov, nChan, a, b, c, d, e, f) << '\n';
	}
}

double stddev(double sum, double sumSq, double sumweight)
{
	double wAverage = sum / sumweight;
	double sumMeanSquared = sum * sum / sumweight;
	return sqrt((sumSq - sumMeanSquared) / sumweight);
}

void calculateStdError(std::vector<Sample>& samples, FittingFunc func, double a, double b, double c, double d, double e, double f)
{
	double weights = 0.0, weightedSum = 0.0, unweightedSum = 0.0, weightedSumSq = 0.0, unweightedSumSq = 0.0;
	for(size_t i=0; i!=samples.size(); ++i)
	{
		const Sample& s = samples[i];
		double y = eval(func, s.za, s.nVis, s.nPix, s.fov, s.nChan, a, b, c, d, e, f);
		double dy = y - s.time, dwy = dy * s.weight;
		weights += s.weight;
		weightedSum += dwy;
		weightedSumSq += dwy*dwy;
		unweightedSum += dy;
		unweightedSumSq += dy*dy;
	}
	std::cout << "Mean error: " << weightedSum/weights << "  standard error: " << stddev(weightedSum, weightedSumSq, weights) << '\n'
		<< "Unweighted mean: " << unweightedSum/samples.size() << " unweighted stderr: " << stddev(unweightedSum, unweightedSumSq, samples.size()) << '\n';
}

void printDuplicates(const std::vector<Sample>& samples)
{
	for(std::vector<Sample>::const_iterator s1=samples.begin(); s1!=samples.end(); ++s1)
	{
		for(std::vector<Sample>::const_iterator s2=s1+1; s2!=samples.end(); ++s2)
		{
			if(s1->sameConfiguration(*s2))
				std::cout << "Duplicate configuration: " << s1->za << ", " << s1->nVis << ", " << s1->nPix << ", " << s1->fov << ", " << s1->nChan <<
					"\n Timings: " << s1->time << " vs " << s2->time << '\n';
		}
	}
}

void optimalDeltaW(FittingFunc func, double a, double b, double c, double d, double e, double f, double za, double nVis, double nPix, double fov, double nChan, bool zeroConstant, double& dw, double& casaGain, double& wscleanGain)
{
	double we = sinZA(za) * fovFact(fov);
	// Find min dw such that t: nVis=nVis*dw/we, we=dw
	
	double minY = std::numeric_limits<double>::max(), minDW = 0.0;
	double i = 1;
	double fz = zeroConstant ? 0.0 : f;
	while(i < 100.0)
	{
		double dw = we / double(i);
		double y = double(i) * evalWE(func, dw, nVis / double(i), nPix, nChan, a, b, c, d, e, fz);
		if(y < minY) {
			minDW = dw;
			minY = y;
		}
		i *= 1.1;
	}
	
	if(minDW == we)
		dw = std::numeric_limits<double>::quiet_NaN();
	else
		dw = minDW;
	casaGain = eval(CASAFunc, za, nVis, nPix, fov, nChan)/minY;
	wscleanGain = eval(WSCleanFunc, za, nVis, nPix, fov, nChan)/minY;
}

void optimalDeltaW(const std::vector<Sample>& samples, FittingFunc func, double a, double b, double c, double d, double e, double f)
{
	double maxDW = 0.0, minDW = std::numeric_limits<double>::max(), sumDW = 0.0;
	size_t count = 0;
	double minCG, minWSCG, maxCG, maxWSCG;
	for(std::vector<Sample>::const_iterator s=samples.begin(); s!=samples.end(); ++s)
	{
		double dw, cg, wscg;
		optimalDeltaW(func, a, b, c, d, e, f, s->za, s->nVis, s->nPix, s->fov, s->nChan, false, dw, cg, wscg);
		if(std::isfinite(dw))
		{
			if(dw < minDW) { minDW = dw; minCG = cg; minWSCG = wscg; }
			if(dw > maxDW) { maxDW = dw; maxCG = cg; maxWSCG = wscg; }
			sumDW += dw;
			count ++;
		}
		optimalDeltaW(func, a, b, c, d, e, f, s->za, s->nVis, s->nPix, s->fov, s->nChan, true, dw, cg, wscg);
		if(std::isfinite(dw))
		{
			if(dw < minDW) { minDW = dw; minCG = cg; minWSCG = wscg; }
			if(dw > maxDW) { maxDW = dw; maxCG = cg; maxWSCG = wscg; }
			sumDW += dw;
			count ++;
		}
	}
	std::cout << "Min dw=" << minDW << " (CASA gain: " << minCG << ", WSClean gain: " << minWSCG << ")";
	std::cout << ", max dw=" << maxDW << " (CASA gain: " << maxCG << ", WSClean gain: " << maxWSCG << ")";
	std::cout << ", mean dw=" << sumDW/count << '\n';
}

void optimalDeltaT(FittingFunc func, double a, double b, double c, double d, double e, double f, double za, double nVis, double nPix, double fov, double nChan, bool zeroConstant, double& dt)
{
	double minY = std::numeric_limits<double>::max(), minDT = 0.0;
	double dtTrial = 1.0;
	double fz = zeroConstant ? 0.0 : f;
	while(dtTrial < 300000.0)
	{
		double trialFOV = fov + 2.0 * (dtTrial - 112.0) * (360.0 / 60.0 / 60.0 / 24.0);
		if(trialFOV < 0.0) trialFOV = 0.0;
		double y = (112.0/dtTrial) * eval(func, za, nVis * dtTrial/112.0, nPix, trialFOV, nChan, a, b, c, d, e, fz);
		//std::cout << "za=" << trialFOV<< ", dt=" << dtTrial << ", y=" << y << '\n';
		if(y < minY) {
			minDT = dtTrial;
			minY = y;
		}
		dtTrial *= 1.01;
	}
	dt = minDT;
	double tNormal = eval(func, za, nVis, nPix, fov, nChan, a, b, c, d, e, fz);
	std::cout << "Optimal snapshot time: " << minDT << " (time=" << minY << " vs " << tNormal << ")\n";
}

void fitAllWSClean()
{
	std::vector<Sample> samples;
	readNVisFile(samples, "benchmark-nsamples/timings-zenith-nsamples-wsc.txt", 0.0, 3072, 36.864, 1, 2.0);
	readNVisFile(samples, "benchmark-nsamples/timings-ZA010-nsamples-wsc.txt", 10.0, 3072, 36.864, 1, 2.0);
	readNPixFile(samples, "benchmark-resolution/timings-zenith-resolution-wsclean.txt", 0.0, 349.6, 36.864, 1, 2.0);
	readZAFile(samples, "benchmark-zenith-angle/timings-za3072-wsclean.txt", 3072, 349.6, 36.864, 1, 1.0);
	readZAFile(samples, "benchmark-zenith-angle/timings-za2048-wsclean.txt", 2048, 349.6, 24.576, 1, 1.0);
	readFOVFile(samples, "benchmark-fov/timings-zenith-fov-wsc.txt", 0.0, 3072, 349.6, 1, 1.0);
	readNChanFile(samples, "benchmark-channels/timings-channels-wsc.txt", 0.0, 3072, 349.6, 24.576, 1.0);
	std::cout << "Read " << samples.size() << " values.\n";
	printDuplicates(samples);
	FittingInfo info;
	info.n = samples.size();
	info.samples = samples.data();
	info.func = WSCleanFunc;
	fixConstant = true;
	double a = 1.0, b = 0.0, c = 1.0, d = 0.0, e = 0.0, f = 0.0;
	fit(info, a, b, c, d, e, f);
	std::cout << "Nchan (" << a << " Npix^2 log Npix ((1-sqrt(1-tan(0.5fov)^2)) sin ZA + " << b << ") + " << c << " Nvis) + " << d << "\n";
	writeNVisFit("benchmark-nsamples/fit-zenith-wsc.txt", info.func, a, b, c, d, e, f, 0.0, 3.072, 36.864, 1);
	writeNVisFit("benchmark-nsamples/fit-ZA010-wsc.txt", info.func, a, b, c, d, e, f, 10.0, 3.072, 36.864, 1);
	writeNPixFit("benchmark-resolution/fit-wsclean.txt", info.func, a, b, c, d, e, f, 0.0, 349.6, 36.864, 1);
	writeZAFit("benchmark-zenith-angle/fit-za3072-wsclean.txt", info.func, a, b, c, d, e, f, 3.072, 349.6, 36.864, 1);
	writeZAFit("benchmark-zenith-angle/fit-za2048-wsclean.txt", info.func, a, b, c, d, e, f, 2.048, 349.6, 24.576, 1);
	writeFOVFit("benchmark-fov/fit-zenith-fov-wsc.txt", info.func, a, b, c, d, e, f, 0.0, 3.072, 349.6, 1);
	writeFOVFit("benchmark-fov/fit-ZA010-fov-wsc.txt", info.func, a, b, c, d, e, f, 10.0, 3.072, 349.6, 1);
	writeNChanFit("benchmark-channels/fit-zenith-wsc.txt", info.func, a, b, c, d, e, f, 0.0, 3.072, 349.6, 36.864);
	calculateStdError(samples, info.func, a, b, c, d, e, f);
	double dt;
	optimalDeltaT(info.func, a, b, c, d, e, f, 10.0, 349.6, 3.072, 36.864, 1, false, dt);
	optimalDeltaT(info.func, a, b, c, d, e, f, 0.0, 349.6, 3.072, 36.864, 1, false, dt);
}

void fitAllWSSClean()
{
	const double CHGCENTRE_RUNTIME = 132.6; //seconds for Nvis = 349.6
	
	std::vector<Sample> samples;
	//readNVisFile(samples, "benchmark-nsamples/timings-ZA010-nsamples-wsc.txt", 10.0, 3072, 36.864, 1, 1.0);
	readNPixFile(samples, "benchmark-resolution/timings-zenith-resolution-wsclean.txt", 0.0, 349.6, 36.864, 1, 1.0);
	readZAFile(samples, "benchmark-zenith-angle/timings-za3072-wssclean.txt", 3072, 349.6, 36.864, 1, 1.0);
	readZAFile(samples, "benchmark-zenith-angle/timings-za2048-wssclean.txt", 2048, 349.6, 24.576, 1, 1.0);
	readFOVFile(samples, "benchmark-fov/timings-zenith-fov-wssc.txt", 0.0, 3072, 349.6, 1, 1.0);
	readFOVFile(samples, "benchmark-fov/timings-ZA010-fov-wssc.txt", 10.0, 3072, 349.6, 1, 1.0);
	readNChanFile(samples, "benchmark-channels/timings-channels-wssc.txt", 0.0, 3072, 349.6, 24.576, 1.0);
	std::cout << "Read " << samples.size() << " values.\n";
	printDuplicates(samples);
	for(std::vector<Sample>::iterator i=samples.begin(); i!=samples.end(); ++i)
		i->time += CHGCENTRE_RUNTIME * i->nVis / 349.6;
	FittingInfo info;
	info.n = samples.size();
	info.samples = samples.data();
	info.func = WSSCleanFunc;
	double a = 1.0, b = 0.0, c = 1.0, d = 0.0, e = 0.0, f = 0.0;
	fit(info, a, b, c, d, e, f);
	std::cout << "Nchan (" << a << " Npix^2 log Npix + " << b << " Nvis) + " << c << "\n";
	writeNVisFit("benchmark-nsamples/fit-zenith-wssc.txt", info.func, a, b, c, d, e, f, 0.0, 3.072, 36.864, 1);
	writeNVisFit("benchmark-nsamples/fit-ZA010-wssc.txt", info.func, a, b, c, d, e, f, 10.0, 3.072, 36.864, 1);
	writeNPixFit("benchmark-resolution/fit-wssclean.txt", info.func, a, b, c, d, e, f, 0.0, 349.6, 36.864, 1);
	writeZAFit("benchmark-zenith-angle/fit-za3072-wssclean.txt", info.func, a, b, c, d, e, f, 3.072, 349.6, 36.864, 1);
	writeZAFit("benchmark-zenith-angle/fit-za2048-wssclean.txt", info.func, a, b, c, d, e, f, 2.048, 349.6, 24.576, 1);
	writeFOVFit("benchmark-fov/fit-zenith-fov-wssc.txt", info.func, a, b, c, d, e, f, 0.0, 3.072, 349.6, 1);
	writeFOVFit("benchmark-fov/fit-ZA010-fov-wssc.txt", info.func, a, b, c, d, e, f, 10.0, 3.072, 349.6, 1);
	writeNChanFit("benchmark-channels/fit-zenith-wssc.txt", info.func, a, b, c, d, e, f, 0.0, 3.072, 349.6, 36.864);
	calculateStdError(samples, info.func, a, b, c, d, e, f);
}

void fitAllCASA()
{
	std::vector<Sample> samples;
	readNVisFile(samples, "benchmark-nsamples/timings-zenith-nsamples-casa.txt", 0.0, 3072, 36.864, 1, 1.0);
	readNVisFile(samples, "benchmark-nsamples/timings-ZA010-nsamples-casa.txt", 10.0, 3072, 36.864, 1, 1.0);
	readNPixFile(samples, "benchmark-resolution/timings-zenith-resolution-casa-no-outlier.txt", 0.0, 349.6, 36.864, 1, 1.0);
	readZAFile(samples, "benchmark-zenith-angle/timings-za3072-casa.txt", 3072, 349.6, 36.864, 1, 0.2);
	readZAFile(samples, "benchmark-zenith-angle/timings-za2048-casa.txt", 2048, 349.6, 24.576, 1, 0.2);
	readFOVFile(samples, "benchmark-fov/timings-zenith-fov-casa.txt", 0.0, 3072, 349.6, 1, 0.2);
	readNChanFile(samples, "benchmark-channels/timings-channels-casa.txt", 0.0, 3072, 349.6, 24.576, 1.0);
	std::cout << "Read " << samples.size() << " values.\n";
	printDuplicates(samples);
	FittingInfo info;
	info.n = samples.size();
	info.samples = samples.data();
	info.func = CASAFunc;
	double a = 1.0, b = 1.0, c = 1.0, d = 0.0, e = 0.0, f = 0.0;
	fixDW = true;
	fit(info, a, b, c, d, e, f);
	fixDW = false;
	fit(info, a, b, c, d, e, f);
	std::cout << "t = Nchan (" << a << " Npix^2 log Npix + " << b << " Nvis (sin ZA * fov + " << d << ")^2) + " << f << '\n';
	writeNVisFit("benchmark-nsamples/fit-zenith-casa.txt", info.func, a, b, c, d, e, f, 0.0, 3.072, 36.864, 1);
	writeNVisFit("benchmark-nsamples/fit-ZA010-casa.txt", info.func, a, b, c, d, e, f, 10.0, 3.072, 36.864, 1);
	writeNPixFit("benchmark-resolution/fit-casa.txt", info.func, a, b, c, d, e, f, 0.0, 349.6, 36.864, 1);
	writeZAFit("benchmark-zenith-angle/fit-za3072-casa.txt", info.func, a, b, c, d, e, f, 3.072, 349.6, 36.864, 1);
	writeZAFit("benchmark-zenith-angle/fit-za2048-casa.txt", info.func, a, b, c, d, e, f, 2.048, 349.6, 24.576, 1);//24.576
	writeFOVFit("benchmark-fov/fit-zenith-fov-casa.txt", info.func, a, b, c, d, e, f, 0.0, 3.072, 349.6, 1);
	writeFOVFit("benchmark-fov/fit-ZA010-fov-casa.txt", info.func, a, b, c, d, e, f, 10.0, 3.072, 349.6, 1);
	writeNChanFit("benchmark-channels/fit-zenith-casa.txt", info.func, a, b, c, d, e, f, 0.0, 3.072, 349.6, 36.864);
	calculateStdError(samples, info.func, a, b, c, d, e, f);
	optimalDeltaW(samples, info.func, a, b, c, d, e, f);
	double dt;
	optimalDeltaT(info.func, a, b, c, d, e, f, 10.0, 349.6, 3.072, 36.864, 1, false, dt);
	optimalDeltaT(info.func, a, b, c, d, e, f, 0.0, 349.6, 3.072, 36.864, 1, false, dt);
}

int main(int argc, char* argv[])
{
	if(argc == 2 && std::string(argv[1]) == "all-wsclean")
		fitAllWSClean();
	else if(argc == 2 && std::string(argv[1]) == "all-casa")
		fitAllCASA();
	else if(argc == 2 && std::string(argv[1]) == "all-wssclean")
		fitAllWSSClean();
	else {
		if(argc < 4)
		{
			std::cout << "syntax: fit <type> <input> <output>\n";
			return -1;
		}
		std::string fitType = argv[1], filename = argv[2], output = argv[3];
		
		std::vector<std::pair<double, TimingRecord>> valueArray;
		readFile(valueArray, filename);
		
		size_t n = valueArray.size();
		std::vector<double> xs(n), ys(n), ws(n);
		for(size_t i=0; i!=n; ++i)
		{
			const TimingRecord& record = valueArray[i].second;
			xs[i] = valueArray[i].first;
			ys[i] = record.average;
			ws[i] = 1.0 / record.stddev;
		}
		FittingInfo info;
		info.n = n;
		info.xs = xs.data();
		info.ys = ys.data();
		info.ws = ws.data();
		
		std::ofstream outFile(output);
		const double multStep = 1.01, addStep = 0.1;
		
		if(fitType == "n" || fitType == "nnlogn2" || fitType == "nnlogn3" || fitType == "nnlogn4")
		{
			double a, b, c, d = 0.0;
			if(fitType == "n")
			{
				info.func = NFunc;
				a = 0.0;
				b = 0.0;
				c = 1.0;
			}
			else if(fitType == "nnlogn2")
			{
				info.func = NSqLogNFunc2;
				a = 1.0;
				b = 0.0;
				c = 0.0;
			}
			else if(fitType == "nnlogn3")
			{
				info.func = NSqLogNFunc3;
				a = 1.0;
				b = 1.0;
				c = 0.0;
			}
			else {
				info.func = NSqLogNFunc4;
				a = 1.0;
				b = 1.0;
				c = 1.0;
			}
			fit(info, a, b, c, d);
			std::cout << "t(N) = ";
			bool hasTerm = false;
			if(a != 0.0) {
				std::cout << a << " N^2 log N ";
				hasTerm = true;
			}
			if(b != 0.0) {
				if(hasTerm) std::cout << " + ";
				else hasTerm = true;
				std::cout << b << " N^2 ";
			}
			if(c != 0.0) {
				if(hasTerm) std::cout << " + ";
				else hasTerm = true;
				std::cout << c << " N ";
			}
			if(d != 0.0) {
				if(hasTerm) std::cout << " + ";
				std::cout << d;
			}
			std::cout << '\n';
			
			double x=xs.front() / multStep;
			while(x < xs.back())
			{
				x *= multStep;
				outFile << x << '\t' << (a*x*x*log2(x) + b*x*x + c*x + d) << '\n';
			}
		}
		if(fitType == "nza")
		{
			double a = 1.0, b = 0.0, c = 0.0, d = 0.0;
			info.func = NZAFunc;
			fit(info, a, b, c, d);
			std::cout << " t(ZA) ~ " << a << " sin(ZA) + " << b << '\n';
			
			double x=xs.front()-addStep;
			while(x < xs.back())
			{
				x += addStep;
				double za = x*(M_PI/180.0);
				outFile << x << '\t' << (a / 2900.0) * (2900.0*sin(za) + 5.5*cos(za)) + b << '\n';
			}
		}
		if(fitType == "nzasq2" || fitType == "nzasq3")
		{
			double a = 1.0, b = 1.0, c = 0.0, d = 0.0;
			if(fitType == "nzasq2")
				info.func = NZASqFunc2;
			else
				info.func = NZASqFunc3;
			fit(info, a, b, c, d);
			std::cout << " t(ZA) = " << a << " sin(ZA)^2 + " << b << " sin(ZA) + " << c << '\n';
			
			double x=xs.front()-addStep;
			while(x < xs.back())
			{
				x += addStep;
				double za = x*(M_PI/180.0);
				double we = (2900.0*sin(za) + 5.5*cos(za)) / 2900.0;
				outFile << x << '\t' << a * we * we + b * we + c << '\n';
			}
		}
	}
}
