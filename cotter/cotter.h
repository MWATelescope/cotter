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
		void SetRFIDetection(bool performRFIDetection) { _rfiDetection = performRFIDetection; }
		void SetCollectStatistics(bool collectStatistics) { _collectStatistics = collectStatistics; }
		void SetHistoryInfo(const std::string &commandLine) { _commandLine = commandLine; }
		void SetMetaFilename(const char *metaFilename) { _metaFilename = metaFilename; }
		void SetMaxBufferSize(const size_t bufferSizeInSamples) { _maxBufferSize = bufferSizeInSamples; }
		
	private:
		MWAConfig _mwaConfig;
		Writer *_writer;
		GPUFileReader *_reader;
		aoflagger::AOFlagger *_flagger;
		aoflagger::Strategy *_strategy;
		
		std::vector<double> _subbandCorrectionFactors[4];
		std::vector<double> _subbandGainCorrection;
		
		std::vector<std::vector<std::string> > _fileSets;
		size_t _threadCount;
		size_t _maxBufferSize;
		size_t _subbandCount;
		size_t _quackSampleCount;
		size_t _missingEndScans;
		size_t _curChunkStart, _curChunkEnd;
		bool _rfiDetection, _collectStatistics;
		std::string _commandLine;
		std::string _metaFilename;
		
		std::map<std::pair<size_t, size_t>, aoflagger::ImageSet*> _imageSetBuffers;
		std::map<std::pair<size_t, size_t>, aoflagger::FlagMask*> _flagBuffers;
		std::vector<double> _channelFrequenciesHz;
		std::vector<double> _scanTimes;
		std::queue<std::pair<size_t,size_t> > _baselinesToProcess;
		std::vector<size_t> _subbandOrder;
		
		boost::mutex _mutex;
		aoflagger::QualityStatistics *_statistics;
		aoflagger::FlagMask *_correlatorMask, *_fullysetMask;
		
		bool *_outputFlags;
		std::complex<float> *_outputData;
		float *_outputWeights;
		
		void createReader(const std::vector<std::string> &curFileset);
		void initializeReader();
		void processAndWriteTimestep(size_t timeIndex);
		void baselineProcessThreadFunc();
		void processBaseline(size_t antenna1, size_t antenna2, aoflagger::QualityStatistics &statistics);
		void correctConjugated(aoflagger::ImageSet& imageSet, size_t imageIndex) const;
		void correctCableLength(aoflagger::ImageSet& imageSet, size_t polarization, double cableDelay) const;
		void writeAntennae();
		void writeSPW();
		void writeSource();
		void writeField();
		void readSubbandGainsFile();
		void readSubbandPassbandFile();
		void flagBadCorrelatorSamples(aoflagger::FlagMask &flagMask) const;
		void initializeWeights(float *outputWeights);
		void reorderSubbands(aoflagger::ImageSet& imageSet) const;
		void initializeSbOrder(size_t centerSbNumber);
		void writeAlignmentScans();
		
		// Implementing UVWCalculater
		virtual void CalculateUVW(double date, size_t antenna1, size_t antenna2, double &u, double &v, double &w);

		Cotter(const Cotter&) { }
		void operator=(const Cotter&) { }
};

#endif
