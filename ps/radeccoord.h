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
			bool sign = false;
			for(size_t i=0; i!=str.size(); ++i)
			{
				if(str[i] == '-')
				{
					sign = true;
					break;
				} else if(str[i] != ' ')
				{
					sign = false;
					break;
				}
			}
			long double secs=0.0, mins=0.0,
				hrs = strtol(str.c_str(), &cstr, 10);
			// Parse format '00h00m00.0s'
			if(*cstr == 'h')
			{
				++cstr;
				mins = strtol(cstr, &cstr, 10);
				if(*cstr == 'm')
				{
					++cstr;
					secs = strtold(cstr, &cstr);
					if(*cstr == 's')
						++cstr;
					else throw std::runtime_error("Missing 's'");
				} else throw std::runtime_error("Missing 'm'");
			}
			// Parse format '00:00:00.0'
			else if(*cstr == ':')
			{
				++cstr;
				mins = strtol(cstr, &cstr, 10);
				if(*cstr == ':')
				{
					++cstr;
					secs = strtold(cstr, &cstr);
				} else throw std::runtime_error("Missing ':' after minutes");
			}
			else throw std::runtime_error("Missing 'h' or ':'");
			if(*cstr != 0)
				throw std::runtime_error("Could not parse RA (string contains more tokens than expected)");
			if(sign)
				return (hrs/24.0 - mins/(24.0*60.0) - secs/(24.0*60.0*60.0))*2.0*M_PI;
			else
				return (hrs/24.0 + mins/(24.0*60.0) + secs/(24.0*60.0*60.0))*2.0*M_PI;
		}
		
		static long double ParseDec(const std::string &str)
		{
			char *cstr;
			bool sign = false;
			for(size_t i=0; i!=str.size(); ++i)
			{
				if(str[i] == '-')
				{
					sign = true;
					break;
				} else if(str[i] != ' ')
				{
					sign = false;
					break;
				}
			}
			long double secs=0.0, mins=0.0,
				degs = strtol(str.c_str(), &cstr, 10);
			// Parse format '00d00m00.0s'
			if(*cstr == 'd')
			{
				++cstr;
				mins = strtol(cstr, &cstr, 10);
				if(*cstr == 'm')
				{
					++cstr;
					secs = strtold(cstr, &cstr);
					if(*cstr == 's')
						++cstr;
					else throw std::runtime_error("Missing 's'");
				} else throw std::runtime_error("Missing 'm'");
			}
			// Parse format '00.00.00.0'
			else if(*cstr == '.')
			{
				++cstr;
				mins = strtol(cstr, &cstr, 10);
				if(*cstr == '.')
				{
					++cstr;
					secs = strtold(cstr, &cstr);
				} else throw std::runtime_error("Missing '.' after minutes");
			}
			else throw std::runtime_error("Missing 'd' or '.' after degrees");
			if(*cstr != 0)
				throw std::runtime_error("Could not parse Dec (string contains more tokens than expected)");
			else if(sign)
				return (degs/360.0 - mins/(360.0*60.0) - secs/(360.0*60.0*60.0))*2.0*M_PI;
			else
				return (degs/360.0 + mins/(360.0*60.0) + secs/(360.0*60.0*60.0))*2.0*M_PI;
		}
		
		static std::string RAToString(long double ra)
		{
			long double hrs = fmodl(ra * (12.0L / M_PIl), 24.0L);
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
