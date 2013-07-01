#ifndef COTTER_H
#define COTTER_H

#include "averagingmswriter.h"
#include "gpufilereader.h"
#include "mwaconfig.h"
#include "stopwatch.h"
#include "progressbar.h"

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
		enum OutputFormat { MSOutputFormat, FitsOutputFormat, FlagsOutputFormat };
		
		Cotter();
		~Cotter();
		
		void Run(const char *outputFilename, size_t timeAvgFactor, size_t freqAvgFactor);
		
		void SetOutputFormat(enum OutputFormat format) { _outputFormat = format; }
		void SetFileSets(const std::vector<std::vector<std::string> >& fileSets) { _fileSets = fileSets; }
		void SetThreadCount(size_t threadCount) { _threadCount = threadCount; }
		void SetRFIDetection(bool performRFIDetection) { _rfiDetection = performRFIDetection; }
		void SetCollectStatistics(bool collectStatistics) { _collectStatistics = collectStatistics; }
		void SetHistoryInfo(const std::string &commandLine) { _commandLine = commandLine; }
		void SetMetaFilename(const char *metaFilename) { _metaFilename = metaFilename; }
		void SetMaxBufferSize(const size_t bufferSizeInSamples) { _maxBufferSize = bufferSizeInSamples; }
		void SetDisableGeometricCorrections(bool disableCorrections) { _disableGeometricCorrections = disableCorrections; }
		void SetOverridePhaseCentre(long double newRARad, long double newDecRad)
		{
			_overridePhaseCentre = true;
			_customRARad = newRARad;
			_customDecRad = newDecRad;
		}
		void SetSubbandCount(size_t subbandCount) { _subbandCount = subbandCount; }
		void SetRemoveFlaggedAntennae(bool removeFlaggedAntennae) { _removeFlaggedAntennae = removeFlaggedAntennae; }
		void SetRemoveAutoCorrelations(bool removeAutoCorrelations) { _removeAutoCorrelations = removeAutoCorrelations; }
		void SetReadSubbandPassbandFile(const std::string subbandPassbandFilename)
		{
			_subbandPassbandFilename = subbandPassbandFilename;
		}
		void SetFlagAutoCorrelations(bool flagAutoCorrelations) { _flagAutos = flagAutoCorrelations; }
		void SetInitDurationToFlag(double initDuration) { _initDurationToFlag = initDuration; }
		void FlagAntenna(size_t antIndex) { _userFlaggedAntennae.push_back(antIndex); }
	private:
		MWAConfig _mwaConfig;
		Writer *_writer;
		GPUFileReader *_reader;
		aoflagger::AOFlagger *_flagger;
		aoflagger::Strategy *_strategy;
		
		std::vector<double> _subbandCorrectionFactors[4];
		std::vector<double> _subbandGainCorrection;
		bool *_isAntennaFlaggedMap;
		size_t _unflaggedAntennaCount;
		
		Stopwatch _readWatch, _processWatch, _writeWatch;
		
		std::vector<std::vector<std::string> > _fileSets;
		size_t _threadCount;
		size_t _maxBufferSize;
		size_t _subbandCount;
		size_t _quackSampleCount;
		size_t _missingEndScans;
		size_t _curChunkStart, _curChunkEnd;
		bool _rfiDetection, _collectStatistics;
		enum OutputFormat _outputFormat;
		std::string _commandLine;
		std::string _metaFilename;
		std::string _subbandPassbandFilename;
		std::vector<size_t> _userFlaggedAntennae;
		
		std::map<std::pair<size_t, size_t>, aoflagger::ImageSet*> _imageSetBuffers;
		std::map<std::pair<size_t, size_t>, aoflagger::FlagMask*> _flagBuffers;
		std::vector<double> _channelFrequenciesHz;
		std::vector<double> _scanTimes;
		std::queue<std::pair<size_t,size_t> > _baselinesToProcess;
		std::auto_ptr<ProgressBar> _progressBar;
		size_t _baselinesToProcessCount;
		std::vector<size_t> _subbandOrder;
		
		boost::mutex _mutex;
		aoflagger::QualityStatistics *_statistics;
		aoflagger::FlagMask *_correlatorMask, *_fullysetMask;
		
		bool *_outputFlags;
		std::complex<float> *_outputData;
		float *_outputWeights;
		bool _disableGeometricCorrections, _removeFlaggedAntennae, _removeAutoCorrelations, _flagAutos;
		bool _overridePhaseCentre;
		long double _customRARad, _customDecRad;
		double _initDurationToFlag;
		
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
		void writeObservation();
		void initSubbandGainsFromMeta();
		void readSubbandGainsFile();
		void readSubbandPassbandFile();
		void initializeSubbandPassband();
		void flagBadCorrelatorSamples(aoflagger::FlagMask &flagMask) const;
		void initializeWeights(float *outputWeights);
		void reorderSubbands(aoflagger::ImageSet& imageSet) const;
		void initializeSbOrder(size_t centerSbNumber);
		void writeAlignmentScans();
		void writeMWAFields(const char *outputFilename, size_t flagWindowSize);
		size_t rowsPerTimescan() const
		{
			if(_removeFlaggedAntennae && _removeAutoCorrelations)
				return _unflaggedAntennaCount*(_unflaggedAntennaCount-1)/2;
			else if(_removeFlaggedAntennae)
				return _unflaggedAntennaCount*(_unflaggedAntennaCount+1)/2;
			else if(_removeAutoCorrelations)
				return _mwaConfig.NAntennae()*(_mwaConfig.NAntennae()-1)/2;
			else
				return _mwaConfig.NAntennae()*(_mwaConfig.NAntennae()+1)/2;
		}
		bool outputBaseline(size_t antenna1, size_t antenna2) const
		{
			bool output = true;
			if(_removeFlaggedAntennae)
				output = output && (!_isAntennaFlaggedMap[antenna1]) && (!_isAntennaFlaggedMap[antenna2]);
			if(_removeAutoCorrelations)
				output = output && (antenna1 != antenna2);
			return output;
		}
		
		// Implementing UVWCalculater
		virtual void CalculateUVW(double date, size_t antenna1, size_t antenna2, double &u, double &v, double &w);

		Cotter(const Cotter&) { }
		void operator=(const Cotter&) { }
};

#endif
