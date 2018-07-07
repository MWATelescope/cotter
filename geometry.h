#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <cmath>

#include <star/pal.h>

#define SPEED_OF_LIGHT 299792458.0        // speed of light in m/s

/**
 * These functions come from corr2uvfits written by Randall Wayth.
 */
class Geometry
{
public:
	/**
	 * Convert coordinator from local topocentric East, North, Height units to
	 * 'local' XYZ units. Local means Z point north, X points through the equator
	 * from the geocenter along the local meridian and Y is East.
	 * Latitude is geodetic, in radian.
	 */
	static void ENH2XYZ_local(double east, double north, double height, double lattitude, double& x, double& y, double& z) {
		double
			latSin = sin(lattitude),
			latCos = cos(lattitude);

		x = -north * latSin + height * latCos;
		y = east;
		z = north * latCos + height * latSin;
	}
	
	static void Rotate(double rotationRad, double &x, double &y)
	{
		double cosRotation = cos(rotationRad), sinRotation = sin(rotationRad);
		double xtemp = x * cosRotation - y * sinRotation;
		y = x * sinRotation + y * cosRotation;
		x = xtemp;
	}
	
	static void RotateLongLat(double longRad, double latRad, double &x, double &y, double &z)
	{
		double r = sqrt(x*x + y*y + z*z);
		if(r != 0)
		{
			double azi = atan2(y, x);
			double elev = acos(z / r);
			elev += latRad;
			azi += longRad;
			x = r * sin(elev) * cos(azi);
			y = r * sin(elev) * sin(azi);
			z = r * cos(elev);
		}
	}
	
	struct UVWTimestepInfo
	{
		double rmatpr[3][3];
		double ha2000;
		double lmst;
		double dec_aber;
		double lmst2000;
	};
	
	static void CalcUVW(UVWTimestepInfo &uvwInfo, double x, double y, double z, double &u, double &v, double &w)
	{
		double ha2000 = uvwInfo.ha2000;
		double lmst = uvwInfo.lmst;
		double dec_aber = uvwInfo.dec_aber;
		double lmst2000 = uvwInfo.lmst2000;
		double xprec, yprec, zprec;
		precXYZ(uvwInfo.rmatpr, x, y, z, lmst, &xprec, &yprec, &zprec, lmst2000);
		CalcUVW(ha2000, dec_aber, xprec, yprec, zprec, u, v, w);
	}
	
	static void CalcUVW(double ha, double dec, double x, double y, double z, double &u, double &v, double &w)
	{
    const double sh = sin(ha), sd = sin(dec);
    const double ch = cos(ha), cd = cos(dec);
    u  = sh*x + ch*y;
    v  = -sd*ch*x + sd*sh*y + cd*z;
    w  = cd*ch*x  - cd*sh*y + sd*z;
	}
	
	static double GetMJD(int year, int month, double day, int refHour, int refMinute, double refSecond)
	{
		int res;
		double jdBase;
		palCaldj(year, month, day, &jdBase, &res); // get MJD for calendar day of obs
		return jdBase +
			//2400000.5 +  // convert MJD to JD
			refHour/24.0 + refMinute/1440.0 + refSecond/86400.0; // add intra-day offset
	}
	
	static void PrepareTimestepUVW(UVWTimestepInfo &uvwInfo, double mjd, double arrayLongitudeRad, double arrayLattitudeRad, double raHrs, double decDeg)
	{
		
		double lmst = palDranrm(palGmst(mjd) + arrayLongitudeRad);  // local mean sidereal time, given
		
		/* convert mean RA/DEC of phase center to apparent for current observing time. This applies precession, nutation, annual abberation. */
		double ra_app, dec_app;
    palMap(raHrs*(M_PI/12.0), decDeg*(M_PI/180.0), 0.0, 0.0, 0.0, 0.0, 2000.0,
       mjd, &ra_app, &dec_app);
		
    /* calc apparent HA of phase center, normalise to be between 0 and 2*pi */
    // double ha = palDranrm(lmst - ra_app);
		
		/* Compute the apparent direction of the phase center in the J2000 coordinate system */
		double ra_aber, dec_aber;
		aber_radec_rad(2000.0, mjd, raHrs*(M_PI/12.0), decDeg*(M_PI/180.0), &ra_aber,&dec_aber);
								 
		double rmattr[3][3];
		palPrenut(2000.0, mjd, rmattr);
		
		mat_transpose(rmattr, uvwInfo.rmatpr);
	
		double ha2000, newarrlat, lmst2000;
		ha_dec_j2000(uvwInfo.rmatpr,lmst,arrayLattitudeRad,ra_aber,dec_aber,&ha2000,&newarrlat,&lmst2000);
		
		uvwInfo.dec_aber = dec_aber;
		uvwInfo.ha2000 = ha2000;
		uvwInfo.lmst = lmst;
		uvwInfo.lmst2000 = lmst2000;
	}

	/**
	* lmst, lmst2000 are the local mean sidereal times in radians                                         
	* for the obs. and J2000 epochs.                                                                      
	*/
	static void precXYZ(double rmat[3][3], double x, double y, double z, double lmst,
					double *xp, double *yp, double *zp, double lmst2000)
	{
		double sep = sin(lmst);
		double cep = cos(lmst);
		double s2000 = sin(lmst2000);
		double c2000 = cos(lmst2000);

		/* rotate to frame with x axis at zero RA */
		double xpr = cep*x - sep*y;
		double ypr = sep*x + cep*y;
		double zpr = z;

		double xpr2 = (rmat[0][0])*xpr + (rmat[0][1])*ypr + (rmat[0][2])*zpr;
		double ypr2 = (rmat[1][0])*xpr + (rmat[1][1])*ypr + (rmat[1][2])*zpr;
		double zpr2 = (rmat[2][0])*xpr + (rmat[2][1])*ypr + (rmat[2][2])*zpr;

		/* rotate back to frame with xp pointing out at lmst2000 */
		*xp = c2000*xpr2 + s2000*ypr2;
		*yp = -s2000*xpr2 + c2000*ypr2;
		*zp = zpr2;
	}
	
	static void aber_radec_rad(double eq, double mjd, double ra1, double dec1, double *ra2, double *dec2)
	{
		double v1[3], v2[3];

		palDcs2c(ra1, dec1, v1);
		stelaber(eq, mjd, v1, v2);
		palDcc2s(v2, ra2, dec2);
		*ra2 = palDranrm(*ra2);
	}

	static void stelaber(double eq, double mjd, double v1[3], double v2[3])
	{
		double amprms[21], v1n[3], v2un[3], w, ab1, abv[3], p1dv;
		int i;

		palMappa(eq,mjd,amprms);

		/* code from mapqk.c (w/ a few names changed): */

		/* Unpack scalar and vector parameters */
		ab1 = amprms[11];
		for ( i = 0; i < 3; i++ )
		{
				abv[i] = amprms[i+8];
		}

		palDvn ( v1, v1n, &w );

		/* Aberration (normalization omitted) */
		p1dv = palDvdv ( v1n, abv );
		w = 1.0 + p1dv / ( ab1 + 1.0 );
		for ( i = 0; i < 3; i++ ) {
				v2un[i] = ab1 * v1n[i] + w * abv[i];
		}
		/* normalize  (not in mapqk.c */
		palDvn ( v2un, v2, &w );
	}

	static void mat_transpose(double rmat1[3][3], double rmat2[3][3])
	{
		int i, j;

		for(i=0;i<3;++i) {
			for(j=0;j<3;++j) {
				rmat2[j][i] = rmat1[i][j];
			}
		}
	}

	static void ha_dec_j2000(double rmat[3][3], double lmst, double lat_rad, double ra2000,
                  double dec2000, double *newha, double *newlat, double *newlmst)
	{
		double nwlmst, nwlat;

		rotate_radec(rmat,lmst,lat_rad,&nwlmst,&nwlat);
		*newlmst = nwlmst;
		*newha = palDranrm(nwlmst - ra2000);
		*newlat = nwlat;
	}
	
	/* rmat = 3x3 rotation matrix for going from one to another epoch                                      
	* ra1, dec1, ra2, dec2 are in radians                                                                 
	*/
	static void rotate_radec(double rmat[3][3], double ra1, double dec1,
						double *ra2, double *dec2)
	{
		double v1[3], v2[3];

		palDcs2c(ra1,dec1,v1);
		palDmxv(rmat,v1,v2);
		palDcc2s(v2,ra2,dec2);
		*ra2 = palDranrm(*ra2);
	}

	/* constants for WGS84 Geoid model for the Earth */
	#define EARTH_RAD_WGS84 6378137.0 /* meters */
	#define E_SQUARED 6.69437999014e-3

	/**
		* Convert Geodetic lat/lon/height to XYZ coords
		* uses constants from WGS84 system
		*/
	static void Geodetic2XYZ(double lat_rad, double lon_rad, double height_meters, double &X, double &Y, double &Z) {
		double s_lat,s_lon,c_lat,c_lon;
		double chi;

		s_lat = sin(lat_rad); c_lat = cos(lat_rad);
		s_lon = sin(lon_rad); c_lon = cos(lon_rad);
		chi = sqrt(1.0 - E_SQUARED*s_lat*s_lat);

		X = (EARTH_RAD_WGS84/chi + height_meters)*c_lat*c_lon;
		Y = (EARTH_RAD_WGS84/chi + height_meters)*c_lat*s_lon;
		Z = (EARTH_RAD_WGS84*(1.0-E_SQUARED)/chi + height_meters)*s_lat;
	}
	
	
};

#endif
