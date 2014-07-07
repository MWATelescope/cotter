/***************************************************************************
 *   Copyright (C) 2008 by A.R. Offringa   *
 *   offringa@astro.rug.nl   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef MSIODATE_H
#define MSIODATE_H

#include <string>
#include <sstream>
#include <cmath>

class Date {
	public:
		static double JDToYears(double jd) {
			return jd / 365.25L - 4712.0L;
		}
		static double JDToMonth(double jd) {
			return ((int) floor((jd / (365.25L)- 4712.0L)/12.0L))%12;
		}
		static int JDToDayOfYear(double jd) {
			//return floor(jd - floor(jd / (365.25L))*365.25L);
			double x2 = (jd-1721119.5); // number of days since year 0
			int c2 = (int) ((8.0*x2 + 7.0)/292194.0);
			double x1 = x2-floor(146097.0*c2/4.0); // number of days since beginning of century
			int c1 = (int) ((200*x1 + 199)/73050);
			double x0 = x1 - floor(36525.0*c1/100.0);
			return (int) x0;
		}
		static void JDToDate(double jd, int &dayOfMonth, int &month, int &year)
		{
			double x2 = (jd-1721119.5); // number of days since year 0
			int c2 = (int) ((8.0*x2 + 7.0)/292194.0);
			double x1 = x2-floor(146097.0*c2/4.0); // number of days since beginning of century
			int c1 = (int) ((200*x1 + 199)/73050);
			double x0 = x1 - floor(36525.0*c1/100.0);
			year = 100*c2 + c1;
			month = (int) ((10.0*x0 + 923.0)/306.0);
			dayOfMonth = (int) x0-((153*month-457)/5) + 1;
			if(month>12)
			{
				month -= 12;
				++year;
			}
		}
		static double JDToHourOfDay(double jd) {
			return fmodl(jd+0.5, 1.0) * 24.0;
		}
		static double MJDToJD(double mjd) {
			return mjd + 2400000.5;
		}
		static double JDToMJD(double jd) {
			return jd - 2400000.5;
		}
		static double JDToAipsMJD(double jd) {
			return JDToMJD(jd) * (60.0*60.0*24.0);
		} 
		static double MJDToAipsMJD(double jd) {
			return jd * (60.0*60.0*24.0);
		} 
		static double AipsMJDToJD(double aipsMjd) {
			return MJDToJD(aipsMjd / (60.0*60.0*24.0));
		}
		static double AipsMJDToYears(double aipsMjd) {
			return JDToYears(AipsMJDToJD(aipsMjd));
		}
		static std::string ToString(double time, int dayOfMonth, int month, int year)
		{
			std::stringstream s;
			int mins = int(time*60)%60;
			int secs = int(time*3600)%60;
			int msec = int(time*3600000)%1000;
			s << floor(time) << ":" << (mins/10) << (mins%10) << ":" << (secs/10) << (secs%10) << "." << msec/100 << (msec/10)%10 << (msec)%10
				<< ", " << year << "-";
			if(month < 10) s << "0";
			s << month << "-";
			if(dayOfMonth < 10) s << "0";
			s << dayOfMonth;
			return s.str();
		}
		static std::string ToString(double time)
		{
			std::stringstream s;
			int msec = int(round(time*3600000))%1000;
			time -= msec/3600000.0;
			
			int secs = int(round(time*3600))%60;
			time -= secs/3600.0;
			
			int mins = int(round(time*60))%60;
			time -= mins/60.0;
			
			int hours = int(round(time));
			time -= hours;
			s << hours << ":" << (mins/10) << (mins%10);
			if(msec != 0 || secs != 0)
			{
				s << ":" << (secs/10) << (secs%10);
				if(msec != 0)
				{
					s << "." << msec/100;
					if(msec%100!=0)
					{
						s << (msec/10)%10;
						if(msec%10!=0)
							s << (msec)%10;
					}
				}
			}
			return s.str();
		}
		static std::string ToString(int dayOfMonth, int month, int year)
		{
			std::stringstream s;
			s << year << "-";
			if(month < 10) s << "0";
			s << month << "-";
			if(dayOfMonth < 10) s << "0";
			s << dayOfMonth;
			return s.str();
		}
		static std::string AipsMJDToString(double aipsMjd) {
			double jd = AipsMJDToJD(aipsMjd);
			double time = JDToHourOfDay(jd);
			int year, month, day;
			JDToDate(jd, day, month, year);
			return ToString(time, day, month, year);
		}
		static std::string AipsMJDToDateString(double aipsMjd) {
			double jd = AipsMJDToJD(aipsMjd);
			int year, month, day;
			JDToDate(jd, day, month, year);
			return ToString(day, month, year);
		}
		static std::string AipsMJDToTimeString(double aipsMjd) {
			double jd = AipsMJDToJD(aipsMjd);
			double time = JDToHourOfDay(jd);
			return ToString(time);
		}
		static std::string AipsMJDToRoundTimeString(double aipsMjd) {
			double jd = AipsMJDToJD(aipsMjd);
			double time = round(JDToHourOfDay(jd)*60.0*60.0)/(60.0*60.0);
			return ToString(time);
		}
		static std::string JDToString(double jd)
		{
			double time = JDToHourOfDay(jd);
			int year, month, day;
			JDToDate(jd, day, month, year);
			return ToString(time, day, month, year);
		}
};

#endif //MSIODATE_H
