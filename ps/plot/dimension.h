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
#ifndef DIMENSION_H
#define DIMENSION_H

#include "plot2dpointset.h"

/**
	@author A.R. Offringa <offringa@astro.rug.nl>
*/
class Dimension {
	public:
		Dimension() : _pointSets(0) { }
		~Dimension() { }

		void AdjustRanges(Plot2DPointSet &pointSet)
		{
			if(_pointSets == 0)
			{
				_xRangeMin = pointSet.XRangeMin();
				_xRangeMax = pointSet.XRangeMax();
				_yRangeMin = pointSet.YRangeMin();
				_yRangeMax = pointSet.YRangeMax();
				_yRangePositiveMin = pointSet.YRangePositiveMin();
				_yRangePositiveMax = pointSet.YRangePositiveMax();
			} else {
				if(_xRangeMin > pointSet.XRangeMin() && std::isfinite(pointSet.XRangeMin()))
					_xRangeMin = pointSet.XRangeMin();
				if(_xRangeMax < pointSet.XRangeMax() && std::isfinite(pointSet.XRangeMax()))
					_xRangeMax = pointSet.XRangeMax();
				
				if(_yRangeMin > pointSet.YRangeMin() && std::isfinite(pointSet.YRangeMin()))
					_yRangeMin = pointSet.YRangeMin();
				if(_yRangePositiveMin > pointSet.YRangePositiveMin() && std::isfinite(pointSet.YRangePositiveMin())) 
					_yRangePositiveMin = pointSet.YRangePositiveMin();
				
				if(_yRangeMax < pointSet.YRangeMax() && std::isfinite(pointSet.YRangeMax()))
					_yRangeMax = pointSet.YRangeMax();
				if(_yRangePositiveMax < pointSet.YRangePositiveMax() && std::isfinite(pointSet.YRangePositiveMax())) 
					_yRangePositiveMin = pointSet.YRangePositiveMax();
			}
			++_pointSets;
		}

		double XRangeMin() const { return _xRangeMin; }
		double XRangeMax() const { return _xRangeMax; }
		double YRangeMin() const { return _yRangeMin; }
		double YRangeMax() const { return _yRangeMax; }
		double YRangePositiveMin() const { return _yRangePositiveMin; }
		double YRangePositiveMax() const { return _yRangePositiveMax; }
	private:
		size_t _pointSets;
		double _xRangeMin, _xRangeMax;
		double _yRangeMin, _yRangeMax;
		double _yRangePositiveMin, _yRangePositiveMax;
};

#endif
