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
#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <string>

/**
	@author A.R. Offringa <offringa@astro.rug.nl>
*/
class Stopwatch{
	public:
		Stopwatch();
		Stopwatch(bool start);
		~Stopwatch();
		void Start();
		void Pause();
		void Reset();

		std::string ToString() const;
		std::string ToShortString() const;
		long double Seconds() const;
		
		std::string ToDaysString() const;
		std::string ToHoursString() const;
		std::string ToMinutesString() const;
		std::string ToSecondsString() const;
		std::string ToMilliSecondsString() const;
		std::string ToMicroSecondsString() const;
		std::string ToNanoSecondsString() const;
	private:
		bool _running;
		boost::posix_time::ptime _startTime;
		boost::posix_time::time_duration _sum;
};

#endif
