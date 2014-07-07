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
#include "colorscale.h"

const double ColorScale::BAR_WIDTH = 15.0;

ColorScale::ColorScale()
: _width(0.0), _verticalPlotScale()
{
	_verticalPlotScale.SetUnitsCaption("z");
}

void ColorScale::initWidth(Cairo::RefPtr<Cairo::Context> cairo)
{
	if(_width == 0.0)
	{
		const double textHeight = _verticalPlotScale.GetTextHeight(cairo);
		const double scaleHeight = _plotHeight - 2.0*textHeight - _topMargin;
		_verticalPlotScale.SetPlotDimensions(_plotWidth, scaleHeight, _topMargin + textHeight);
		_scaleWidth = _verticalPlotScale.GetWidth(cairo);
		_width = _scaleWidth + BAR_WIDTH;
	}
}

void ColorScale::Draw(Cairo::RefPtr<Cairo::Context> cairo)
{
	initWidth(cairo);
	const double textHeight = _verticalPlotScale.GetTextHeight(cairo);
	const double scaleTop = _topMargin + textHeight;
	const double scaleHeight = _plotHeight - 2.0*textHeight - _topMargin;
	ColorValue backValue;
	if(!_colorValues.empty())
	{
		backValue = _colorValues.begin()->second;
	} else {
		backValue.red = 1.0;
		backValue.green = 1.0;
		backValue.blue = 1.0;
	}
	cairo->rectangle(_plotWidth - _width + _scaleWidth, scaleTop,
										BAR_WIDTH, scaleHeight);
	cairo->set_source_rgb(backValue.red, backValue.green, backValue.blue);
	cairo->fill();
	
	for(std::map<double, ColorValue>::const_iterator i=_colorValues.begin();
		i!=_colorValues.end();++i)
	{
		double val;
		if(_isLogaritmic)
		{
			const double minLog10 = log10(_min);
			if(i->first <= 0.0)
				val = 0.0;
			else
				val = (log10(i->first) - minLog10) / (log10(_max) - minLog10);
		} else {
			val = (i->first - _min) / (_max - _min);
			if(val < 0.0) val = 0.0;
			if(val > 1.0) val = 1.0;
		}
		const double height = scaleHeight * (1.0 - val);
		const ColorValue &color = i->second;
		cairo->set_source_rgb(color.red, color.green, color.blue);
		cairo->rectangle(_plotWidth - _width + _scaleWidth, scaleTop,
											BAR_WIDTH, height);
		cairo->fill();
	}
	
	cairo->rectangle(_plotWidth - _width + _scaleWidth, scaleTop,
										BAR_WIDTH, scaleHeight);
	cairo->set_source_rgb(0.0, 0.0, 0.0);
	cairo->stroke();
	
	_verticalPlotScale.Draw(cairo, _plotWidth - _width, 0.0);
}
