#ifndef GNUPLOT_H
#define GNUPLOT_H

#include <fstream>
#include <string>
#include <vector>

class GNUPlot
{
public:
	class Line
	{
	public:
		friend class GNUPlot;
		void AddPoint(double x, double y)
		{
			_file.precision(16);
			_file << x << '\t' << y << '\n';
		}
	private:
		Line(const std::string& filename) : _file(filename)
		{ }
		
		std::ofstream _file;
	};
	
	GNUPlot(const std::string& filenamePrefix, const std::string& xLabel, const std::string& yLabel, bool yLog=false) : _file(filenamePrefix + ".plt")
	{
		_file <<
			"set terminal postscript enhanced color\n";
		if(yLog)
			_file << "set logscale y\n";
		_file <<
			"#set xrange [0.1:]\n"
			"#set yrange [0.1:]\n"
			"set output \"" << filenamePrefix << ".ps\"\n"
			"#set key bottom left\n"
			"set xlabel \"" << xLabel << "\"\n"
			"set ylabel \"" << yLabel << "\"\n";
	}
	
	~GNUPlot()
	{
		_file << '\n';
		for(std::vector<Line*>::iterator i=_lines.begin(); i!=_lines.end(); ++i)
			delete *i;
	}
	
	Line* AddLine(const std::string& filename, const std::string& caption)
	{
		if(_lines.empty())
			_file << "plot \\\n";
		else
			_file << ",\\\n";
		_file << "\"" << filename << "\" using 1:2 with lines title '" << caption << "' lw 2.0";
		_lines.push_back(new Line(filename));
		return _lines.back();
	}
	
private:
	std::vector<Line*> _lines;
	std::ofstream _file;
};

#endif
