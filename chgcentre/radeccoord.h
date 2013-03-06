#ifndef RADECCOORD_H
#define RADECCOORD_H

#include <string>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <cmath>

class RaDecCoord
{
	public:
		static long double ParseRA(const std::string &str)
		{
			char *cstr;
			long double secs=0.0, mins=0.0,
				hrs = strtold(str.c_str(), &cstr);
			if(*cstr == 'h')
			{
				++cstr;
				mins = strtold(cstr, &cstr);
				if(*cstr == 'm')
				{
					++cstr;
					secs = strtold(cstr, &cstr);
					if(*cstr == 's')
						++cstr;
					else throw std::runtime_error("Missing 's'");
				} else throw std::runtime_error("Missing 'm'");
			} else throw std::runtime_error("Missing 'h'");
			if(*cstr != 0)
				throw std::runtime_error("Could not parse RA");
			else
				return (hrs/24.0 + mins/(24.0*60.0) + secs/(24.0*60.0*60.0))*2.0*M_PI;
		}
		
		static long double ParseDec(const std::string &str)
		{
			char *cstr;
			long double secs=0.0, mins=0.0,
				degs = strtold(str.c_str(), &cstr);
			bool sign = degs < 0.0;
			if(*cstr == 'd')
			{
				++cstr;
				mins = strtold(cstr, &cstr);
				if(*cstr == 'm')
				{
					++cstr;
					secs = strtold(cstr, &cstr);
					if(*cstr == 's')
						++cstr;
					else throw std::runtime_error("Missing 's'");
				} else throw std::runtime_error("Missing 'm'");
			} else throw std::runtime_error("Missing 'd'");
			if(*cstr != 0)
				throw std::runtime_error("Could not parse Dec");
			else if(sign)
				return (degs/360.0 - mins/(360.0*60.0) - secs/(360.0*60.0*60.0))*2.0*M_PI;
			else
				return (degs/360.0 + mins/(360.0*60.0) + secs/(360.0*60.0*60.0))*2.0*M_PI;
		}
		
		static std::string RAToString(long double ra)
		{
			long double hrs = ra * (12.0L / M_PIl);
			hrs = round(hrs * 60.0L*60.0L*10.0L)/(60.0L*60.0L*10.0L);
			std::stringstream s;
			if(hrs < 0) {
				hrs = -hrs;
				s << '-';
			}
			hrs = ((round(hrs*60.0L*60.0L*10.0L))+0.5) / (60.0L*60.0L*10.0L);
			int hrsInt = int(floor(hrs)), minInt = int(floor(fmod(hrs,1.0L)*60.0L)),
				secInt = int(floor(fmod(hrs*60.0L,1.0L)*(60.0L))),
				subSecInt = int(floor(fmod(hrs*(60.0L*60.0L),1.0L)*(10.0L)));
			s << (char) ((hrsInt/10)+'0') << (char) ((hrsInt%10)+'0') << 'h'
				<< (char) ((minInt/10)+'0') << (char) ((minInt%10)+'0') << 'm'
				<< (char) ((secInt/10)+'0') << (char) ((secInt%10)+'0')
				<< '.' << (char) (subSecInt+'0') << 's';
			return s.str();
		}
		
		static std::string DecToString(long double dec)
		{
			long double deg = dec * (180.0 / M_PI);
			deg = round(deg * 60.0*60.0)/(60.0*60.0);
			std::stringstream s;
			if(deg < 0) {
				deg = -deg;
				s << '-';
			}
			deg = (round(deg*60.0L*60.0L)+0.5) / (60.0L*60.0L);
			int degInt = int(floor(deg)), minInt = int(floor(fmod(deg,1.0)*60.0)),
				secInt = int(floor(fmod(deg,1.0/60.0)*(60.0*60.0)));
			s << (char) ((degInt/10)+'0') << (char) ((degInt%10)+'0') << 'd'
				<< (char) ((minInt/10)+'0') << (char) ((minInt%10)+'0') << 'm'
				<< (char) ((secInt/10)+'0') << (char) ((secInt%10)+'0') << 's';
			return s.str();
		}
	private:
		RaDecCoord() { }
};

#endif
