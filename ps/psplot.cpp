#include "plot/pswidget.h"
#include "fitsreader.h"

#include <gtkmm/main.h>
#include <gtkmm/window.h>

#include <string>
#include <iostream>

class PSWindow : public Gtk::Window {
	public:
    PSWindow()
		{
			add(_psWidget);
			_psWidget.show();
		}
		
		void OpenPath(const std::string& filename)
		{
			FitsReader reader(filename);
			ao::uvector<double> image(reader.ImageWidth() * reader.ImageHeight());
			double minX, minY, maxX, maxY;
			bool kwExists = reader.ReadDoubleKeyIfExists("PSXMIN", minX);
			if(!reader.ReadDoubleKeyIfExists("PSYMIN", minY))
				kwExists = false;
			if(!reader.ReadDoubleKeyIfExists("PSXMAX", maxX))
				kwExists = false;
			if(!reader.ReadDoubleKeyIfExists("PSYMAX", maxY))
				kwExists = false;
			if(!kwExists)
				std::cout << "Warning: PS range keywords missing -- axes won't have proper units.\n";
			else {
				_psWidget.SetXAxisMin(minX);
				_psWidget.SetYAxisMin(minY);
				_psWidget.SetXAxisMax(maxX);
				_psWidget.SetYAxisMax(maxY);
			}
			reader.Read(image.data());
			_psWidget.SetImage(image.data(), reader.ImageWidth(), reader.ImageHeight());
		}
		
	private:
		PSWidget _psWidget;
};

int main(int argc, char* argv[])
{
	if(argc!=2)
		std::cout << "Syntax: psplot [name]\n";
	else {
		Gtk::Main kit(argc, argv);
		PSWindow window;
		window.present();
		window.OpenPath(argv[1]);
		kit.run(window);
	}
}
