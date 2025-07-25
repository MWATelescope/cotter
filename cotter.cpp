#include "cotter.h"

#include "applysolutionswriter.h"
#include "baselinebuffer.h"
#include "flagreader.h"
#include "flagwriter.h"
#include "fitswriter.h"
#include "geometry.h"
#include "mswriter.h"
#include "mwafits.h"
#include "mwams.h"
#include "subbandpassband.h"
#include "progressbar.h"
#include "threadedwriter.h"
#include "radeccoord.h"
#include "version.h"

#include <thread>
#include <functional>

#include <fstream>
#include <iostream>
#include <map>
#include <cmath>
#include <complex>

#include <xmmintrin.h>

#define USE_SSE

using namespace aoflagger;

Cotter::Cotter() :
	_unflaggedAntennaCount(0),
	_threadCount(1),
	_maxBufferSize(0),
	_subbandCount(24),
	_quackInitSampleCount(4),
	_subbandEdgeFlagWidthKHz(80.0),
	_subbandEdgeFlagCount(2),
	_defaultFilename(true),
	_rfiDetection(true),
	_collectStatistics(true),
	_collectHistograms(false),
	_usePointingCentre(false),
	_outputFormat(MSOutputFormat),
	_applySolutionsBeforeAveraging(false),
	_disableGeometricCorrections(false),
	_removeFlaggedAntennae(true),
	_removeAutoCorrelations(false),
	_flagAutos(true),
	_overridePhaseCentre(false),
	_doAlign(true),
	_doFlagMissingSubbands(true),
	_applySBGains(true),
	_flagDCChannels(true),
	_skipWriting(false),
    _doCorrectCableLength(true),
	_offlineGPUBoxFormat(false),
	_customRARad(0.0),
	_customDecRad(0.0),
	_initDurationToFlag(4.0),
	_endDurationToFlag(0.0),
	_useDysco(false),
	_dyscoDataBitRate(8),
	_dyscoWeightBitRate(12),
	_dyscoDistribution("TruncatedGaussian"),
	_dyscoNormalization("AF"),
	_dyscoDistTruncation(2.5),
	_outputData(empty_aligned<std::complex<float>>()),
	_outputWeights(empty_aligned<float>())
{
}

Cotter::~Cotter() = default;

void Cotter::Run(double timeRes_s, double freqRes_kHz)
{
	_readWatch.Start();
	bool lockPointing = false;
	
	if(_metaFilename.empty())
		throw std::runtime_error("No metafits file specified! This is required since 2013-08-02, because the text files (header/instr_config/antenna_location) are missing some of the information (e.g. digital gains). You can still override the information in the metafits file with a text file (see the -a, -h and -i options)");
	_mwaConfig.ReadMetaFits(_metaFilename.c_str(), lockPointing);
	if(!_headerFilename.empty())
	{
		std::cout << "Overriding header values in metafits file with " << _headerFilename << "\n";
		_mwaConfig.ReadHeader(_headerFilename, lockPointing);
	}
	if(!_instrConfigFilename.empty())
	{
		std::cout << "Overriding instrumental configuration values in metafits file with " << _instrConfigFilename << "\n";
		_mwaConfig.ReadInputConfig(_instrConfigFilename);
	}
	if(!_antennaLocationsFilename.empty())
	{
		std::cout << "Overriding antenna locations in metafits file with " << _antennaLocationsFilename << "\n";
		_mwaConfig.ReadAntennaPositions(_antennaLocationsFilename);
	}
	
	if(_disableGeometricCorrections)
		_mwaConfig.HeaderRW().geomCorrection = false;
	if(_overridePhaseCentre)
	{
		std::cout << "Using manually-specified phase centre: " << RaDecCoord::RAToString(_customRARad) << ' ' << RaDecCoord::DecToString(_customDecRad) << '\n';
		_mwaConfig.HeaderRW().raHrs = _customRARad * (12.0/M_PI);
		_mwaConfig.HeaderRW().decDegs = _customDecRad * (180.0/M_PI);
	}
	else if(_usePointingCentre)
	{
		std::cout << "Using pointing centre as phase centre: " << RaDecCoord::RAToString(_mwaConfig.HeaderExt().tilePointingRARad) << ' ' << RaDecCoord::DecToString(_mwaConfig.HeaderExt().tilePointingDecRad) << '\n';
		_mwaConfig.HeaderRW().raHrs = _mwaConfig.HeaderExt().tilePointingRARad * (12.0/M_PI);
		_mwaConfig.HeaderRW().decDegs = _mwaConfig.HeaderExt().tilePointingDecRad * (180.0/M_PI);
	}
	_mwaConfig.CheckSetup();
	
	size_t timeAvgFactor = round(timeRes_s/_mwaConfig.Header().integrationTime);
	if(timeAvgFactor == 0)
		timeAvgFactor = 1;
	timeRes_s = timeAvgFactor*_mwaConfig.Header().integrationTime;
	size_t freqAvgFactor = round(freqRes_kHz/(1000.0*_mwaConfig.Header().bandwidthMHz / _mwaConfig.Header().nChannels));
	if(freqAvgFactor == 0)
		freqAvgFactor = 1;
	freqRes_kHz = freqAvgFactor*(1000.0*_mwaConfig.Header().bandwidthMHz / _mwaConfig.Header().nChannels);
	std::cout << "Output resolution: " << timeRes_s << " s / " << freqRes_kHz << " kHz (time avg: " << timeAvgFactor << "x, freq avg: " << freqAvgFactor << "x).\n";
	
	_subbandEdgeFlagCount = round(_subbandEdgeFlagWidthKHz / (1000.0*_mwaConfig.Header().bandwidthMHz / _mwaConfig.Header().nChannels));
	
	_quackInitSampleCount = round(_initDurationToFlag / _mwaConfig.Header().integrationTime);
	_quackEndSampleCount = round(_endDurationToFlag / _mwaConfig.Header().integrationTime);
	std::cout << "The first " << _quackInitSampleCount << " samples (" << round(10.0 * _quackInitSampleCount * _mwaConfig.Header().integrationTime)/10.0 << " s), last " << _quackEndSampleCount << " samples (" << round(10.0 * _quackEndSampleCount * _mwaConfig.Header().integrationTime)/10.0 << " s) and " << _subbandEdgeFlagCount << " edge channels will be flagged.\n";
	
	if(_subbandPassbandFilename.empty())
		initializeSubbandPassband();
	else
		readSubbandPassbandFile();
	
	initPerInputSubbandGains();
	
	initializeSbOrder();
	if(_subbandEdgeFlagCount > _mwaConfig.Header().nChannels / (_subbandCount*2))
		throw std::runtime_error("Tried to flag more edge channels than available");
	
	processAllContiguousBands(timeAvgFactor, freqAvgFactor);
	
	std::cout
		<< "Wall-clock time in reading: " << _readWatch.ToString()
		<< " processing: " << _processWatch.ToString()
		<< " writing: " << _writeWatch.ToString() << '\n';
}

void Cotter::processAllContiguousBands(size_t timeAvgFactor, size_t freqAvgFactor)
{
	std::vector<std::pair<int, int> > contiguousSBRanges;
	int subbandNumber = _mwaConfig.HeaderExt().subbandNumbers[0];
	int rangeStartSB = 0;
	for(size_t sb=1; sb!=_subbandCount; ++sb)
	{
		int curNumber = _mwaConfig.HeaderExt().subbandNumbers[sb];
		if(curNumber != subbandNumber+1)
		{
			contiguousSBRanges.push_back(std::pair<int, int>(rangeStartSB, sb));
			rangeStartSB = sb;
		}
		subbandNumber = curNumber;
	}
	contiguousSBRanges.push_back(std::pair<int, int>(rangeStartSB, _subbandCount));
	
	if(contiguousSBRanges.size() == 1)
	{
		std::cout << "Observation's bandwidth is contiguous.\n";
		_curSbStart = 0;
		_curSbEnd = _subbandCount;
		
		_channelFrequenciesHz.resize(_mwaConfig.Header().nChannels);
		for(size_t ch=0; ch!=_mwaConfig.Header().nChannels; ++ch)
			_channelFrequenciesHz[ch] = _mwaConfig.ChannelFrequencyHz(ch);
		
		if(_defaultFilename)
			_outputFilename = "preprocessed.ms";
	
		processOneContiguousBand(_outputFilename, timeAvgFactor, freqAvgFactor);
	}
	else {
		std::cout << "Observation's bandwidth is non-contiguous.\n";
		
		std::string bandFilename;
		if(_defaultFilename)
			bandFilename = "preprocessed.ms";
		else
			bandFilename = _outputFilename;
		
		size_t dotPos = bandFilename.find(".");
		if(dotPos == std::string::npos)
			throw std::runtime_error("Something is wrong with the output filename.");
		
		if(_outputFormat != FlagsOutputFormat)
		{
			bandFilename = bandFilename.substr(0, dotPos) + "\?\?\?-\?\?\?" + bandFilename.substr(dotPos);
		}
		
		for(size_t bandIndex = 0; bandIndex!=contiguousSBRanges.size(); ++bandIndex)
		{
			_curSbStart = contiguousSBRanges[bandIndex].first;
			_curSbEnd = contiguousSBRanges[bandIndex].second;
			
			size_t nChannels = nChannelsInCurSBRange(), nChannelPerSb = _mwaConfig.Header().nChannels / _subbandCount;
			_channelFrequenciesHz.resize(nChannels);
			int
				chStartNo = _mwaConfig.HeaderExt().subbandNumbers[_curSbStart],
				chEndNo =_mwaConfig.HeaderExt().subbandNumbers[_curSbEnd-1];
			std::vector<double>::iterator chFreqIter = _channelFrequenciesHz.begin();
			for(int coarseChannel=chStartNo; coarseChannel!=chEndNo+1; ++coarseChannel)
			{
				for(size_t ch=0; ch!=nChannelPerSb; ++ch)
				{
					*chFreqIter = _mwaConfig.ChannelFrequencyHz(coarseChannel, ch);
					++chFreqIter;
				}
			}
			
			if(_outputFormat != FlagsOutputFormat)
			{
				bandFilename[dotPos] = (char) ('0' + (chStartNo/100));
				bandFilename[dotPos+1] = (char) ('0' + ((chStartNo/10)%10));
				bandFilename[dotPos+2] = (char) ('0' + (chStartNo%10));
				bandFilename[dotPos+4] = (char) ('0' + (chEndNo/100));
				bandFilename[dotPos+5] = (char) ('0' + ((chEndNo/10)%10));
				bandFilename[dotPos+6] = (char) ('0' + (chEndNo%10));
			}
			std::cout << " |=== BAND " << (bandIndex+1) << " / " << contiguousSBRanges.size() << " ===|\n";
			std::cout << "Writing contiguous band " << (bandIndex+1) << " to " << bandFilename << ".\n";
			processOneContiguousBand(bandFilename, timeAvgFactor, freqAvgFactor);
		}
	}
}

void Cotter::processOneContiguousBand(const std::string& outputFilename, size_t timeAvgFactor, size_t freqAvgFactor)
{
	switch(_outputFormat)
	{
		case FlagsOutputFormat:
			std::cout << "Only flags will be outputted.\n";
			if(freqAvgFactor != 1 || timeAvgFactor != 1)
				throw std::runtime_error("You have specified time or frequency averaging and outputting only flags: this is incompatible");
			if(_removeFlaggedAntennae || _removeAutoCorrelations)
				throw std::runtime_error("Can't prune flagged/auto-correlated antennas when writing flag file");
			_writer.reset(new FlagWriter(outputFilename, _mwaConfig.HeaderExt().gpsTime, _mwaConfig.Header().nScans, _curSbStart, _curSbEnd, _subbandOrder));
			break;
		case FitsOutputFormat:
			_writer.reset(new ThreadedWriter(std::unique_ptr<FitsWriter>(new FitsWriter(outputFilename))));
			break;
		case MSOutputFormat: {
			std::unique_ptr<MSWriter> msWriter(new MSWriter(outputFilename));
			if(_useDysco)
				msWriter->EnableCompression(_dyscoDataBitRate, _dyscoWeightBitRate, _dyscoDistribution, _dyscoDistTruncation, _dyscoNormalization);
			_writer.reset(new ThreadedWriter(std::move(msWriter)));
		} break;
	}
	if(!_solutionFilename.empty() && !_applySolutionsBeforeAveraging)
	{
		_writer.reset(new ApplySolutionsWriter(std::move(_writer), _solutionFilename, ((_curSbStart * _mwaConfig.Header().nChannels) / _subbandCount) / freqAvgFactor, _mwaConfig.Header().nChannels / freqAvgFactor));
	}
	if(freqAvgFactor != 1 || timeAvgFactor != 1)
	{
		_writer.reset(new ThreadedWriter(std::unique_ptr<AveragingWriter>(new AveragingWriter(std::move(_writer), timeAvgFactor, freqAvgFactor, *this))));
	}
	if(!_solutionFilename.empty() && _applySolutionsBeforeAveraging)
	{
		_writer.reset(new ApplySolutionsWriter(std::move(_writer), _solutionFilename, (_curSbStart * _mwaConfig.Header().nChannels) / _subbandCount, _mwaConfig.Header().nChannels));
	}
	writeAntennae();
	writeSPW();
	writeSource();
	writeField();
	_writer->WritePolarizationForLinearPols(false);
	writeObservation();

	if(!_qualityStatisticsFilename.empty())
	{
		std::unique_ptr<Writer> qsWriter(new MSWriter(_qualityStatisticsFilename));
		std::swap(qsWriter, _writer);
		writeAntennae();
		writeSPW();
		writeSource();
		writeField();
		_writer->WritePolarizationForLinearPols(false);
		writeObservation();
		std::swap(qsWriter, _writer);
	}
	
	_hduOffsetsPerGPUBox.assign(_subbandCount, 9999);
	const size_t
		nChannels = nChannelsInCurSBRange(),
		antennaCount = _mwaConfig.NAntennae();
	size_t maxScansPerPart = _maxBufferSize / (nChannels*(antennaCount+1)*antennaCount*2);
	
	if(maxScansPerPart<1)
	{
		std::cout << "WARNING! The given amount of memory is not even enough for one scan and therefore below the minimum that Cotter will need; will use more memory. Expect swapping and very poor flagging accuracy.\nWARNING! This is a *VERY BAD* condition, so better make sure to resolve it!";
		maxScansPerPart = 1;
	} else if(maxScansPerPart<20 && _rfiDetection)
	{
		std::cout << "WARNING! This computer does not have enough memory for accurate flagging; expect non-optimal flagging accuracy.\n"; 
	}
	size_t partCount = 1 + _mwaConfig.Header().nScans / maxScansPerPart;
	if(partCount == 1)
		std::cout << "All " << _mwaConfig.Header().nScans << " scans fit in memory; no partitioning necessary.\n";
	else
		std::cout << "Observation does not fit fully in memory, will partition data in " << partCount << " chunks of at least " << (_mwaConfig.Header().nScans/partCount) << " scans.\n";
	
	_scanTimes.resize(_mwaConfig.Header().nScans);
	for(size_t t=0; t!=_mwaConfig.Header().nScans; ++t)
	{
		double dateMJD = _mwaConfig.Header().dateFirstScanMJD * 86400.0 + t * _mwaConfig.Header().integrationTime;
		_scanTimes[t] = dateMJD;
	}
	
	std::vector<std::string> params;
	std::stringstream paramStr;
	paramStr << "timeavg=" << timeAvgFactor << ",freqavg=" << freqAvgFactor << ",windowSize=" << (_mwaConfig.Header().nScans/partCount);
	params.push_back(paramStr.str());
	_writer->WriteHistoryItem(_commandLine, "Cotter MWA preprocessor", params);
	
	if(_strategyFilename.empty())
		_strategyFilename = _flagger.FindStrategyFile(TelescopeId::MWA_TELESCOPE);
		
	std::vector<std::vector<std::string> >::const_iterator
		currentFileSetPtr = _fileSets.begin();
	createReader(*currentFileSetPtr);
	
	_readWatch.Pause();
	
	for(size_t chunkIndex = 0; chunkIndex != partCount; ++chunkIndex)
	{
		std::cout << "=== Processing chunk " << (chunkIndex+1) << " of " << partCount << " ===\n";
		_readWatch.Start();
		
		_curChunkStart = _mwaConfig.Header().nScans*chunkIndex/partCount;
		_curChunkEnd = _mwaConfig.Header().nScans*(chunkIndex+1)/partCount;
		
		// Initialize buffers
		if(chunkIndex == 0)
		{
			// First time: allocate the buffers
			const size_t requiredWidthCapacity = (_mwaConfig.Header().nScans+partCount-1)/partCount;
			for(size_t antenna1=0;antenna1!=antennaCount;++antenna1)
			{
				for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
				{
					_imageSetBuffers.emplace(
						std::pair<size_t,size_t>(antenna1, antenna2),
						_flagger.MakeImageSet(_curChunkEnd-_curChunkStart, nChannels, 8, 0.0f, requiredWidthCapacity)
					);
				}
			}
		} else {
			// Resize the buffers, but don't reallocate. I used to reallocate all buffers
			// here, but this gave awful memory fragmentation issues, since the buffers can have slightly
			// different sizes during each run. This led to ~2x as much memory usage.
			for(auto& buffer : _imageSetBuffers)
			{
				buffer.second.ResizeWithoutReallocation(_curChunkEnd-_curChunkStart);
				buffer.second.Set(0.0f);
			}
		}
		
		size_t bufferPos = 0;
		bool continueWithNextFile;
		do {
			initializeReader();
			
			bool firstRead = (bufferPos == 0 && chunkIndex == 0);
			
			bool moreAvailableInCurrentFile = _reader->Read(bufferPos, _curChunkEnd-_curChunkStart);
			
			if(firstRead && _reader->HasStartTime())
			{
				std::time_t startTime = _reader->StartTime();
				std::tm startTimeTm;
				gmtime_r(&startTime, &startTimeTm);
				if(startTimeTm.tm_year+1900 != _mwaConfig.Header().year ||
					startTimeTm.tm_mon+1 != _mwaConfig.Header().month ||
					startTimeTm.tm_mday != _mwaConfig.Header().day ||
					startTimeTm.tm_hour != _mwaConfig.Header().refHour ||
					startTimeTm.tm_min != _mwaConfig.Header().refMinute ||
					startTimeTm.tm_sec != _mwaConfig.Header().refSecond)
				{
					std::cout << "WARNING: start time according to raw files is "
						<< startTimeTm.tm_year+1900  << '-' << twoDigits(startTimeTm.tm_mon+1) << '-' << twoDigits(startTimeTm.tm_mday) << ' '
						<< twoDigits(startTimeTm.tm_hour) << ':' << twoDigits(startTimeTm.tm_min) << ':' << twoDigits(startTimeTm.tm_sec)
						<< ",\nbut meta files say "
						<< _mwaConfig.Header().year << '-' << twoDigits(_mwaConfig.Header().month) << '-' << twoDigits(_mwaConfig.Header().day) << ' '
						<< twoDigits(_mwaConfig.Header().refHour) << ':' << twoDigits(_mwaConfig.Header().refMinute) << ':'
						<< twoDigits(_mwaConfig.Header().refSecond)
						<< " !\nWill use start time from raw file, which should be most accurate.\n";
					_mwaConfig.HeaderRW().year = startTimeTm.tm_year+1900;
					_mwaConfig.HeaderRW().month = startTimeTm.tm_mon+1;
					_mwaConfig.HeaderRW().day = startTimeTm.tm_mday;
					_mwaConfig.HeaderRW().refHour = startTimeTm.tm_hour;
					_mwaConfig.HeaderRW().refMinute = startTimeTm.tm_min;
					_mwaConfig.HeaderRW().refSecond = startTimeTm.tm_sec;
					_mwaConfig.HeaderRW().dateFirstScanMJD = _mwaConfig.Header().GetDateFirstScanFromFields();
				}
			}
			
			if(!moreAvailableInCurrentFile && bufferPos < (_curChunkEnd-_curChunkStart))
			{
				if(currentFileSetPtr != _fileSets.end())
				{
					// Go to the next set of GPU files and add them to the buffer
					++currentFileSetPtr;
					continueWithNextFile = (currentFileSetPtr!=_fileSets.end());
					if(continueWithNextFile)
						createReader(*currentFileSetPtr);
				} else {
					continueWithNextFile = false;
				}
			} else {
				continueWithNextFile = false;
			}
		} while(continueWithNextFile);
		
		if(bufferPos < _curChunkEnd-_curChunkStart)
		{
			_missingEndScans = (_curChunkEnd-_curChunkStart)- bufferPos;
			std::cout << "Warning: header specifies " << _mwaConfig.Header().nScans << " scans, but there are only " << (bufferPos+_curChunkStart) << " in the data.\n"
			"Last " << _missingEndScans << " scan(s) will be flagged.\n";
		} else {
			_missingEndScans = 0;
		}
		if(_curChunkEnd + _quackEndSampleCount > _mwaConfig.Header().nScans)
		{
			size_t extraSamples = (_curChunkEnd + _quackEndSampleCount) - _mwaConfig.Header().nScans;
			_missingEndScans += extraSamples;
			std::cout << "Flagging extra " << extraSamples << " samples at end.\n";
		}
		
		_fullysetMask = FlagMask(_flagger.MakeFlagMask(_curChunkEnd-_curChunkStart, _reader->ChannelCount(), true));
		_correlatorMask = FlagMask(_flagger.MakeFlagMask(_curChunkEnd-_curChunkStart, _reader->ChannelCount(), false));
		flagBadCorrelatorSamples(_correlatorMask);
		
		for(size_t antenna1=0;antenna1!=antennaCount;++antenna1)
		{
			for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
			{
				_baselinesToProcess.push(std::pair<size_t,size_t>(antenna1, antenna2));
				
				// We will put a place holder in the flagbuffer map, so we don't have to write (and lock)
				// during multi threaded processing.
				_flagBuffers.emplace(
					std::pair<size_t,size_t>(antenna1, antenna2),
					FlagMask()
				);
			}
		}
		_baselinesToProcessCount = _baselinesToProcess.size();
		
		_readWatch.Pause();
		_processWatch.Start();
		
		if(!_flagFileTemplate.empty())
		{
			_progressBar.reset(new ProgressBar("Reading flags"));
			if(_flagReader.get() == 0)
				_flagReader.reset(new FlagReader(_flagFileTemplate, _hduOffsetsPerGPUBox, _subbandOrder, _curSbStart, _curSbEnd));
			// Create the flag masks
			for(size_t antenna1=0;antenna1!=antennaCount;++antenna1)
			{
				for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
				{
					FlagMask& baseline = _flagBuffers.find(std::make_pair(antenna1, antenna2))->second;
					baseline = FlagMask(_flagger.MakeFlagMask(_curChunkEnd-_curChunkStart, _reader->ChannelCount()));
				}
			}
			// Fill the flag masks by reading the files
			for(size_t t=_curChunkStart; t!=_curChunkEnd; ++t)
			{
				_progressBar->SetProgress(t-_curChunkStart, _curChunkEnd-_curChunkStart);
				size_t baselineIndex = 0;
				for(size_t antenna1=0;antenna1!=antennaCount;++antenna1)
				{
					for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
					{
						FlagMask& mask = _flagBuffers.find(std::make_pair(antenna1, antenna2))->second;
						size_t stride = mask.HorizontalStride();
						bool* bufferPos = mask.Buffer() + (t - _curChunkStart);
						_flagReader->Read(t, baselineIndex, bufferPos, stride);
						++baselineIndex;
					}
				}
			}
			_progressBar.reset();
		}
		
		std::string taskDescription;
		if(_rfiDetection)
		{
			if(_collectStatistics)
				taskDescription = "RFI detection, stats, conjugations, subband ordering and cable length corrections";
			else
				taskDescription = "RFI detection, conjugations, subband ordering and cable length corrections";
		}
		else
		{
			if(_collectStatistics)
				taskDescription = "Stats, conjugations, subband ordering and cable length corrections";
			else
				taskDescription = "Conjugations, subband ordering and cable length corrections";
		}
		_progressBar.reset(new ProgressBar(taskDescription));
		
		std::vector<std::thread> threadGroup;
		for(size_t i=0; i!=_threadCount; ++i)
			threadGroup.emplace_back(std::bind(&Cotter::baselineProcessThreadFunc, this));
		for(std::thread& t : threadGroup)
			t.join();
		
		_progressBar.reset();
		_processWatch.Pause();
		_writeWatch.Start();
		
		if(_skipWriting)
		{
			std::cout << "Skipping writing of visibilities.\n";
		}
		else {
			_progressBar.reset(new ProgressBar("Writing"));
			_outputFlags.reset(new bool[nChannels*4]);
			_outputData = make_aligned<std::complex<float>>(nChannels*4, 16);
			_outputWeights = make_aligned<float>(nChannels*4, 16);
			for(size_t t=_curChunkStart; t!=_curChunkEnd; ++t)
			{
				_progressBar->SetProgress(t-_curChunkStart, _curChunkEnd-_curChunkStart);
				if(_outputFormat == FlagsOutputFormat)
					processAndWriteTimestepFlagsOnly(t);
				else
					processAndWriteTimestep(t);
			}
			_outputData.reset();
			_outputWeights.reset();
			_outputFlags.reset();
			_progressBar.reset();
		}
		
		_flagBuffers.clear();
		
		_correlatorMask = FlagMask();
		_fullysetMask = FlagMask();
		
		_writeWatch.Pause();
	} // end for chunkIndex!=partCount
	
	_imageSetBuffers.clear();
	
	_writeWatch.Start();
	
	writeAlignmentScans();
	
	const bool writerSupportsStatistics = _writer->CanWriteStatistics();
	
	_writer.reset();
	_reader.reset();
	
	// Necessary to make sure it is reinitialized in the following cont band:
	_flagReader.reset();
	
	if(_collectStatistics && writerSupportsStatistics) {
		std::cout << "Writing statistics to measurement set...\n";
		_statistics->WriteStatistics(outputFilename);
	}
	
	if(_collectStatistics && !_qualityStatisticsFilename.empty()) {
		std::cout << "Writing statistics to " << _qualityStatisticsFilename << "...\n";
		_statistics->WriteStatistics(_qualityStatisticsFilename);
	}
	
	// Reset statistics so that a potentially next subband starts with empty statistics
	_statistics.reset();
	
	if(_outputFormat == MSOutputFormat)
	{
		std::cout << "Writing MWA fields to measurement set...\n";
		writeMWAFieldsToMS(outputFilename, _mwaConfig.Header().nScans/partCount);
	}
	else if(_outputFormat == FitsOutputFormat)
	{
		std::cout << "Writing MWA fields to UVFits file...\n";
		writeMWAFieldsToUVFits(outputFilename);
	}
	
	_writeWatch.Pause();
}

void Cotter::createReader(const std::vector<std::string>& curFileset)
{
	_reader.reset();
	_reader.reset(new GPUFileReader(_mwaConfig.NAntennae(), nChannelsInCurSBRange(), _threadCount, _offlineGPUBoxFormat));
	_reader->SetHDUOffsetsChangeCallback(std::bind(&Cotter::onHDUOffsetsChange, this, std::placeholders::_1));

	// Add the gpubox files in the right order
	for(size_t sb=_curSbStart; sb!=_curSbEnd; ++sb)
	{
		size_t fileBelongingToSB = _subbandOrder[sb];
		_reader->AddFile(curFileset[fileBelongingToSB].c_str());
	}

	_reader->Initialize(_mwaConfig.Header().integrationTime, _doAlign);
}

void Cotter::initializeReader()
{
	const size_t antennaCount = _mwaConfig.NAntennae();
	
	for(size_t i=0; i!=_mwaConfig.Header().nInputs; ++i)
		_reader->SetCorrInputToOutput(i, _mwaConfig.Input(i).antennaIndex, _mwaConfig.Input(i).polarizationIndex);
	
	// Initialize buffers of reader
	_reader->ResetBuffers();
	for(size_t antenna1=0;antenna1!=antennaCount;++antenna1)
	{
		for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
		{
			ImageSet &imageSet = _imageSetBuffers.find(std::pair<size_t, size_t>(antenna1, antenna2))->second;
			BaselineBuffer buffer;
			for(size_t p=0; p!=4; ++p)
			{
				buffer.real[p] = imageSet.ImageBuffer(p*2);
				buffer.imag[p] = imageSet.ImageBuffer(p*2+1);
			}
			buffer.nElementsPerRow = imageSet.HorizontalStride();
			_reader->SetDestBaselineBuffer(antenna1, antenna2, buffer);
		}
	}
}

void Cotter::processAndWriteTimestep(size_t timeIndex)
{
	const size_t antennaCount = _mwaConfig.NAntennae();
	const size_t nChannels = nChannelsInCurSBRange();
	const double dateMJD = _mwaConfig.Header().dateFirstScanMJD + timeIndex * _mwaConfig.Header().integrationTime/86400.0;
	
	Geometry::UVWTimestepInfo uvwInfo;
	Geometry::PrepareTimestepUVW(uvwInfo, dateMJD, _mwaConfig.ArrayLongitudeRad(), _mwaConfig.ArrayLattitudeRad(), _mwaConfig.Header().raHrs, _mwaConfig.Header().decDegs);
	
	double antU[antennaCount], antV[antennaCount], antW[antennaCount];
	for(size_t antenna=0; antenna!=antennaCount; ++antenna)
	{
		const double
			x = _mwaConfig.Antenna(antenna).position[0],
			y = _mwaConfig.Antenna(antenna).position[1],
			z = _mwaConfig.Antenna(antenna).position[2];
		Geometry::CalcUVW(uvwInfo, x, y, z, antU[antenna],antV[antenna], antW[antenna]);
	}
	
	_writer->AddRows(rowsPerTimescan());
	
	double cosAngles[nChannels], sinAngles[nChannels];
	
	initializeWeights(_outputWeights);
	for(size_t antenna1=0; antenna1!=antennaCount; ++antenna1)
	{
		for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
		{
			if(outputBaseline(antenna1, antenna2))
			{
				const ImageSet& imageSet = _imageSetBuffers.find(std::pair<size_t, size_t>(antenna1, antenna2))->second;
				const FlagMask& flagMask = _flagBuffers.find(std::pair<size_t, size_t>(antenna1, antenna2))->second;
				
				const size_t stride = imageSet.HorizontalStride();
				const size_t flagStride = flagMask.HorizontalStride();
				double
					u = antU[antenna1] - antU[antenna2],
					v = antV[antenna1] - antV[antenna2],
					w = antW[antenna1] - antW[antenna2];
					
				// Pre-calculate rotation coefficients for geometric phase delay correction
				if(_mwaConfig.Header().geomCorrection)
				{
					for(size_t ch=0; ch!=nChannels; ++ch)
					{
						double angle = -2.0*M_PI*w*_channelFrequenciesHz[ch] / SPEED_OF_LIGHT;
						double sinAng, cosAng;
						sincos(angle, &sinAng, &cosAng);
						sinAngles[ch] = sinAng; cosAngles[ch] = cosAng;
					}
				}
				
				size_t bufferIndex = timeIndex - _curChunkStart;
	#ifndef USE_SSE
				for(size_t p=0; p!=4; ++p)
				{
					const float
						*realPtr = imageSet.ImageBuffer(p*2)+bufferIndex,
						*imagPtr = imageSet.ImageBuffer(p*2+1)+bufferIndex;
					const bool *flagPtr = flagMask.Buffer()+bufferIndex;
					std::complex<float> *outDataPtr = &_outputData[p];
					bool *outputFlagPtr = &_outputFlags[p];
					for(size_t ch=0; ch!=nChannels; ++ch)
					{
						// Apply geometric phase delay (for w)
						if(_mwaConfig.Header().geomCorrection)
						{
							const float rtmp = *realPtr, itmp = *imagPtr;
							*outDataPtr = std::complex<float>(
								cosAngles[ch] * rtmp - sinAngles[ch] * itmp,
								sinAngles[ch] * rtmp + cosAngles[ch] * itmp
							);
						} else {
							*outDataPtr = std::complex<float>(*realPtr, *imagPtr);
						}
						*outputFlagPtr = *flagPtr;
						realPtr += stride;
						imagPtr += stride;
						flagPtr += flagStride;
						outDataPtr += 4;
						outputFlagPtr += 4;
					}
				}
	#else
				const float
					*realAPtr = imageSet.ImageBuffer(0)+bufferIndex,
					*imagAPtr = imageSet.ImageBuffer(1)+bufferIndex,
					*realBPtr = imageSet.ImageBuffer(2)+bufferIndex,
					*imagBPtr = imageSet.ImageBuffer(3)+bufferIndex,
					*realCPtr = imageSet.ImageBuffer(4)+bufferIndex,
					*imagCPtr = imageSet.ImageBuffer(5)+bufferIndex,
					*realDPtr = imageSet.ImageBuffer(6)+bufferIndex,
					*imagDPtr = imageSet.ImageBuffer(7)+bufferIndex;
				const bool *flagPtr = flagMask.Buffer()+bufferIndex;
				std::complex<float> *outDataPtr = &_outputData[0];
				bool *outputFlagPtr = &_outputFlags[0];
				for(size_t ch=0; ch!=nChannels; ++ch)
				{
					// Apply geometric phase delay (for w)
					if(_mwaConfig.Header().geomCorrection)
					{
						// Note that order within set_ps is reversed; for the four complex numbers,
						// the first two compl are loaded corresponding to set_ps(imag2, real2, imag1, real1).
						__m128 ra = _mm_set_ps(*realBPtr, *realBPtr, *realAPtr, *realAPtr);
						__m128 rb = _mm_set_ps(*realDPtr, *realDPtr, *realCPtr, *realCPtr);
						__m128 rgeom = _mm_set_ps(sinAngles[ch], cosAngles[ch], sinAngles[ch], cosAngles[ch]);
						__m128 ia = _mm_set_ps(*imagBPtr, *imagBPtr, *imagAPtr, *imagAPtr);
						__m128 ib = _mm_set_ps(*imagDPtr, *imagDPtr, *imagCPtr, *imagCPtr);
						__m128 igeom = _mm_set_ps(cosAngles[ch], -sinAngles[ch], cosAngles[ch], -sinAngles[ch]);
						__m128 outa = _mm_add_ps(_mm_mul_ps(ra, rgeom), _mm_mul_ps(ia, igeom));
						__m128 outb = _mm_add_ps(_mm_mul_ps(rb, rgeom), _mm_mul_ps(ib, igeom));
						_mm_store_ps((float*) outDataPtr, outa);
						_mm_store_ps((float*) (outDataPtr+2), outb);
					} else {
						*outDataPtr = std::complex<float>(*realAPtr, *imagAPtr);
						*(outDataPtr+1) = std::complex<float>(*realBPtr, *imagBPtr);
						*(outDataPtr+2) = std::complex<float>(*realCPtr, *imagCPtr);
						*(outDataPtr+3) = std::complex<float>(*realDPtr, *imagDPtr);
					}
					*outputFlagPtr = *flagPtr; ++outputFlagPtr;
					*outputFlagPtr = *flagPtr; ++outputFlagPtr;
					*outputFlagPtr = *flagPtr; ++outputFlagPtr;
					*outputFlagPtr = *flagPtr; ++outputFlagPtr;
					realAPtr += stride; imagAPtr += stride;
					realBPtr += stride; imagBPtr += stride;
					realCPtr += stride; imagCPtr += stride;
					realDPtr += stride; imagDPtr += stride;
					flagPtr += flagStride;
					outDataPtr += 4;
				}
	#endif
				
				_writer->WriteRow(dateMJD*86400.0, dateMJD*86400.0, antenna1, antenna2, u, v, w, _mwaConfig.Header().integrationTime, _outputData.get(), _outputFlags.get(), _outputWeights.get());
			}
		}
	}
}

void Cotter::processAndWriteTimestepFlagsOnly(size_t timeIndex)
{
	const size_t antennaCount = _mwaConfig.NAntennae();
	const size_t nChannels = nChannelsInCurSBRange();
	const double dateMJD = _mwaConfig.Header().dateFirstScanMJD + timeIndex * _mwaConfig.Header().integrationTime/86400.0;
	
	_writer->AddRows(rowsPerTimescan());
	
	initializeWeights(_outputWeights);
	for(size_t antenna1=0; antenna1!=antennaCount; ++antenna1)
	{
		for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
		{
			if(outputBaseline(antenna1, antenna2))
			{
				const FlagMask& flagMask = _flagBuffers.find(std::pair<size_t, size_t>(antenna1, antenna2))->second;
				
				const size_t flagStride = flagMask.HorizontalStride();
				
				size_t bufferIndex = timeIndex - _curChunkStart;
				for(size_t p=0; p!=4; ++p)
				{
					const bool *flagPtr = flagMask.Buffer()+bufferIndex;
					bool *outputFlagPtr = &_outputFlags[p];
					for(size_t ch=0; ch!=nChannels; ++ch)
					{
						*outputFlagPtr = *flagPtr;
						flagPtr += flagStride;
						outputFlagPtr += 4;
					}
				}
				
				_writer->WriteRow(dateMJD*86400.0, dateMJD*86400.0, antenna1, antenna2, 0.0, 0.0, 0.0, _mwaConfig.Header().integrationTime, _outputData.get(), _outputFlags.get(), _outputWeights.get());
			}
		}
	}
}

void Cotter::CalculateUVW(double date, size_t antenna1, size_t antenna2, double &u, double &v, double &w)
{
	// TODO we could cache the station uvw per timestep for improved performance
	
	Geometry::UVWTimestepInfo uvwInfo;
	Geometry::PrepareTimestepUVW(uvwInfo, date/86400.0, _mwaConfig.ArrayLongitudeRad(), _mwaConfig.ArrayLattitudeRad(), _mwaConfig.Header().raHrs, _mwaConfig.Header().decDegs);
	const double
		x1 = _mwaConfig.Antenna(antenna1).position[0],
		y1 = _mwaConfig.Antenna(antenna1).position[1],
		z1 = _mwaConfig.Antenna(antenna1).position[2],
		x2 = _mwaConfig.Antenna(antenna2).position[0],
		y2 = _mwaConfig.Antenna(antenna2).position[1],
		z2 = _mwaConfig.Antenna(antenna2).position[2];
	double u1, v1, w1, u2, v2, w2;
	Geometry::CalcUVW(uvwInfo, x1, y1, z1, u1, v1, w1);
	Geometry::CalcUVW(uvwInfo, x2, y2, z2, u2, v2, w2);
	u = u1 - u2,
	v = v1 - v2,
	w = w1 - w2;
}

void Cotter::baselineProcessThreadFunc()
{
	try {
		QualityStatistics threadStatistics =
			_flagger.MakeQualityStatistics(&_scanTimes[_curChunkStart], _curChunkEnd-_curChunkStart, &_channelFrequenciesHz[0], _channelFrequenciesHz.size(), 4, _collectHistograms);
		Strategy strategy;
		if(_rfiDetection)
			strategy = _flagger.LoadStrategyFile(_strategyFilename);
		
		std::unique_lock<std::mutex> lock(_mutex);
		while(!_baselinesToProcess.empty())
		{
			std::pair<size_t, size_t> baseline = _baselinesToProcess.front();
			size_t currentTaskCount = _baselinesToProcess.size();
			_progressBar->SetProgress(_baselinesToProcessCount - currentTaskCount, _baselinesToProcessCount);
			_baselinesToProcess.pop();
			lock.unlock();
			
			processBaseline(baseline.first, baseline.second, strategy, threadStatistics);
			lock.lock();
		}
		
		// Mutex still needs to be locked
		if(!_statistics)
			_statistics.reset(new QualityStatistics(threadStatistics));
		else
			(*_statistics) += threadStatistics;
	}
	catch(std::exception& exception)
	{
		std::cout <<
			"***\n"
			"*** Exception occurred while processing the baselines!\n"
			"***\n"
			"Error message:\n"
			<< exception.what();
		std::terminate();
	}
}

void Cotter::processBaseline(size_t antenna1, size_t antenna2, aoflagger::Strategy& strategy, QualityStatistics& statistics)
{
	ImageSet& imageSet = _imageSetBuffers.find(std::pair<size_t,size_t>(antenna1, antenna2))->second;
	const MWAInput
		&input1X = _mwaConfig.AntennaXInput(antenna1),
		&input1Y = _mwaConfig.AntennaYInput(antenna1),
		&input2X = _mwaConfig.AntennaXInput(antenna2),
		&input2Y = _mwaConfig.AntennaYInput(antenna2);
		
	// Correct conjugated baselines
	if(_reader->IsConjugated(antenna1, antenna2, 0, 0)) {
		correctConjugated(imageSet, 1);
	}
	if(_reader->IsConjugated(antenna1, antenna2, 0, 1)) {
		correctConjugated(imageSet, 3);
	}
	if(_reader->IsConjugated(antenna1, antenna2, 1, 0)) {
		correctConjugated(imageSet, 5);
	}
	if(_reader->IsConjugated(antenna1, antenna2, 1, 1)) {
		correctConjugated(imageSet, 7);
	}
	
	// Correct cable delay
    if(_doCorrectCableLength) {
	correctCableLength(imageSet, 0, input2X.cableLenDelta - input1X.cableLenDelta);
	correctCableLength(imageSet, 1, input2Y.cableLenDelta - input1X.cableLenDelta);
	correctCableLength(imageSet, 2, input2X.cableLenDelta - input1Y.cableLenDelta);
	correctCableLength(imageSet, 3, input2Y.cableLenDelta - input1Y.cableLenDelta);
    }
	
	// Correct passband
	for(size_t i=0; i!=8; ++i)
	{
		const double* subbandGains1Ptr = (i<4) ? input1X.pfbGains : input1Y.pfbGains;
		const double* subbandGains2Ptr = (i==0 || i==1 || i==4 || i==5) ? input2X.pfbGains : input2Y.pfbGains;
		
		const size_t channelsPerSubband = imageSet.Height()/(_curSbEnd - _curSbStart);
		for(size_t sb=0; sb!=_curSbEnd - _curSbStart; ++sb)
		{
			double subbandGainCorrection = 1.0 / (subbandGains1Ptr[sb+_curSbStart] * subbandGains2Ptr[sb+_curSbStart]);
			
			for(size_t ch=0; ch!=channelsPerSubband; ++ch)
			{
				float *channelPtr = imageSet.ImageBuffer(i) + (ch+sb*channelsPerSubband) * imageSet.HorizontalStride();
				const float correctionFactor = _subbandCorrectionFactors[i/2][ch] * subbandGainCorrection;
				for(size_t x=0; x!=imageSet.Width(); ++x)
				{
					*channelPtr *= correctionFactor;
					++channelPtr;
				}
			}
		}
	}
	
	FlagMask flagMask;
	FlagMask *correlatorMask;
	// Perform RFI detection, if baseline is not flagged.
	bool skipFlagging = input1X.isFlagged || input1Y.isFlagged || input2X.isFlagged || input2Y.isFlagged || _isAntennaFlaggedMap[antenna1] || _isAntennaFlaggedMap[antenna2];
	if(skipFlagging)
	{
		if(_flagFileTemplate.empty())
			flagMask = _fullysetMask;
		else
			flagMask = std::move(_flagBuffers.find(std::pair<size_t, size_t>(antenna1, antenna2))->second);
		correlatorMask = &_fullysetMask;
	}
	else 
	{
		correlatorMask = &_correlatorMask;
		if(!_flagFileTemplate.empty())
		{
			flagMask = std::move(_flagBuffers.find(std::pair<size_t, size_t>(antenna1, antenna2))->second);
			if(antenna1 == antenna2)
			{
				flagMask = _flagger.MakeFlagMask(_curChunkEnd-_curChunkStart, _reader->ChannelCount(), false);
			}
		}
		else if(_rfiDetection && (antenna1 != antenna2))
			flagMask = strategy.Run(imageSet, *correlatorMask);
		else
			flagMask = _flagger.MakeFlagMask(_curChunkEnd-_curChunkStart, _reader->ChannelCount(), false);
		flagBadCorrelatorSamples(flagMask);
	}
	
	// Collect statistics
	if(_collectStatistics)
		statistics.CollectStatistics(imageSet, flagMask, *correlatorMask, antenna1, antenna2);
	
	// If this is an auto-correlation, it wouldn't have been flagged yet
	// to allow collecting its statistics. But we want to flag it...
	if(antenna1 == antenna2 && _flagAutos)
	{
		flagMask = _fullysetMask;
	}
	
	_flagBuffers.find(std::pair<size_t, size_t>(antenna1, antenna2))->second = std::move(flagMask);
}

void Cotter::correctConjugated(ImageSet& imageSet, size_t imgImageIndex) const
{
	float *imags = imageSet.ImageBuffer(imgImageIndex);
	for(size_t y=0; y!=imageSet.Height(); ++y)
	{
		for(size_t x=0; x!=imageSet.HorizontalStride(); ++x)
		{
			*imags = -*imags;
			++imags;
		}
	}
}

void Cotter::correctCableLength(ImageSet& imageSet, size_t polarization, double cableDelay) const
{
	float *reals = imageSet.ImageBuffer(polarization*2);
	float *imags = imageSet.ImageBuffer(polarization*2+1);
	
	for(size_t y=0; y!=imageSet.Height(); ++y)
	{
		double angle = -2.0 * M_PI * cableDelay * _channelFrequenciesHz[y] / SPEED_OF_LIGHT;
		double rotSinl, rotCosl;
		sincos(angle, &rotSinl, &rotCosl);
		float rotSin = rotSinl, rotCos = rotCosl;
		
		/// @todo This should use actual time step count in window
		float *realPtr = reals + y * imageSet.HorizontalStride();
		float *imagPtr = imags + y * imageSet.HorizontalStride();
		for(size_t x=0; x!=imageSet.Width(); ++x)
		{
			float r = *realPtr;
			*realPtr = rotCos * r - rotSin * (*imagPtr);
			*imagPtr = rotSin * r + rotCos * (*imagPtr);
			++realPtr;
			++imagPtr;
		}
	}
}

void Cotter::writeAntennae()
{
	double arrayX, arrayY, arrayZ;
	Geometry::Geodetic2XYZ(_mwaConfig.ArrayLattitudeRad(), _mwaConfig.ArrayLongitudeRad(), _mwaConfig.ArrayHeightMeters(), arrayX, arrayY, arrayZ);
	_writer->SetArrayLocation(arrayX, arrayY, arrayZ);
	
	if(_writer->AreAntennaPositionsLocal())
		std::cout << "Antenna positions are written in LOCAL MERIDIAN\n";
	
	std::vector<MSWriter::AntennaInfo> antennae(_mwaConfig.NAntennae());
	
	_isAntennaFlaggedMap.reset(new bool[antennae.size()]);
	
	for(size_t i=0; i!=antennae.size(); ++i)
		_isAntennaFlaggedMap[i] = false;
	
	for(std::vector<size_t>::const_iterator userFlagIter=_userFlaggedAntennae.begin();
			userFlagIter!=_userFlaggedAntennae.end(); ++userFlagIter)
		_isAntennaFlaggedMap[*userFlagIter] = true;
	
	_unflaggedAntennaCount = 0;
	for(size_t i=0; i!=_mwaConfig.NAntennae(); ++i)
	{
		const MWAAntenna &mwaAnt = _mwaConfig.Antenna(i);
		MSWriter::AntennaInfo &antennaInfo = antennae[i];
		antennaInfo.name = mwaAnt.name;
		antennaInfo.station = "MWA";
		antennaInfo.type = "GROUND-BASED";
		antennaInfo.mount = "ALT-AZ"; // TODO should be "FIXED", but Casa does not like
		double
			x = mwaAnt.position[0],
			y = mwaAnt.position[1],
			z = mwaAnt.position[2];
		// The following rotation is necessary because we found that the XYZ locations are
		// still in local frame (local meridian). However, the UVW calculations depend on
		// this assumption, so that's why I rotate them only when writing...
		if(!_writer->AreAntennaPositionsLocal())
		{
			Geometry::Rotate(_mwaConfig.ArrayLongitudeRad(), x, y);
			x += arrayX;
			y += arrayY;
			z += arrayZ;
		}
		antennaInfo.x = x;
		antennaInfo.y = y;
		antennaInfo.z = z;
		antennaInfo.diameter = 4; /** TODO can probably give more exact size! */
		bool isFlagged =
			_mwaConfig.AntennaXInput(i).isFlagged || _mwaConfig.AntennaYInput(i).isFlagged || _isAntennaFlaggedMap[i];
		antennaInfo.flag = isFlagged;
		_isAntennaFlaggedMap[i] = isFlagged;
		if(!isFlagged)
			_unflaggedAntennaCount++;
	}
	_writer->WriteAntennae(antennae, _mwaConfig.Header().dateFirstScanMJD*86400.0);
}

void Cotter::writeSPW()
{
	const size_t nCurChannels = nChannelsInCurSBRange();
	std::vector<MSWriter::ChannelInfo> channels(nCurChannels);
	std::ostringstream str;
	double centreFrequencyMHz = 0.0000005 * (_channelFrequenciesHz[nCurChannels/2-1] + _channelFrequenciesHz[nCurChannels/2]);
	str << "MWA_BAND_" << (round(centreFrequencyMHz*10.0)/10.0);
	const double chWidth = _mwaConfig.Header().bandwidthMHz * 1000000.0 / _mwaConfig.Header().nChannels;
	for(size_t ch=0;ch!=nCurChannels;++ch)
	{
		MSWriter::ChannelInfo &channel = channels[ch];
		channel.chanFreq = _channelFrequenciesHz[ch];
		channel.chanWidth = chWidth;
		channel.effectiveBW = chWidth;
		channel.resolution = chWidth;
	}
	_writer->WriteBandInfo(str.str(),
		channels,
		centreFrequencyMHz*1000000.0,
		nCurChannels*chWidth,
		false
	);
}

void Cotter::writeSource()
{
	const MWAHeader &header = _mwaConfig.Header();
	MSWriter::SourceInfo source;
	source.sourceId = 0;
	source.time = (header.dateFirstScanMJD + header.GetDateLastScanMJD()) * (86400.0/ 2.0);
	source.interval = header.GetDateLastScanMJD() - header.dateFirstScanMJD;
	source.spectralWindowId = 0;
	source.numLines = 0;
	source.name = header.fieldName;
	source.calibrationGroup = 0;
	source.code = "";
	source.directionRA = header.raHrs * (M_PI/12.0);
	source.directionDec = header.decDegs * (M_PI/180.0);
	source.properMotion[0] = 0.0;
	source.properMotion[1] = 0.0;
	_writer->WriteSource(source);
}

void Cotter::writeField()
{
	const MWAHeader &header = _mwaConfig.Header();
	MSWriter::FieldInfo field;
	field.name = header.fieldName;
	field.code = std::string();
	field.time = header.GetStartDateMJD() * 86400.0;
	field.numPoly = 0;
	field.delayDirRA = header.raHrs * (M_PI/12.0);
	field.delayDirDec = header.decDegs * (M_PI/180.0);
	field.phaseDirRA = field.delayDirRA;
	field.phaseDirDec = field.delayDirDec;
	field.referenceDirRA = field.delayDirRA;
	field.referenceDirDec = field.delayDirDec;
	field.sourceId = -1;
	field.flagRow = false;
	_writer->WriteField(field);
}

void Cotter::writeObservation()
{
	Writer::ObservationInfo observation;
	observation.telescopeName = "MWA";
	observation.startTime = _mwaConfig.Header().dateFirstScanMJD*86400.0;
	observation.endTime = _mwaConfig.Header().GetDateLastScanMJD()*86400.0;
	observation.observer = _mwaConfig.HeaderExt().observerName;
	if(observation.observer.empty())
		observation.observer = "Unknown";
	observation.scheduleType = "MWA";
	observation.project = _mwaConfig.HeaderExt().projectName;
	if(observation.project.empty())
		observation.project = "Unknown";
	observation.releaseDate = 0;
	observation.flagRow = false;
	
	_writer->WriteObservation(observation);
}

void Cotter::readSubbandPassbandFile()
{
	std::ifstream passbandFile(_subbandPassbandFilename.c_str());
	if(!passbandFile.good())
		throw std::runtime_error("Unable to read subband passband file");
	while(passbandFile.good())
	{
		size_t rowIndex;
		passbandFile >> rowIndex;
		if(!passbandFile.good()) break;
		for(size_t p=0; p!=4; ++p) {
			double passbandGain;
			passbandFile >> passbandGain;
			_subbandCorrectionFactors[p].push_back(1.0/passbandGain);
		}
	}
	size_t
		channelsPerSubband = _subbandCorrectionFactors[0].size(),
		subBandCount = _mwaConfig.Header().nChannels / channelsPerSubband;
	std::cout << "Read subband passband, using " << channelsPerSubband << " gains to correct for " << subBandCount << " subbands.\n";
	if(subBandCount != _subbandCount)
	{
		std::ostringstream str;
		str
			<< "Unexpected number of channels in subband passband correction file: "
			<< "file implies " << subBandCount << " sub-bands, but I expect " << _subbandCount << ".";
		throw std::runtime_error(str.str());
	}
}

void Cotter::initializeSubbandPassband()
{
	if(_mwaConfig.Header().nChannels % _subbandCount != 0)
		throw std::runtime_error("Total number of channels is not divisable by subband count!");

	const size_t nChannelsPerSubband = _mwaConfig.Header().nChannels / _subbandCount;
	std::vector<double> passband;
	SubbandPassband::GetSubbandPassband(passband, nChannelsPerSubband);
	
	for(size_t p=0; p!=4; ++p)
		_subbandCorrectionFactors[p].resize(nChannelsPerSubband);
	
	std::cout << "Using a-priori subband passband with " << nChannelsPerSubband << " channels.\n";
	for(size_t ch=0; ch!=nChannelsPerSubband; ++ch)
	{
		double correctionFactor = 1.0 / passband[ch];
		for(size_t p=0; p!=4; ++p) {
			_subbandCorrectionFactors[p][ch] = correctionFactor;
		}
	}
}

void Cotter::initPerInputSubbandGains()
{
	if(_applySBGains)
	{
		std::vector<double> gains(_subbandCount, 0.0);
		if(_mwaConfig.HeaderExt().hasGlobalSubbandGains)
		{
			std::cout << "Using global subband gains from meta-fits file: ";
			for(size_t sb=0; sb!=_subbandCount; ++sb)
			{
				double gain = _mwaConfig.HeaderExt().subbandGains[sb];
				gains[sb] = gain;
				for(size_t inpIndex=0; inpIndex!=_mwaConfig.NAntennae()*2; ++inpIndex)
				{
					MWAInput& input = _mwaConfig.InputRW(inpIndex);
					input.pfbGains[sb] = gain;
				}
			}
		}
		else {
			std::cout << "Using per-input subband gains. Average gains: ";
			for(size_t inpIndex=0; inpIndex!=_mwaConfig.NAntennae()*2; ++inpIndex)
			{
				const MWAInput& input = _mwaConfig.Input(inpIndex);
				for(size_t sb=0; sb!=_subbandCount; ++sb)
					gains[sb] += input.pfbGains[sb];
			}
			for(size_t sb=0; sb!=_subbandCount; ++sb)
				gains[sb] /= _mwaConfig.NAntennae()*2;
		}
		std::cout << gains[0];
		for(size_t sb=1; sb!=_subbandCount; ++sb)
			std::cout << ',' << gains[sb];
		std::cout << '\n';
	}
	else {
		std::cout << "Subband gains are disabled.\n";
		for(size_t inpIndex=0; inpIndex!=_mwaConfig.NAntennae()*2; ++inpIndex)
		{
			MWAInput& input = _mwaConfig.InputRW(inpIndex);
			for(size_t sb=0; sb!=_subbandCount; ++sb)
			{
				input.pfbGains[sb] = 1.0;
			}
		}
	}
}

void Cotter::flagBadCorrelatorSamples(FlagMask &flagMask) const
{
	// Flag MWA side and centre channels
	const size_t
		scanCount = _curChunkEnd - _curChunkStart,
		curSBCount = _curSbEnd - _curSbStart,
		chPerSb = flagMask.Height() / curSBCount;
	for(size_t sb=0; sb!=curSBCount; ++sb)
	{
		bool *sbStart = flagMask.Buffer() + (sb*chPerSb)*flagMask.HorizontalStride();
		
		// Flag first edges of sb
		for(size_t ch=0; ch!=_subbandEdgeFlagCount; ++ch)
		{
			bool *channelPtr = sbStart + ch * flagMask.HorizontalStride();
			bool *endPtr = sbStart + ch * flagMask.HorizontalStride() + scanCount;
			while(channelPtr != endPtr) { *channelPtr=true; ++channelPtr; }
		}
		
		// Flag centre channel of sb
		if(_flagDCChannels)
		{
			size_t halfBand = chPerSb/2;
			bool *channelPtr = sbStart + halfBand*flagMask.HorizontalStride();
			bool *endPtr = sbStart + halfBand*flagMask.HorizontalStride() + scanCount;
			while(channelPtr != endPtr) { *channelPtr=true; ++channelPtr; }
		}
		
		// Flag last edge channels of sb
		for(size_t ch=chPerSb-_subbandEdgeFlagCount; ch!=chPerSb; ++ch)
		{
			bool *channelPtr = sbStart + ch * flagMask.HorizontalStride();
			bool *endPtr = sbStart + ch * flagMask.HorizontalStride() + scanCount;
			while(channelPtr != endPtr) { *channelPtr=true; ++channelPtr; }
		}
	}
	
	// Flag subbands that have been requested to be flagged
	for(std::set<size_t>::const_iterator sbIter=_flaggedSubbands.begin();
			sbIter!=_flaggedSubbands.end(); ++sbIter)
	{
		if(*sbIter >= _curSbStart && *sbIter < _curSbEnd)
		{
			size_t sb = *sbIter - _curSbStart;
			bool *sbStart = flagMask.Buffer() + (sb*chPerSb)*flagMask.HorizontalStride();
			for(size_t ch=0; ch!=chPerSb; ++ch)
			{
				bool *channelPtr = sbStart + ch*flagMask.HorizontalStride();
				bool *endPtr = sbStart + ch*flagMask.HorizontalStride() + scanCount;
				while(channelPtr != endPtr) { *channelPtr=true; ++channelPtr; }
			}
		}
	}
	
	// Flag first samples
	if(_quackInitSampleCount >= _curChunkStart)
	{
		for(size_t ch=0; ch!=flagMask.Height(); ++ch)
		{
			bool *channelPtr = flagMask.Buffer() + ch*flagMask.HorizontalStride();
			const size_t count = std::min(_quackInitSampleCount - _curChunkStart, flagMask.Width());
			for(size_t x=0; x!=count; ++x)
			{
				*channelPtr = true;
				++channelPtr;
			}
		}
	}
	
	// If samples are missing at the end, flag them.
	for(size_t ch=0; ch!=flagMask.Height(); ++ch)
	{
		bool *channelPtr = flagMask.Buffer() + ch*flagMask.HorizontalStride() + scanCount - _missingEndScans;
		for(size_t t=scanCount - _missingEndScans; t!=scanCount; ++t)
		{
			*channelPtr = true;
			++channelPtr;
		}
	}
}

void Cotter::initializeWeights(aligned_ptr<float>& outputWeights)
{
	// Weights are normalized so that default res of 10 kHz, 1s has weight of "1" per sample
	// Note that this only holds for numbers in the WEIGHTS_SPECTRUM column; WEIGHTS will hold the sum.
	double weightFactor = _mwaConfig.Header().integrationTime * (100.0*_mwaConfig.Header().bandwidthMHz/_mwaConfig.Header().nChannels);
	size_t curSBRangeSize = _curSbEnd - _curSbStart;
	for(size_t sb=0; sb!=curSBRangeSize; ++sb)
	{
		size_t channelsPerSubband = _mwaConfig.Header().nChannels / _subbandCount;
		for(size_t ch=0; ch!=channelsPerSubband; ++ch)
		{
			for(size_t p=0; p!=4; ++p)
				outputWeights[(ch+channelsPerSubband*sb)*4 + p] = weightFactor / (_subbandCorrectionFactors[p][ch]);
		}
	}
}

void Cotter::initializeSbOrder()
{
	_subbandOrder.resize(_subbandCount);
	size_t sb = 0;
	while(sb != _subbandCount)
	{
		size_t coarseCh = _mwaConfig.HeaderExt().subbandNumbers[sb];
		if(coarseCh>128)
			break;
		_subbandOrder[sb] = sb;
		++sb;
	}
	size_t rightSB = _subbandCount, stopPoint = sb;
	while(rightSB != stopPoint)
	{
		--rightSB;
		_subbandOrder[rightSB] = sb;
		++sb;
	}
	
	std::cout << "Subband order: " << _subbandOrder[0];
	for(size_t i=1; i!=_subbandCount; ++i)
		std::cout << ',' << _subbandOrder[i];
	std::cout << '\n';
	
	if(_doFlagMissingSubbands)
	{
		for(size_t i=0; i!=_subbandCount; ++i)
		{
			if(isGPUBoxMissing(i))
			{
				size_t sb = _subbandOrder[i];
				std::cout << "At least one file is missing from gpubox " << (i+1) << ": flagging subband " << sb << ".\n";
				FlagSubband(sb);
			}
		}
	}
}

void Cotter::writeAlignmentScans()
{
	if(!_writer->IsTimeAligned(0, 0))
	{
		const size_t nChannels = nChannelsInCurSBRange();
		const size_t antennaCount = _mwaConfig.NAntennae();
		std::cout << "Nr of timesteps did not match averaging size, last averaged sample will be downweighted" << std::flush;
		size_t timeIndex = _mwaConfig.Header().nScans;
		
		_outputFlags.reset(new bool[nChannels*4]);
		_outputData = make_aligned<std::complex<float>>(nChannels*4, 16);
		_outputWeights = make_aligned<float>(nChannels*4, 16);
		for(size_t ch=0; ch!=nChannels*4; ++ch)
		{
			_outputData[ch] = std::complex<float>(0.0, 0.0);
			_outputFlags[ch] = true;
			_outputWeights[ch] = 0.0;
		}
		while(!_writer->IsTimeAligned(0, 0))
		{
			_writer->AddRows(rowsPerTimescan());
			const double dateMJD = _mwaConfig.Header().dateFirstScanMJD + timeIndex * _mwaConfig.Header().integrationTime/86400.0;
			for(size_t antenna1=0;antenna1!=antennaCount;++antenna1)
			{
				for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
				{
					if(outputBaseline(antenna1, antenna2))
					{
						_writer->WriteRow(dateMJD*86400.0, dateMJD*86400.0, antenna1, antenna2, 0.0, 0.0, 0.0, _mwaConfig.Header().integrationTime, _outputData.get(), _outputFlags.get(), _outputWeights.get());
					}
				}
			}
			++timeIndex;
			std::cout << '.' << std::flush;
		}
		_outputData.reset();
		_outputWeights.reset();
		_outputFlags.reset();
		std::cout << '\n';
	}
}

void Cotter::writeMWAFieldsToMS(const std::string& outputFilename, size_t flagWindowSize)
{
	MWAMS mwaMs(outputFilename);
	mwaMs.InitializeMWAFields();
	
	size_t nAnt = _mwaConfig.NAntennae();
	for(size_t i=0; i!=nAnt; ++i)
	{
		const MWAAntenna &ant = _mwaConfig.Antenna(i);
		const MWAInput &inpX = _mwaConfig.AntennaXInput(i);
		const MWAInput &inpY = _mwaConfig.AntennaYInput(i);
		
		MWAMS::MWAAntennaInfo antennaInfo;
		antennaInfo.inputX = inpX.inputIndex;
		antennaInfo.inputY = inpY.inputIndex;
		antennaInfo.tileNr = ant.tileNumber;
		antennaInfo.receiver = ant.receiver;
		antennaInfo.slotX = inpX.slot;
		antennaInfo.slotY = inpY.slot;
		antennaInfo.cableLengthX = inpX.cableLenDelta;
		antennaInfo.cableLengthY = inpY.cableLenDelta;
		
		mwaMs.UpdateMWAAntennaInfo(i, antennaInfo);
	}
	
	mwaMs.UpdateMWAFieldInfo(_mwaConfig.HeaderExt().hasCalibrator);
	
	MWAMS::MWAObservationInfo obsInfo;
	obsInfo.gpsTime = _mwaConfig.HeaderExt().gpsTime;
	obsInfo.filename = _mwaConfig.HeaderExt().filename;
	obsInfo.observationMode = _mwaConfig.HeaderExt().mode;
	obsInfo.rawFileCreationDate = 0;
	obsInfo.flagWindowSize = flagWindowSize;
	obsInfo.dateRequested = _mwaConfig.HeaderExt().dateRequestedMJD*86400.0;
	mwaMs.UpdateMWAObservationInfo(obsInfo);
	
	mwaMs.UpdateSpectralWindowInfo(_mwaConfig.CentreSubbandNumber());
	
	mwaMs.WriteMWATilePointingInfo(_mwaConfig.Header().GetStartDateMJD()*86400.0,
		_mwaConfig.Header().GetDateLastScanMJD()*86400.0, _mwaConfig.HeaderExt().delays,
		_mwaConfig.HeaderExt().tilePointingRARad, _mwaConfig.HeaderExt().tilePointingDecRad);
	
	for(int i=0; i!=24; ++i)
		mwaMs.WriteMWASubbandInfo(i, _mwaConfig.HeaderExt().subbandGains[i], false);
	
	mwaMs.WriteMWAKeywords(_mwaConfig.HeaderExt().metaDataVersion, _mwaConfig.HeaderExt().mwaPyVersion, COTTER_VERSION_STR, COTTER_VERSION_DATE);
}

void Cotter::writeMWAFieldsToUVFits(const std::string& outputFilename)
{
	MWAFits mwaFits(outputFilename);
	mwaFits.WriteMWAKeywords(_mwaConfig.HeaderExt().metaDataVersion, _mwaConfig.HeaderExt().mwaPyVersion, COTTER_VERSION_STR, COTTER_VERSION_DATE);
}

void Cotter::onHDUOffsetsChange(const std::vector<int>& newHDUOffsets)
{
	bool isChanged = false;
	for(size_t sb=_curSbStart; sb!=_curSbEnd; ++sb)
	{
		size_t gpuboxIndex = _subbandOrder[sb];
		if(_hduOffsetsPerGPUBox[gpuboxIndex] == 9999) {
			_hduOffsetsPerGPUBox[gpuboxIndex] = newHDUOffsets[sb-_curSbStart];
			isChanged = true;
		}
		else if(_hduOffsetsPerGPUBox[gpuboxIndex] != newHDUOffsets[sb-_curSbStart])
			std::cout << "WARNING! The HDU offsets change over time, this should never happen!\n";
	}
	
	if(isChanged) {
		if(_doAlign)
			_writer->SetOffsetsPerGPUBox(_hduOffsetsPerGPUBox);
		else {
			std::vector<int> zeros(newHDUOffsets.size(), 0);
			_writer->SetOffsetsPerGPUBox(zeros);
		}
	}
}
