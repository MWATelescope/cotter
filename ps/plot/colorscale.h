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
#ifndef PLOT_COLORSCALE_H
#define PLOT_COLORSCALE_H

#include <string>
#include <map>

#include <gtkmm/drawingarea.h>

#include "verticalplotscale.h"

/**
	@author A.R. Offringa <offringa@astro.rug.nl>
*/
class ColorScale {
	public:
		ColorScale();
		
		virtual ~ColorScale()
		{
		}
		void SetPlotDimensions(double plotWidth, double plotHeight, double topMargin)
		{
			_plotWidth = plotWidth;
			_plotHeight = plotHeight;
			_topMargin = topMargin;
			_width = 0.0;
		}
		double GetWidth(Cairo::RefPtr<Cairo::Context> cairo)
		{
			if(_width == 0.0)
				initWidth(cairo);
			return _width;
		}
		void Draw(Cairo::RefPtr<Cairo::Context> cairo);
		void InitializeNumericTicks(double min, double max)
		{
			_width = 0.0;
			_min = min;
			_max = max;
			_isLogaritmic = false;
			_verticalPlotScale.InitializeNumericTicks(min, max);
		}
		void InitializeLogarithmicTicks(double min, double max)
		{
			_width = 0.0;
			_min = min;
			_max = max;
			_isLogaritmic = true;
			_verticalPlotScale.InitializeLogarithmicTicks(min, max);
		}
		void SetColorValue(double value, double red, double green, double blue)
		{
			ColorValue cValue;
			cValue.red = red;
			cValue.green = green;
			cValue.blue = blue;
			_colorValues.insert(std::pair<double, ColorValue>(value, cValue));
		}
		void SetDescriptionFontSize(double fontSize)
		{
			_verticalPlotScale.SetDescriptionFontSize(fontSize);
		}
		void SetTickValuesFontSize(double fontSize)
		{
			_verticalPlotScale.SetTickValuesFontSize(fontSize);
		}
		void SetDrawWithDescription(bool drawWithDescription)
		{
			_verticalPlotScale.SetDrawWithDescription(drawWithDescription);
		}
		void SetUnitsCaption(const std::string &caption)
		{
			_verticalPlotScale.SetUnitsCaption(caption);
		}
	private:
		static const double BAR_WIDTH;
		
		struct ColorValue
		{
			double red, green, blue;
		};
		
		void initWidth(Cairo::RefPtr<Cairo::Context> cairo);
		
		double _plotWidth, _plotHeight, _topMargin;
		double _scaleWidth, _width;
		double _min, _max;
		class VerticalPlotScale _verticalPlotScale;
		std::map<double, ColorValue> _colorValues;
		bool _isLogaritmic;
};

#endif
