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
#ifndef PSWIDGET_H
#define PSWIDGET_H

#include <gtkmm/drawingarea.h>

#include <cairomm/surface.h>

#include <vector>
#include "../aocommon/uvector.h"

class ColorMap;

/**
	@author A.R. Offringa <offringa@astro.rug.nl>
*/
class PSWidget : public Gtk::DrawingArea {
	public:
		enum CMap { BWMap, InvertedMap, HotColdMap, RedBlueMap, RedYellowBlueMap, BlackRedMap };
		enum Range { MinMax, Specified };
		
		PSWidget();
		~PSWidget();

		CMap GetColorMap() const { return _colorMap; }
		void SetColorMap(CMap colorMap) { _colorMap = colorMap; }
		
		void SetRange(enum Range range)
		{
			_range = range;
		}
		enum Range Range() const
		{
			return _range;
		}
		void SetLogZScale(bool logZScale)
		{
			_logZScale = logZScale;
		}
		bool LogZScale() const { return _logZScale; }
		
		void Update(); 

		void SetXRange(size_t start, size_t end)
		{
			_xStart = start;
			_xEnd = end;
		}
		void SetYRange(size_t start, size_t end)
		{
			_yStart = start;
			_yEnd = end;
		}
		double XStart() const { return _xStart; }
		double XEnd() const { return _xEnd; }
		double YStart() const { return _yStart; }
		double YEnd() const { return _yEnd; }
		
		double Max() const { return _max; }
		double Min() const { return _min; }
		
		void SetMax(double max) { _max = max; }
		void SetMin(double min) { _min = min; }
		
		void SavePdf(const std::string &filename);
		void SaveSvg(const std::string &filename);
		void SavePng(const std::string &filename);
		
		bool ShowTitle() const { return _showTitle; }
		void SetShowTitle(bool showTitle) {
			_showTitle = showTitle;
		}
		
		bool ShowXYAxes() const { return _showXYAxes; }
		void SetShowXYAxes(bool showXYAxes)
		{
			_showXYAxes = showXYAxes;
		}
		
		bool ShowColorScale() const { return _showColorScale; }
		void SetShowColorScale(bool showColorScale)
		{
			_showColorScale = showColorScale;
		}
		
		bool ShowXAxisDescription() const { return _showXAxisDescription; }
		void SetShowXAxisDescription(bool showXAxisDescription)
		{
			_showXAxisDescription = showXAxisDescription;
		}
		
		bool ShowYAxisDescription() const { return _showYAxisDescription; }
		void SetShowYAxisDescription(bool showYAxisDescription)
		{
			_showYAxisDescription = showYAxisDescription;
		}
		
		bool ShowZAxisDescription() const { return _showZAxisDescription; }
		void SetShowZAxisDescription(bool showZAxisDescription)
		{
			_showZAxisDescription = showZAxisDescription;
		}
		
		void Clear();
		
		void SetCairoFilter(Cairo::Filter filter)
		{
			_cairoFilter = filter;
		}
		void SetTitleText(const std::string& title)
		{
			_titleText = title;
		}
		void SetXAxisDescription(const std::string& description)
		{
			_xAxisDescription = description;
		}
		void SetYAxisDescription(const std::string& description)
		{
			_yAxisDescription = description;
		}
		void SetZAxisDescription(const std::string& description)
		{
			_zAxisDescription = description;
		}
		
		bool ManualTitle() const { return _manualTitle; }
		void SetManualTitle(bool manualTitle) { _manualTitle = manualTitle; }
		
		const std::string &ManualTitleText() {
			return _manualTitleText;
		}
		void SetManualTitleText(const std::string &manualTitle) {
			_manualTitleText = manualTitle;
		}
		
		bool ManualXAxisDescription() const { return _manualXAxisDescription; }
		void SetManualXAxisDescription(bool manualDesc)
		{
			_manualXAxisDescription = manualDesc;
		}
		bool ManualYAxisDescription() const { return _manualYAxisDescription; }
		void SetManualYAxisDescription(bool manualDesc)
		{
			_manualYAxisDescription = manualDesc;
		}
		bool ManualZAxisDescription() const { return _manualZAxisDescription; }
		void SetManualZAxisDescription(bool manualDesc)
		{
			_manualZAxisDescription = manualDesc;
		}
		bool HasImage() const { return !_image.empty(); }
		void SetImage(const double* image, size_t width, size_t height)
		{
			_image.resize(width*height);
			memcpy(_image.data(), image, width*height*sizeof(double));
			_imageWidth = width;
			_xStart = 0;
			_xEnd = width;
			_yStart = 0;
			_yEnd = height;
		}
		void SetXAxisMin(double xAxisMin) { _xAxisMin = xAxisMin; }
		void SetXAxisMax(double xAxisMax) { _xAxisMax = xAxisMax; }
		void SetYAxisMin(double yAxisMin) { _yAxisMin = yAxisMin; }
		void SetYAxisMax(double yAxisMax) { _yAxisMax = yAxisMax; }
	private:
		void findMinMax(double& min, double& max);
		ColorMap* createColorMap();
		void update(Cairo::RefPtr<Cairo::Context> cairo, unsigned width, unsigned height);
		void redrawWithoutChanges(Cairo::RefPtr<Cairo::Context> cairo, unsigned width, unsigned height);
		bool onDraw(const Cairo::RefPtr<Cairo::Context>& cr);
		std::string actualTitleText() const
		{
			if(_manualTitle)
				return _manualTitleText;
			else
				return _titleText;
		}

		bool _isInitialized;
		unsigned _initializedWidth, _initializedHeight;
		Cairo::RefPtr<Cairo::ImageSurface> _imageSurface;

		enum CMap _colorMap;
		
		double _leftBorderSize, _rightBorderSize, _topBorderSize, _bottomBorderSize;

		size_t _xStart, _xEnd;
		size_t _yStart, _yEnd;
		class HorizontalPlotScale *_horiScale;
		class VerticalPlotScale *_vertScale;
		class ColorScale *_colorScale;
		class Title *_plotTitle;
		bool _logZScale;
		bool _showXYAxes;
		bool _showColorScale;
		bool _showXAxisDescription;
		bool _showYAxisDescription;
		bool _showZAxisDescription;
		bool _showTitle;
		double _max, _min;
		double _xAxisMin, _xAxisMax, _yAxisMin, _yAxisMax;
		std::string _titleText, _manualTitleText;
		enum Range _range;
		Cairo::Filter _cairoFilter;
		std::string _xAxisDescription, _yAxisDescription, _zAxisDescription;
		bool _manualTitle;
		bool _manualXAxisDescription;
		bool _manualYAxisDescription;
		bool _manualZAxisDescription;
		
		ao::uvector<double> _image;
		size_t _imageWidth;
};

#endif
