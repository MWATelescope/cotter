#include "title.h"

void Title::Draw(Cairo::RefPtr< Cairo::Context >& cairo)
{
	initializeMetrics(cairo);

	cairo->set_source_rgb(0.0, 0.0, 0.0);
	cairo->set_font_size(_fontSize);
	
	Cairo::TextExtents extents;
	cairo->get_text_extents(_text, extents);
	cairo->move_to(_plotWidth/2.0 - extents.width/2.0, _topMargin+extents.height+6);
	cairo->show_text(_text);
}

void Title::initializeMetrics(Cairo::RefPtr< Cairo::Context >& cairo)
{
	if(!_metricsAreInitialized)
	{
		cairo->set_font_size(_fontSize);
		Cairo::TextExtents extents;
		cairo->get_text_extents(_text, extents);
		_height = extents.height + 12;
		
		_metricsAreInitialized = true;
	}
}
