#include "pswidget.h"

#include "colormap.h"
#include "colorscale.h"
#include "horizontalplotscale.h"
#include "verticalplotscale.h"
#include "title.h"

PSWidget::PSWidget() :
	_isInitialized(false),
	_initializedWidth(0),
	_initializedHeight(0),
	_colorMap(HotColdMap),
	_xStart(0),
	_xEnd(0),
	_yStart(0),
	_yEnd(0),
	_horiScale(0),
	_vertScale(0),
	_colorScale(0),
	_plotTitle(0),
	_logZScale(true),
	_showXYAxes(true),
	_showColorScale(true),
	_showXAxisDescription(true),
	_showYAxisDescription(true),
	_showZAxisDescription(true),
	_showTitle(true),
	_max(1.0), _min(0.0),
	_range(MinMax),
	_cairoFilter(Cairo::FILTER_BEST),
	_manualTitle(false),
	_manualXAxisDescription(false),
	_manualYAxisDescription(false),
	_manualZAxisDescription(false)
{
	signal_draw().connect(sigc::mem_fun(*this, &PSWidget::onDraw) );
}

PSWidget::~PSWidget()
{
	Clear();
}

void PSWidget::Clear()
{
	if(_horiScale != 0) {
		delete _horiScale;
		_horiScale = 0;
	}
	if(_vertScale != 0) {
		delete _vertScale;
		_vertScale = 0;
	}
	if(_colorScale != 0) {
		delete _colorScale;
		_colorScale = 0;
	}
	if(_plotTitle != 0) {
		delete _plotTitle;
		_plotTitle = 0;
	}
}

bool PSWidget::onDraw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	if(get_width() == (int) _initializedWidth && get_height() == (int) _initializedHeight)
		redrawWithoutChanges(get_window()->create_cairo_context(), get_width(), get_height());
	else
		Update();
	return true;
}

void PSWidget::Update()
{
  if(HasImage())
	{
		Glib::RefPtr<Gdk::Window> window = get_window();
		if(window != 0 && get_width() > 0 && get_height() > 0)
			update(window->create_cairo_context(), get_width(), get_height());
	}
}

void PSWidget::SavePdf(const std::string &filename)
{
	unsigned width = get_width(), height = get_height();
	Cairo::RefPtr<Cairo::PdfSurface> surface = Cairo::PdfSurface::create(filename, width, height);
	Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
	if(HasImage())
	{
		update(cairo, width, height);
	}
	cairo->show_page();
	// We finish the surface. This might be required, because some of the subclasses store the cairo context. In that
	// case, it won't be written.
	surface->finish();
}

void PSWidget::SaveSvg(const std::string &filename)
{
	unsigned width = get_width(), height = get_height();
	Cairo::RefPtr<Cairo::SvgSurface> surface = Cairo::SvgSurface::create(filename, width, height);
	Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
	if(HasImage())
	{
		update(cairo, width, height);
	}
	cairo->show_page();
	surface->finish();
}

void PSWidget::SavePng(const std::string &filename)
{
	unsigned width = get_width(), height = get_height();
	Cairo::RefPtr<Cairo::ImageSurface> surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, width, height);
	Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
	if(HasImage())
	{
		update(cairo, width, height);
	}
	surface->write_to_png(filename);
}

void PSWidget::update(Cairo::RefPtr<Cairo::Context> cairo, unsigned width, unsigned height)
{
	size_t
		imageWidth = _xEnd - _xStart,
		imageHeight = _yEnd - _yStart;
		
	double min, max;
	findMinMax(min, max);
	
	delete _horiScale;
	delete _vertScale;
	delete _colorScale;
	delete _plotTitle;
		
	if(_showXYAxes)
	{
		_vertScale = new VerticalPlotScale();
		_vertScale->SetDrawWithDescription(_showYAxisDescription);
		_horiScale = new HorizontalPlotScale();
		_horiScale->SetDrawWithDescription(_showXAxisDescription);
	} else {
		_vertScale = 0;
		_horiScale = 0;
	}
	if(_showColorScale)
	{
		_colorScale = new ColorScale();
		_colorScale->SetDrawWithDescription(_showZAxisDescription);
	} else {
		_colorScale = 0;
	}
	if(_showXYAxes)
	{
		_vertScale->InitializeLogarithmicTicks(0.1, 10.0);
		_vertScale->SetUnitsCaption("k\u2225 (h/Mpc)\u00B3");
	
		_horiScale->InitializeLogarithmicTicks(0.1, 10.0);
		_horiScale->SetUnitsCaption("k\u22A5 (h/Mpc)\u00B3"); // \u27C2 is official perp symbol
		
		if(_manualXAxisDescription)
			_horiScale->SetUnitsCaption(_xAxisDescription);
		if(_manualYAxisDescription)
			_vertScale->SetUnitsCaption(_yAxisDescription);
	}
	if(_showColorScale)
	{
		if(_manualZAxisDescription)
			_colorScale->SetUnitsCaption(_zAxisDescription);
		else
			_colorScale->SetUnitsCaption("mK");
		if(_logZScale)
			_colorScale->InitializeLogarithmicTicks(min, max);
		else
			_colorScale->InitializeNumericTicks(min, max);
	}

	if(_showTitle && !actualTitleText().empty())
	{
		_plotTitle = new Title();
		_plotTitle->SetText(actualTitleText());
		_plotTitle->SetPlotDimensions(width, height, 0.0);
		_topBorderSize = _plotTitle->GetHeight(cairo);
	} else {
		_plotTitle = 0;
		_topBorderSize = 10.0;
	}
	// The scale dimensions are depending on each other. However, since the height of the horizontal scale is practically
	// not dependent on other dimensions, we give the horizontal scale temporary width/height, so that we can calculate its height:
	if(_showXYAxes)
	{
		_horiScale->SetPlotDimensions(width, height, 0.0, 0.0);
		_bottomBorderSize = _horiScale->GetHeight(cairo);
		_rightBorderSize = _horiScale->GetRightMargin(cairo);
	
		_vertScale->SetPlotDimensions(width - _rightBorderSize + 5.0, height - _topBorderSize - _bottomBorderSize, _topBorderSize);
		_leftBorderSize = _vertScale->GetWidth(cairo);
	} else {
		_bottomBorderSize = 0.0;
		_rightBorderSize = 0.0;
		_leftBorderSize = 0.0;
	}
	if(_showColorScale)
	{
		_colorScale->SetPlotDimensions(width - _rightBorderSize, height - _topBorderSize, _topBorderSize);
		_rightBorderSize += _colorScale->GetWidth(cairo) + 5.0;
	}
	if(_showXYAxes)
	{
		_horiScale->SetPlotDimensions(width - _rightBorderSize + 5.0, height -_topBorderSize - _bottomBorderSize, _topBorderSize, 	_vertScale->GetWidth(cairo));
	}

	class ColorMap *colorMap = createColorMap();
	
	const double
		minLog10 = min>0.0 ? log10(min) : 0.0,
		maxLog10 = max>0.0 ? log10(max) : 0.0;
	if(_showColorScale)
	{
		for(unsigned x=0;x<256;++x)
		{
			double colorVal = (2.0 / 256.0) * x - 1.0;
			double imageVal;
			if(_logZScale)
				imageVal = exp10((x / 256.0) * (log10(max) - minLog10) + minLog10);
			else 
				imageVal = (max-min) * x / 256.0 + min;
			double
				r = colorMap->ValueToColorR(colorVal),
				g = colorMap->ValueToColorG(colorVal),
				b = colorMap->ValueToColorB(colorVal);
			_colorScale->SetColorValue(imageVal, r/255.0, g/255.0, b/255.0);
		}
	}
	
	_imageSurface.clear();
	_imageSurface =
		Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, imageWidth, imageHeight);

	_imageSurface->flush();
	unsigned char *data = _imageSurface->get_data();
	size_t rowStride = _imageSurface->get_stride();

	for(unsigned long y=_yStart; y<_yEnd; ++y) {
		
		guint8* rowpointer = data + rowStride * (_yEnd - y - 1);
		double* imageRow = &_image[y * _imageWidth];
		
		for(unsigned long x=_xStart; x<_xEnd; ++x) {
			int xa = (x-_xStart) * 4;
			unsigned char r,g,b,a;
			double val = imageRow[x];
			if(val > max) val = max;
			else if(val < min) val = min;

			if(_logZScale)
			{
				if(val <= 0.0)
					val = -1.0;
				else
					val = (log10(val) - minLog10) * 2.0 / (maxLog10 - minLog10) - 1.0;
			}
			else
				val = (val - min) * 2.0 / (max - min) - 1.0;
			if(val < -1.0) val = -1.0;
			else if(val > 1.0) val = 1.0;
			r = colorMap->ValueToColorR(val);
			g = colorMap->ValueToColorG(val);
			b = colorMap->ValueToColorB(val);
			a = colorMap->ValueToColorA(val);
			rowpointer[xa]=b;
			rowpointer[xa+1]=g;
			rowpointer[xa+2]=r;
			rowpointer[xa+3]=a;
		}
	}
	delete colorMap;

	_imageSurface->mark_dirty();

	_isInitialized = true;
	_initializedWidth = width;
	_initializedHeight = height;
	redrawWithoutChanges(cairo, width, height);
} 

ColorMap *PSWidget::createColorMap()
{
	switch(_colorMap) {
		case BWMap:
			return new MonochromeMap();
		case InvertedMap:
			return new class InvertedMap();
		case HotColdMap:
			return new ColdHotMap();
		case RedBlueMap:
			return new class RedBlueMap();
		case RedYellowBlueMap:
			return new class RedYellowBlueMap();
		case BlackRedMap:
			return new class BlackRedMap();
		default:
			return 0;
	}
}

void PSWidget::findMinMax(double& min, double& max)
{
	switch(_range)
	{
		case MinMax:
			max = std::numeric_limits<double>::min();
			min = std::numeric_limits<double>::max();
			for(ao::uvector<double>::const_iterator i=_image.begin(); i!=_image.end(); ++i)
			{
				if(max < *i) max = *i;
				if(min > *i) min = *i;
			}
		break;
		case Specified:
			min = _min;
			max = _max;
		break;
		default:
			min = -1.0;
			max = 1.0;
		break;
	}
	if(min == max)
	{
		min -= 1.0;
		max += 1.0;
	}
	if(_logZScale && min<=0.0)
	{
		if(max <= 0.0)
			max = 1.0;
		
		min = max / 10000.0;
	}
	_max = max;
	_min = min;
}

void PSWidget::redrawWithoutChanges(Cairo::RefPtr<Cairo::Context> cairo, unsigned width, unsigned height)
{
	if(_isInitialized) {
		cairo->set_source_rgb(1.0, 1.0, 1.0);
		cairo->set_line_width(1.0);
		cairo->rectangle(0, 0, width, height);
		cairo->fill();
		
		int
			destWidth = width - (int) floor(_leftBorderSize + _rightBorderSize),
			destHeight = height - (int) floor(_topBorderSize + _bottomBorderSize),
			sourceWidth = _imageSurface->get_width(),
			sourceHeight = _imageSurface->get_height();
		cairo->save();
		cairo->translate((int) round(_leftBorderSize), (int) round(_topBorderSize));
		cairo->scale((double) destWidth / (double) sourceWidth, (double) destHeight / (double) sourceHeight);
		Cairo::RefPtr<Cairo::SurfacePattern> pattern = Cairo::SurfacePattern::create(_imageSurface);
		pattern->set_filter(_cairoFilter);
		cairo->set_source(pattern);
		cairo->rectangle(0, 0, sourceWidth, sourceHeight);
		cairo->clip();
		cairo->paint();
		cairo->restore();
		cairo->set_source_rgb(0.0, 0.0, 0.0);
		cairo->rectangle(round(_leftBorderSize), round(_topBorderSize), destWidth, destHeight);
		cairo->stroke();

		if(_showColorScale)
			_colorScale->Draw(cairo);
		if(_showXYAxes)
		{
			_vertScale->Draw(cairo);
			_horiScale->Draw(cairo);
		}
		if(_plotTitle != 0)
			_plotTitle->Draw(cairo);
	}
}
