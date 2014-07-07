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
#ifndef SYSTEM_H
#define SYSTEM_H

#include <map>
#include <string>

#include "dimension.h"

/**
	@author A.R. Offringa <offringa@astro.rug.nl>
*/
class System {
	public:
		System() : _includeZeroYAxis(false)
		{
		}

		~System()
		{
			Clear();
		}

		void AddToSystem(class Plot2DPointSet &pointSet)
		{
			Dimension *dimension;
			if(_dimensions.count(pointSet.YUnits()) == 0)
			{
				dimension = new Dimension();
				_dimensions.insert(std::pair<std::string, Dimension*>(pointSet.YUnits(), dimension));
			} else {
				dimension = _dimensions.find(pointSet.YUnits())->second;
			}
			dimension->AdjustRanges(pointSet);
		}

		double XRangeMin(class Plot2DPointSet &pointSet) const
		{
			return _dimensions.find(pointSet.YUnits())->second->XRangeMin();
		}
		double XRangeMax(class Plot2DPointSet &pointSet) const
		{
			return _dimensions.find(pointSet.YUnits())->second->XRangeMax();
		}
		double YRangeMin(class Plot2DPointSet &pointSet) const
		{
			const double yMin = _dimensions.find(pointSet.YUnits())->second->YRangeMin();
			if(yMin > 0.0 && _includeZeroYAxis)
				return 0.0;
			else
				return yMin;
		}
		double YRangePositiveMin(class Plot2DPointSet &pointSet) const
		{
			return _dimensions.find(pointSet.YUnits())->second->YRangePositiveMin();
		}
		double YRangeMax(class Plot2DPointSet &pointSet) const
		{
			const double yMax = _dimensions.find(pointSet.YUnits())->second->YRangeMax();
			if(yMax < 0.0 && _includeZeroYAxis)
				return 0.0;
			else
				return yMax;
		}
		double YRangePositiveMax(class Plot2DPointSet &pointSet) const
		{
			return _dimensions.find(pointSet.YUnits())->second->YRangePositiveMax();
		}
		void Clear()
		{
			for(std::map<std::string, Dimension*>::iterator i=_dimensions.begin();i!=_dimensions.end();++i)
				delete i->second;
			_dimensions.clear();
		}
		void SetIncludeZeroYAxis(bool includeZeroYAxis) { _includeZeroYAxis = includeZeroYAxis; }
	private:
		std::map<std::string, Dimension*> _dimensions;
		bool _includeZeroYAxis;
};

#endif
