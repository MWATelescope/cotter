#ifndef COTTER_H
#define COTTER_H

#include "aligned_ptr.h"
#include "averagingwriter.h"
#include "gpufilereader.h"
#include "mwaconfig.h"
#include "stopwatch.h"
#include "progressbar.h"

#include <aoflagger.h>

#include <memory>
#include <vector>
#include <queue>
#include <set>
#include <string>

class GPUFileReader;
class MSWriter;

class Cotter : private UVWCalculater
{
	public:
		enum OutputFormat { MSOutputFormat, FitsOutputFormat, FlagsOutputFormat };
		
		Cotter();
		~Cotter();
		
		void Run(double timeRes_s, double freqRes_kHz);
		
		void SetOutputFilename(const std::string& outputFilename) { _outputFilename = outputFilename; _defaultFilename = false; }
		void SetOutputFormat(enum OutputFormat format) { _outputFormat = format; }
		void SetFileSets(const std::vector<std::vector<std::string> >& fileSets) { _fileSets = fileSets; }
		void SetThreadCount(size_t threadCount) { _threadCount = threadCount; }
		void SetRFIDetection(bool performRFIDetection) { _rfiDetection = performRFIDetection; }
		void SetCollectStatistics(bool collectStatistics) { _collectStatistics = collectStatistics; }
		void SetCollectHistograms(bool collectHistograms) { _collectHistograms = collectHistograms; }
		void SetHistoryInfo(const std::string &commandLine) { _commandLine = commandLine; }
		void SetMetaFilename(const char *metaFilename) { _metaFilename = metaFilename; }
		void SetAntennaLocationsFilename(const char *filename) { _antennaLocationsFilename = filename; }
		void SetHeaderFilename(const char *filename) { _headerFilename = filename; }
		void SetInstrConfigFilename(const char *filename) { _instrConfigFilename = filename; }
		void SetMaxBufferSize(const size_t bufferSizeInSamples) { _maxBufferSize = bufferSizeInSamples; }
		void SetDisableGeometricCorrections(bool disableCorrections) { _disableGeometricCorrections = disableCorrections; }
		void SetOverridePhaseCentre(long double newRARad, long double newDecRad)
		{
			_overridePhaseCentre = true;
			_customRARad = newRARad;
			_customDecRad = newDecRad;
		}
		void SetUsePointingCentre(bool usePointingCentre)
		{
			_usePointingCentre = usePointingCentre;
		}
		void SetDoAlign(bool doAlign) { _doAlign = doAlign; }
		void SetDoFlagMissingSubbands(bool doFlagMissingSubbands) { _doFlagMissingSubbands = doFlagMissingSubbands; }
        void SetDoCorrectCableLength(bool doCorrectCableLength) { _doCorrectCableLength = doCorrectCableLength; }
		void SetSubbandCount(size_t subbandCount) { _subbandCount = subbandCount; }
		void SetRemoveFlaggedAntennae(bool removeFlaggedAntennae) { _removeFlaggedAntennae = removeFlaggedAntennae; }
		void SetRemoveAutoCorrelations(bool removeAutoCorrelations) { _removeAutoCorrelations = removeAutoCorrelations; }
		void SetReadSubbandPassbandFile(const std::string& subbandPassbandFilename)
		{
			_subbandPassbandFilename = subbandPassbandFilename;
		}
		void SetFlagAutoCorrelations(bool flagAutoCorrelations) { _flagAutos = flagAutoCorrelations; }
		void SetInitDurationToFlag(double initDuration) { _initDurationToFlag = initDuration; }
		void SetEndDurationToFlag(double endDuration) { _endDurationToFlag = endDuration; }
		void SetApplySBGains(bool applySBGains) { _applySBGains = applySBGains; }
		void SetFlagDCChannels(bool flagDCChannels) { _flagDCChannels = flagDCChannels; }
		void SetFlagFileTemplate(const std::string& flagFileTemplate) { _flagFileTemplate = flagFileTemplate; }
		void SetSaveQualityStatistics(const std::string& file) { _qualityStatisticsFilename = file; }
		void SetSkipWriting(bool skipWriting) { _skipWriting = skipWriting; }
		void FlagAntenna(size_t antIndex) { _userFlaggedAntennae.push_back(antIndex); }
		void FlagSubband(size_t sbIndex) { _flaggedSubbands.insert(sbIndex); }
		void SetSubbandEdgeFlagWidth(double edgeFlagWidth) { _subbandEdgeFlagWidthKHz = edgeFlagWidth; }
		void SetOfflineGPUBoxFormat(bool offlineFormat) { _offlineGPUBoxFormat = offlineFormat; }
		void SetUseDysco(bool useDysco) { _useDysco = useDysco; }
		void SetAdvancedDyscoOptions(size_t dataBitRate, size_t weightBitRate, const std::string& distribution, double distTruncation, const std::string& normalization)
		{
			_dyscoDataBitRate = dataBitRate;
			_dyscoWeightBitRate = weightBitRate;
			_dyscoDistribution = distribution;
			_dyscoDistTruncation = distTruncation;
			_dyscoNormalization = normalization;
		}
		void SetSolutionFile(const char* solutionFilename) { _solutionFilename = solutionFilename; }
		void SetApplyBeforeAveraging(bool beforeAvg) { _applySolutionsBeforeAveraging = beforeAvg; }
		void SetStrategyFile(const std::string& filename) { _strategyFilename = filename; }
		size_t SubbandCount() const { return _subbandCount; }
		
	private:
		MWAConfig _mwaConfig;
		std::unique_ptr<Writer> _writer;
		std::unique_ptr<GPUFileReader> _reader;
		aoflagger::AOFlagger _flagger;
		
		std::vector<double> _subbandCorrectionFactors[4];
		std::unique_ptr<bool[]> _isAntennaFlaggedMap;
		size_t _unflaggedAntennaCount;
		
		Stopwatch _readWatch, _processWatch, _writeWatch;
		
		std::vector<std::vector<std::string> > _fileSets;
		size_t _threadCount;
		size_t _maxBufferSize;
		size_t _subbandCount;
		size_t _quackInitSampleCount, _quackEndSampleCount;
		double _subbandEdgeFlagWidthKHz;
		size_t _subbandEdgeFlagCount;
		size_t _missingEndScans;
		size_t _curChunkStart, _curChunkEnd, _curSbStart, _curSbEnd;
		bool _defaultFilename, _rfiDetection, _collectStatistics, _collectHistograms, _usePointingCentre;
		enum OutputFormat _outputFormat;
		std::string _outputFilename, _commandLine;
		std::string _metaFilename, _antennaLocationsFilename, _headerFilename, _instrConfigFilename;
		std::string _subbandPassbandFilename, _flagFileTemplate, _qualityStatisticsFilename;
		bool _applySolutionsBeforeAveraging;
		std::string _solutionFilename;
		std::string _strategyFilename;
		std::vector<size_t> _userFlaggedAntennae;
		std::set<size_t> _flaggedSubbands;
		
		std::map<std::pair<size_t, size_t>, aoflagger::ImageSet> _imageSetBuffers;
		std::map<std::pair<size_t, size_t>, aoflagger::FlagMask> _flagBuffers;
		std::vector<double> _channelFrequenciesHz;
		std::vector<double> _scanTimes;
		std::queue<std::pair<size_t,size_t> > _baselinesToProcess;
		std::unique_ptr<ProgressBar> _progressBar;
		size_t _baselinesToProcessCount;
		std::vector<size_t> _subbandOrder;
		std::vector<int> _hduOffsetsPerGPUBox;
		std::unique_ptr<class FlagReader> _flagReader;
		
		std::mutex _mutex;
		std::unique_ptr<aoflagger::QualityStatistics> _statistics;
		aoflagger::FlagMask _correlatorMask, _fullysetMask;
		
		bool _disableGeometricCorrections, _removeFlaggedAntennae, _removeAutoCorrelations, _flagAutos;
		bool _overridePhaseCentre, _doAlign, _doFlagMissingSubbands, _applySBGains, _flagDCChannels, _skipWriting, _doCorrectCableLength;
		bool _offlineGPUBoxFormat;
		long double _customRARad, _customDecRad;
		double _initDurationToFlag, _endDurationToFlag;
		
		bool _useDysco;
		size_t _dyscoDataBitRate;
		size_t _dyscoWeightBitRate;
		std::string _dyscoDistribution;
		std::string _dyscoNormalization;
		double _dyscoDistTruncation;
		
		std::unique_ptr<bool[]> _outputFlags;
		aligned_ptr<std::complex<float>> _outputData;
		aligned_ptr<float> _outputWeights;
		
		void processAllContiguousBands(size_t timeAvgFactor, size_t freqAvgFactor);
		void processOneContiguousBand(const std::string& outputFilename, size_t timeAvgFactor, size_t freqAvgFactor);
		void createReader(const std::vector<std::string> &curFileset);
		void initializeReader();
		void processAndWriteTimestep(size_t timeIndex);
		void processAndWriteTimestepFlagsOnly(size_t timeIndex);
		void baselineProcessThreadFunc();
		void processBaseline(size_t antenna1, size_t antenna2, aoflagger::Strategy& strategy, aoflagger::QualityStatistics& statistics);
		void correctConjugated(aoflagger::ImageSet& imageSet, size_t imageIndex) const;
		void correctCableLength(aoflagger::ImageSet& imageSet, size_t polarization, double cableDelay) const;
		void writeAntennae();
		void writeSPW();
		void writeSource();
		void writeField();
		void writeObservation();
		void initPerInputSubbandGains();
		void readSubbandPassbandFile();
		void initializeSubbandPassband();
		void flagBadCorrelatorSamples(aoflagger::FlagMask &flagMask) const;
		void initializeWeights(aligned_ptr<float>& outputWeights);
		void initializeSbOrder();
		void writeAlignmentScans();
		void writeMWAFieldsToMS(const std::string& outputFilename, size_t flagWindowSize);
		void writeMWAFieldsToUVFits(const std::string& outputFilename);
		void onHDUOffsetsChange(const std::vector<int>& newHDUOffsets);
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
		bool isGPUBoxMissing(size_t gpuBoxIndex) const
		{
			for(std::vector<std::vector<std::string> >::const_iterator i=_fileSets.begin(); i!=_fileSets.end(); ++i)
			{
				const std::vector<std::string> &timeRangeSets = *i;
				if(gpuBoxIndex >= timeRangeSets.size() || timeRangeSets[gpuBoxIndex].empty())
					return true;
			}
			return false;
		}
		size_t nChannelsInCurSBRange() const
		{
			return _mwaConfig.Header().nChannels * (_curSbEnd - _curSbStart) / _subbandCount;
		}
		static std::string twoDigits(int value)
		{
			std::string str("  ");
			str[0] = '0' + (value/10);
			str[1] = '0' + (value%10);
			return str;
		}
		
		// Implementing UVWCalculater
		virtual void CalculateUVW(double date, size_t antenna1, size_t antenna2, double &u, double &v, double &w);

		Cotter(const Cotter&) = delete;
		void operator=(const Cotter&) = delete;
};

#endif
