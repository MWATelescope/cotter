#ifndef COTTER_H
#define COTTER_H

#include "averagingmswriter.h"
#include "gpufilereader.h"
#include "mwaconfig.h"

#include <boost/thread/mutex.hpp>

#include <vector>
#include <queue>
#include <string>

namespace aoflagger {
	class AOFlagger;
	class FlagMask;
	class ImageSet;
	class QualityStatistics;
	class Strategy;
}

class GPUFileReader;
class MSWriter;

class Cotter : private UVWCalculater
{
	public:
		Cotter();
		~Cotter();
		
		void Run(const char *outputFilename, size_t timeAvgFactor, size_t freqAvgFactor);
		
		void SetFileSets(const std::vector<std::vector<std::string> >& fileSets) { _fileSets = fileSets; }
		void SetThreadCount(size_t threadCount) { _threadCount = threadCount; }
		
	private:
		MWAConfig _mwaConfig;
		Writer *_writer;
		GPUFileReader *_reader;
		aoflagger::AOFlagger *_flagger;
		aoflagger::Strategy *_strategy;
		
		std::vector<double> _subbandCorrectionFactors[4];
		
		std::vector<std::vector<std::string> > _fileSets;
		size_t _threadCount;
		size_t _subbandCount;
		size_t _quackSampleCount;
		
		std::map<std::pair<size_t, size_t>, aoflagger::ImageSet*> _imageSetBuffers;
		std::map<std::pair<size_t, size_t>, aoflagger::FlagMask*> _flagBuffers;
		std::vector<double> _channelFrequenciesHz;
		std::vector<double> _scanTimes;
		std::queue<std::pair<size_t,size_t> > _baselinesToProcess;
		
		boost::mutex _mutex;
		aoflagger::QualityStatistics *_statistics;
		
		void baselineProcessThreadFunc();
		void processBaseline(size_t antenna1, size_t antenna2, aoflagger::QualityStatistics &statistics);
		void correctConjugated(aoflagger::ImageSet& imageSet, size_t imageIndex);
		void correctCableLength(aoflagger::ImageSet& imageSet, size_t polarization, double cableDelay);
		void writeAntennae();
		void writeSPW();
		void writeField();
		void readSubbandPassbandFile();
		void flagBadCorrelatorSamples(aoflagger::FlagMask &flagMask);
		
		// Implementing UVWCalculater
		virtual void CalculateUVW(double date, size_t antenna1, size_t antenna2, double &u, double &v, double &w);

		Cotter(const Cotter&) { }
		void operator=(const Cotter&) { }
};

#endif
