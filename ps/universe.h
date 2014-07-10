#ifndef UNIVERSE_H
#define UNIVERSE_H

#include <cmath>

class Universe
{
public:
	/**
	 * Mass density parameter
	 */
	static double OmegaM()
	{
		return 0.27;
	}
	
	/**
	 * Curvature
	 */
	static double OmegaK()
	{
		return 1.0 - OmegaM() - OmegaL();
	}
	
	/**
	 * Dark energy density parameter
	 */
	static double OmegaL()
	{
		return 0.73;
	}
	
	
	static double H0()
	{
		return 0.72;
	}
	
	/**
	 * Hubble distance
	 */
	static double DH()
	{
		return C() / H0();
	}
	
	static double DHinMPC()
	{
		return 4228.0;
	}
	
	/**
	 * Speed of light
	 */
	static double C()
	{
		return 299792458.0;
	}
	
	/**
	 * Hubble parameter
	 */
	static double E(double z)
	{
		return sqrt(OmegaM()*(1.0+z)*(1.0+z)*(1.0+z) + OmegaK()*(1.0+z)*(1.0+z) + OmegaL());
	}
	
	static double ComovingDistanceInMPCoverH(double z)
	{
		double d = 0.0;
		for(size_t i=0; i!=10001; ++i)
		{
			d += 1.0 / E(double(i)*z/10000.0);
		}
		// We integrate over 10001 samples :
		return d * DHinMPC() * z * H0() / 10001.0;
	}
	
	static double HIRestFrequencyInHz()
	{
		return 1420405752.0;
	}
	
	static double HIRedshift(double frequencyInHz)
	{
		return HIRestFrequencyInHz()/frequencyInHz - 1.0;
	}
};

#endif
