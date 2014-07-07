#ifndef GUI_PLOT_TITLE
#define GUI_PLOT_TITLE

#include <gtkmm/drawingarea.h>

class Title
{
	public:
		Title() : _metricsAreInitialized(false), _fontSize(16)
		{
		}
		void SetPlotDimensions(double plotWidth, double plotHeight, double topMargin)
		{
			_plotWidth = plotWidth;
			_plotHeight = plotHeight;
			_topMargin = topMargin;
		}
		double GetHeight(Cairo::RefPtr<Cairo::Context> &cairo)
		{
			initializeMetrics(cairo);
			return _height;
		}
		void Draw(Cairo::RefPtr<Cairo::Context> &cairo);
		
		void SetText(const std::string &text)
		{
			_metricsAreInitialized = false;
			_text = text;
		}
		const std::string &Text() { return _text; }
		
	private:
		void initializeMetrics(Cairo::RefPtr<Cairo::Context> &cairo);
		
		bool _metricsAreInitialized;
		double _plotWidth, _plotHeight, _topMargin;
		double _fontSize, _height;
		std::string _text;
};

#endif
