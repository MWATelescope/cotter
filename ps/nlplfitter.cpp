#include "nlplfitter.h"

#include <stdexcept>
#include <cmath>
#include <limits>
#include <tuple>

#ifdef HAVE_GSL
#include <gsl/gsl_vector.h>
#include <gsl/gsl_multifit_nlin.h>
#endif

class NLPLFitterData
{
public:
	typedef std::vector<std::tuple<double, double, double>> PointVec;
	PointVec points;
#ifdef HAVE_GSL
	gsl_multifit_fdfsolver *solver;
	
	static int fitting_func(const gsl_vector *xvec, void *data, gsl_vector *f)
	{
		const NLPLFitterData &fitterData = *reinterpret_cast<NLPLFitterData*>(data);
		double exponent = gsl_vector_get(xvec, 0);
		double factor = gsl_vector_get(xvec, 1);
		
		for(size_t i=0; i!=fitterData.points.size(); ++i)
		{
			double
				x = std::get<0>(fitterData.points[i]),
				y = std::get<1>(fitterData.points[i]),
				w = std::get<2>(fitterData.points[i]);
			
			gsl_vector_set(f, i, (factor * pow(x, exponent) - y) * w);
		}
			
		return GSL_SUCCESS;
	}
	
	static int fitting_func_deriv(const gsl_vector *xvec, void *data, gsl_matrix *J)
	{
		const NLPLFitterData &fitterData = *reinterpret_cast<NLPLFitterData*>(data);
		double exponent = gsl_vector_get(xvec, 0);
		double factor = gsl_vector_get(xvec, 1);
	
		for(size_t i=0; i!=fitterData.points.size(); ++i)
		{
			double
				x = std::get<0>(fitterData.points[i]),
				w = std::get<2>(fitterData.points[i]);
			
			double xToTheE = pow(x, exponent);
			double dfdexp = factor * log(x) * xToTheE;
			double dfdfac = xToTheE;
				
			gsl_matrix_set(J, i, 0, dfdexp * w);
			gsl_matrix_set(J, i, 1, dfdfac * w);
		}
			
		return GSL_SUCCESS;
	}

	static int fitting_func_both(const gsl_vector *x, void *data, gsl_vector *f, gsl_matrix *J)
	{
		fitting_func(x, data, f);
		fitting_func_deriv(x, data, J);
		return GSL_SUCCESS;
	}
#endif
};

#ifdef HAVE_GSL
void NonLinearPowerLawFitter::Fit(double& exponent, double& factor)
{
	const gsl_multifit_fdfsolver_type *T = gsl_multifit_fdfsolver_lmsder;
	_data->solver = gsl_multifit_fdfsolver_alloc (T, _data->points.size(), 2);
	
	gsl_multifit_function_fdf fdf;
	fdf.f = &NLPLFitterData::fitting_func;
	fdf.df = &NLPLFitterData::fitting_func_deriv;
	fdf.fdf = &NLPLFitterData::fitting_func_both;
	fdf.n = _data->points.size();
	fdf.p = 2;
	fdf.params = &*_data;
	
	double initialValsArray[2] = { exponent, factor };
	gsl_vector_view initialVals = gsl_vector_view_array (initialValsArray, 2);
	gsl_multifit_fdfsolver_set (_data->solver, &fdf, &initialVals.vector);

	int status;
	size_t iter = 0;
	do {
		iter++;
		status = gsl_multifit_fdfsolver_iterate (_data->solver);
		
		if(status)
			break;
		
		status = gsl_multifit_test_delta(_data->solver->dx, _data->solver->x, 1e-7, 1e-7);
		
  } while (status == GSL_CONTINUE && iter < 500);
	
	exponent = gsl_vector_get (_data->solver->x, 0);
	factor = gsl_vector_get (_data->solver->x, 1);
	
	gsl_multifit_fdfsolver_free(_data->solver);
}

#else
#warning "No GSL found: can not do non-linear power law fitting!"

void NonLinearPowerLawFitter::Fit(double& exponent, double& factor)
{
	throw std::runtime_error("Non-linear power law fitter was invoked, but GSL was not found during compilation, and is required for this");
}

#endif

NonLinearPowerLawFitter::NonLinearPowerLawFitter() :
	_data(new NLPLFitterData())
{
}

NonLinearPowerLawFitter::~NonLinearPowerLawFitter()
{
}

void NonLinearPowerLawFitter::AddDataPoint(double x, double y, double w)
{
	_data->points.push_back(std::make_tuple(x, y, w));
}

void NonLinearPowerLawFitter::FastFit(double& exponent, double& factor)
{
	double sumxy = 0.0, sumx = 0.0, sumy = 0.0, sumxx = 0.0;
	size_t n = 0;
	bool requireNonLinear = false;
	
	for(NLPLFitterData::PointVec::const_iterator i=_data->points.begin(); i!=_data->points.end(); ++i)
	{
		double x = std::get<0>(*i), y = std::get<1>(*i);
		if(y <= 0)
		{
			requireNonLinear = true;
			break;
		}
		if(x > 0 && y > 0)
		{
			long double
				logx = std::log(x),
				logy = std::log(y);
			sumxy += logx * logy;
			sumx += logx;
			sumy += logy;
			sumxx += logx * logx;
			++n;
		}
	}
	if(requireNonLinear)
	{
		exponent = 0.0;
		factor = 1.0;
		Fit(exponent, factor);
	}
	else {
		if(n == 0)
		{
			exponent = std::numeric_limits<double>::quiet_NaN();
			factor = std::numeric_limits<double>::quiet_NaN();
		}
		else {
			exponent = (n * sumxy - sumx * sumy) / (n * sumxx - sumx * sumx);
			factor = std::exp((sumy - exponent * sumx) / n);
		}
	}
}
