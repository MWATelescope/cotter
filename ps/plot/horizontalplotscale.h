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
#ifndef HORIZONTALPLOTSCALE_H
#define HORIZONTALPLOTSCALE_H

#include <string>
#include <vector>

#include <gtkmm/drawingarea.h>

/**
	@author A.R. Offringa <offringa@astro.rug.nl>
*/
class HorizontalPlotScale {
	public:
		HorizontalPlotScale();
		virtual ~HorizontalPlotScale();
		void SetPlotDimensions(double plotWidth, double plotHeight, double topMargin, double verticalScaleWidth)
		{
			_plotWidth = plotWidth;
			_plotHeight = plotHeight;
			_topMargin = topMargin;
			_verticalScaleWidth = verticalScaleWidth;
			_metricsAreInitialized = false;
		}
		double GetHeight(Cairo::RefPtr<Cairo::Context> cairo);
		double GetRightMargin(Cairo::RefPtr<Cairo::Context> cairo);
		void Draw(Cairo::RefPtr<Cairo::Context> cairo);
		void InitializeNumericTicks(double min, double max);
		void InitializeTimeTicks(double timeMin, double timeMax);
		void InitializeTextTicks(const std::vector<std::string> &labels);
		void InitializeLogarithmicTicks(double min, double max);
		void SetDrawWithDescription(bool drawWithDescription)
		{
			_drawWithDescription = drawWithDescription;
			_metricsAreInitialized = false;
		}
		void SetUnitsCaption(const std::string &caption)
		{
			_unitsCaption = caption;
			_metricsAreInitialized = false;
		}
		void SetDescriptionFontSize(double fontSize)
		{
			_tickValuesFontSize = fontSize;
			_metricsAreInitialized = false;
		}
		void SetTickValuesFontSize(double fontSize)
		{
			_tickValuesFontSize = fontSize;
			_metricsAreInitialized = false;
		}
		void SetRotateUnits(bool rotate)
		{
			_rotateUnits = rotate;
			_metricsAreInitialized = false;
		}
	private:
		void drawUnits(Cairo::RefPtr<Cairo::Context> cairo);
		bool ticksFit(Cairo::RefPtr<Cairo::Context> cairo);
		void initializeMetrics(Cairo::RefPtr<Cairo::Context> cairo); 

		double _plotWidth, _plotHeight, _topMargin, _verticalScaleWidth;
		bool _metricsAreInitialized;
		double _height, _rightMargin;
		class TickSet *_tickSet;
		bool _drawWithDescription;
		std::string _unitsCaption;
		double _descriptionFontSize;
		double _tickValuesFontSize;
		bool _rotateUnits, _isLogarithmic;
};

#endif
