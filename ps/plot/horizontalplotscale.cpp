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
#include "horizontalplotscale.h"

#include "tickset.h"

HorizontalPlotScale::HorizontalPlotScale()
	: _plotWidth(0), _plotHeight(0), _metricsAreInitialized(false), _tickSet(0), _drawWithDescription(true), _unitsCaption("x"), _descriptionFontSize(14), _tickValuesFontSize(14), _rotateUnits(false), _isLogarithmic(false)
{
}

HorizontalPlotScale::~HorizontalPlotScale()
{
	if(_tickSet != 0)
		delete _tickSet;
}

double HorizontalPlotScale::GetHeight(Cairo::RefPtr<Cairo::Context> cairo)
{
	initializeMetrics(cairo);
	return _height;
}

double HorizontalPlotScale::GetRightMargin(Cairo::RefPtr<Cairo::Context> cairo)
{
	initializeMetrics(cairo);
	return _rightMargin;
}

void HorizontalPlotScale::Draw(Cairo::RefPtr<Cairo::Context> cairo)
{
	initializeMetrics(cairo);
	cairo->set_source_rgb(0.0, 0.0, 0.0);
	cairo->set_font_size(_tickValuesFontSize);
	for(unsigned i=0;i!=_tickSet->Size();++i)
	{
		const Tick tick = _tickSet->GetTick(i);
		double x = tick.first * (_plotWidth - _verticalScaleWidth) + _verticalScaleWidth;
		cairo->move_to(x, _topMargin + _plotHeight);
		cairo->line_to(x, _topMargin + _plotHeight + 3);
		Cairo::TextExtents extents;
		cairo->get_text_extents(tick.second, extents);
		if(_rotateUnits)
		{
			cairo->move_to(x - extents.y_bearing - extents.height/2, _topMargin + _plotHeight + extents.width + 8);
			cairo->save();
			cairo->rotate(-M_PI*0.5);
			cairo->show_text(tick.second);
			cairo->restore();
		}
		else
		{
			cairo->move_to(x - extents.width/2, _topMargin + _plotHeight - extents.y_bearing + extents.height);
			cairo->show_text(tick.second);
		}
	}
	cairo->stroke();
	
	if(_drawWithDescription)
		drawUnits(cairo);
}

void HorizontalPlotScale::drawUnits(Cairo::RefPtr<Cairo::Context> cairo)
{
	cairo->save();
	cairo->set_font_size(_descriptionFontSize);
	Cairo::TextExtents extents;
	cairo->get_text_extents(_unitsCaption, extents);
	cairo->move_to(_verticalScaleWidth + 0.3 * _plotWidth,
								 _topMargin + _plotHeight + _height - extents.y_bearing - extents.height - 5);
	cairo->show_text(_unitsCaption);
	cairo->stroke();
	cairo->restore();

	// Base of arrow
	cairo->move_to(_verticalScaleWidth + 0.1 * _plotWidth, _topMargin + _plotHeight + _height - 0.5*extents.height - 5);
	cairo->line_to(_verticalScaleWidth + 0.275 * _plotWidth, _topMargin + _plotHeight + _height - 0.5*extents.height - 5);
	cairo->stroke();

	// The arrow
	cairo->move_to(_verticalScaleWidth + 0.275 * _plotWidth, _topMargin + _plotHeight + _height - 0.5*extents.height - 5);
	cairo->line_to(_verticalScaleWidth + 0.25 * _plotWidth, _topMargin + _plotHeight + _height - 0.1*extents.height - 5);
	cairo->line_to(_verticalScaleWidth + 0.26 * _plotWidth, _topMargin + _plotHeight + _height - 0.5*extents.height - 5);
	cairo->line_to(_verticalScaleWidth + 0.25 * _plotWidth, _topMargin + _plotHeight + _height - 0.9*extents.height - 5);
	cairo->close_path();
	cairo->fill();
}

void HorizontalPlotScale::initializeMetrics(Cairo::RefPtr<Cairo::Context> cairo)
{
	if(!_metricsAreInitialized)
	{
		if(_tickSet != 0)
		{
			_tickSet->Reset();
			while(!ticksFit(cairo) && _tickSet->Size()>2)
			{
				_tickSet->DecreaseTicks();
			}
			cairo->set_font_size(_tickValuesFontSize);
			double maxHeight = 0;
			for(unsigned i=0;i!=_tickSet->Size();++i)
			{
				const Tick tick = _tickSet->GetTick(i);
				Cairo::TextExtents extents;
				cairo->get_text_extents(tick.second, extents);
				if(_rotateUnits)
				{
					if(maxHeight < extents.width)
						maxHeight = extents.width;
				} else {
					if(maxHeight < extents.height)
						maxHeight = extents.height;
				}
			}
			if(_rotateUnits)
				_height = maxHeight + 15;
			else
				_height = maxHeight*2 + 10;
			if(_drawWithDescription)
			{
				cairo->set_font_size(_descriptionFontSize);
				Cairo::TextExtents extents;
				cairo->get_text_extents(_unitsCaption, extents);
				_height += extents.height;
			}
			
			if(_tickSet->Size() != 0)
			{
				cairo->set_font_size(_tickValuesFontSize);
				Cairo::TextExtents extents;
				cairo->get_text_extents(_tickSet->GetTick(_tickSet->Size()-1).second, extents);
				
				/// TODO this is TOO MUCH, caption is often not in the rightmost position.
				_rightMargin = extents.width/2+5 > 10 ? extents.width/2+5 : 10;
			} else {
				_rightMargin = 0.0;
			}
			
			_metricsAreInitialized = true;
		} else {
			_rightMargin = 0.0;
			_height = 0.0;
		}
	}
} 

void HorizontalPlotScale::InitializeNumericTicks(double min, double max)
{
	if(_tickSet != 0)
		delete _tickSet;
	_tickSet = new NumericTickSet(min, max, 25);
	_isLogarithmic = false;
	_metricsAreInitialized = false;
}

void HorizontalPlotScale::InitializeLogarithmicTicks(double min, double max)
{
	if(_tickSet == 0)
		delete _tickSet;
	_tickSet = new LogarithmicTickSet(min, max, 25);
	_isLogarithmic = true;
	_metricsAreInitialized = false;
}

void HorizontalPlotScale::InitializeTimeTicks(double timeMin, double timeMax)
{
	if(_tickSet != 0)
		delete _tickSet;
	_tickSet = new TimeTickSet(timeMin, timeMax, 25);
	_isLogarithmic = false;
	_metricsAreInitialized = false;
}

void HorizontalPlotScale::InitializeTextTicks(const std::vector<std::string> &labels)
{
	if(_tickSet != 0)
		delete _tickSet;
	_tickSet = new TextTickSet(labels, 100);
	_isLogarithmic = false;
	_metricsAreInitialized = false;
}

bool HorizontalPlotScale::ticksFit(Cairo::RefPtr<Cairo::Context> cairo)
{
	cairo->set_font_size(16.0);
	double prevEndX = 0.0;
	for(unsigned i=0;i!=_tickSet->Size();++i)
	{
		const Tick tick = _tickSet->GetTick(i);
		Cairo::TextExtents extents;
		cairo->get_text_extents(tick.second + "M", extents);
		const double
			midX = tick.first * (_plotWidth - _verticalScaleWidth) + _verticalScaleWidth;
		double startX, endX;
		if(_rotateUnits)
		{
			// Use "M" to get at least an "M" of distance between axis
			startX = midX - extents.height/2,
			endX = startX + extents.height;
		} else
		{
			// Use "M" to get at least an "M" of distance between ticks
			cairo->get_text_extents(tick.second + "M", extents);
			startX = midX - extents.width/2,
			endX = startX + extents.width;
		}
		if(startX < prevEndX && i!=0)
			return false;
		prevEndX = endX;
	}
	return true;
}
