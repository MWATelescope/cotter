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
