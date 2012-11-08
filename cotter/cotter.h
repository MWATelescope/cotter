#ifndef COTTER_H
#define COTTER_H

#include <vector>
#include <string>

#include "gpufilereader.h"
#include "mwaconfig.h"

namespace aoflagger {
	class ImageSet;
}

class MSWriter;

class Cotter
{
	public:
		Cotter() { }
		
		void Run();
		
		void SetFileSets(const std::vector<std::vector<std::string> >& fileSets) {
			_fileSets = fileSets;
		}
		
	private:
		MWAConfig _mwaConfig;
		MSWriter *_msWriter;
		std::vector<std::vector<std::string> > _fileSets;
		
		void correctConjugated(aoflagger::ImageSet& imageSet, size_t imageIndex);
		void correctCableLength(aoflagger::ImageSet& imageSet, size_t polarization, double cableDelay, const double *channelFrequencies);
		void writeAntennae();
		void writeSPW(const double *channelFrequencies);
		void writeField();

		Cotter(const Cotter&) { }
		void operator=(const Cotter&) { }
};

#endif
