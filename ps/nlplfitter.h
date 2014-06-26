#ifndef NLPL_FITTER_H
#define NLPL_FITTER_H

#include <vector>
#include <memory>

/**
 * This class fits a power law to a set of points. Note that there is a
 * linear solution for this problem, but the linear solution requires
 * all values to be positive, which is not the case for e.g. spectral
 * energy distributions, because these have noise.
 * This fitter does not have this requirement.
 */
class NonLinearPowerLawFitter
{
public:
	NonLinearPowerLawFitter();
	
	~NonLinearPowerLawFitter();
	
	void AddDataPoint(double x, double y, double w = 1.0);
	
	void Fit(double& exponent, double& factor);
	
	void FastFit(double& exponent, double& factor);
	
private:
	std::unique_ptr<class NLPLFitterData> _data;
};

#endif
