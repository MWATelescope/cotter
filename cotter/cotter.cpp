#include "cotter.h"

#include "baselinebuffer.h"
#include "geometry.h"
#include "mswriter.h"
#include "mwams.h"

#include <aoflagger.h>

#include <boost/thread/thread.hpp>

#include <fstream>
#include <iostream>
#include <map>
#include <cmath>
#include <complex>

#include <xmmintrin.h>

#define SPEED_OF_LIGHT 299792458.0        // speed of light in m/s

#define USE_SSE

using namespace aoflagger;

Cotter::Cotter() :
	_writer(0),
	_reader(0),
	_flagger(0),
	_strategy(0),
	_threadCount(1),
	_maxBufferSize(0),
	_subbandCount(24),
	_quackSampleCount(4),
	_rfiDetection(true),
	_collectStatistics(true),
	_metaFilename(),
	_statistics(0),
	_correlatorMask(0),
	_fullysetMask(0)
{
}

Cotter::~Cotter()
{
	delete _correlatorMask;
	delete _fullysetMask;
	delete _writer;
	delete _reader;
	delete _flagger;
	delete _strategy;
	delete _statistics;
}

void Cotter::Run(const char *outputFilename, size_t timeAvgFactor, size_t freqAvgFactor)
{
	bool lockPointing = false;
	
	if(_metaFilename.empty())
	{
		_mwaConfig.ReadHeader("header.txt", lockPointing);
		_mwaConfig.ReadInputConfig("instr_config.txt");
		_mwaConfig.ReadAntennaPositions("antenna_locations.txt");
	} else {
		_mwaConfig.ReadMetaFits(_metaFilename.c_str(), lockPointing);
	}
	_mwaConfig.CheckSetup();
	
	_channelFrequenciesHz.resize(_mwaConfig.Header().nChannels);
	for(size_t ch=0; ch!=_mwaConfig.Header().nChannels; ++ch)
		_channelFrequenciesHz[ch] = _mwaConfig.ChannelFrequencyHz(ch);
	
	readSubbandPassbandFile();
	readSubbandGainsFile();
	initializeSbOrder(_mwaConfig.CentreSubbandNumber());
	
	_flagger = new AOFlagger();
	
	const size_t nChannels = _mwaConfig.Header().nChannels;
	const size_t antennaCount = _mwaConfig.NAntennae();
	size_t maxScansPerPart = _maxBufferSize / (nChannels*(antennaCount+1)*antennaCount*2);
	
	if(freqAvgFactor == 1 && timeAvgFactor == 1)
		_writer = new MSWriter(outputFilename);
	else
		_writer = new AveragingMSWriter(outputFilename, timeAvgFactor, freqAvgFactor, *this);
	writeAntennae();
	writeSPW();
	writeSource();
	writeField();
	_writer->WritePolarizationForLinearPols(false);
	writeObservation();
		
	if(maxScansPerPart<1)
	{
		std::cout << "WARNING! The given amount of memory is not enough for even one scan; expect swapping and very poor flagging accuracy.\n";
		maxScansPerPart = 1;
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
	
	_strategy = new Strategy(_flagger->MakeStrategy(MWA_TELESCOPE));
	
	std::vector<std::vector<std::string> >::const_iterator
		currentFileSetPtr = _fileSets.begin();
	createReader(*currentFileSetPtr);
	for(size_t chunkIndex = 0; chunkIndex != partCount; ++chunkIndex)
	{
		_curChunkStart = _mwaConfig.Header().nScans*chunkIndex/partCount;
		_curChunkEnd = _mwaConfig.Header().nScans*(chunkIndex+1)/partCount;
		
		// Initialize buffers
		for(size_t antenna1=0;antenna1!=antennaCount;++antenna1)
		{
			for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
			{
				ImageSet *imageSet = new ImageSet(_flagger->MakeImageSet(_curChunkEnd-_curChunkStart, nChannels, 8));
				_imageSetBuffers.insert(std::pair<std::pair<size_t,size_t>, ImageSet*>(
					std::pair<size_t,size_t>(antenna1, antenna2), imageSet
				));
			}
		}
		
		size_t bufferPos = 0;
		bool continueWithNextFile;
		do {
			initializeReader();
			
			bool moreAvailableInCurrentFile = _reader->Read(bufferPos, _curChunkEnd-_curChunkStart);
			
			if(!moreAvailableInCurrentFile && bufferPos < (_curChunkEnd-_curChunkStart))
			{
				// Go to the next set of GPU files and add them to the buffer
				++currentFileSetPtr;
				continueWithNextFile = (currentFileSetPtr!=_fileSets.end());
				if(continueWithNextFile)
					createReader(*currentFileSetPtr);
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
		
		_fullysetMask = new FlagMask(_flagger->MakeFlagMask(_curChunkEnd-_curChunkStart, _reader->ChannelCount(), true));
		_correlatorMask = new FlagMask(_flagger->MakeFlagMask(_curChunkEnd-_curChunkStart, _reader->ChannelCount(), false));
		flagBadCorrelatorSamples(*_correlatorMask);
		
		for(size_t antenna1=0;antenna1!=antennaCount;++antenna1)
		{
			for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
			{
				_baselinesToProcess.push(std::pair<size_t,size_t>(antenna1, antenna2));
				
				// We will put a place holder in the flagbuffer map, so we don't have to write (and lock)
				// during multi threaded processing.
				_flagBuffers.insert(std::pair<std::pair<size_t, size_t>, FlagMask*>(
				std::pair<size_t,size_t>(antenna1, antenna2), 0));
			}
		}
		if(_rfiDetection)
		{
			if(_collectStatistics)
				std::cout << "RFI detection, stats, conjugations, subband ordering and cable length corrections" << std::flush;
			else
				std::cout << "RFI detection, conjugations, subband ordering and cable length corrections" << std::flush;
		}
		else
		{
			if(_collectStatistics)
				std::cout << "Stats, conjugations, subband ordering and cable length corrections" << std::flush;
			else
				std::cout << "Conjugations, subband ordering and cable length corrections" << std::flush;
		}
		boost::thread_group threadGroup;
		for(size_t i=0; i!=_threadCount; ++i)
			threadGroup.create_thread(boost::bind(&Cotter::baselineProcessThreadFunc, this));
		threadGroup.join_all();
		std::cout << '\n';
		
		if(_mwaConfig.Header().geomCorrection)
			std::cout << "Will apply geometric delay correction.\n";
		
		std::cout << "Writing" << std::flush;
		_outputFlags = new bool[nChannels*4];
		posix_memalign((void**) &_outputData, 16, nChannels*4*sizeof(std::complex<float>));
		posix_memalign((void**) &_outputWeights, 16, nChannels*4*sizeof(float));
		for(size_t t=_curChunkStart; t!=_curChunkEnd; ++t)
		{
			processAndWriteTimestep(t);
			std::cout << '.' << std::flush;
		}
		free(_outputData);
		free(_outputWeights);
		delete[] _outputFlags;
		std::cout << '\n';
		
		for(size_t antenna1=0;antenna1!=antennaCount;++antenna1)
		{
			for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
			{
				delete _flagBuffers.find(std::pair<size_t, size_t>(antenna1, antenna2))->second;
				delete _imageSetBuffers.find(std::pair<size_t, size_t>(antenna1, antenna2))->second;
			}
		}
		_flagBuffers.clear();
		_imageSetBuffers.clear();
		
		delete _correlatorMask;
		_correlatorMask = 0;
		delete _fullysetMask;
		_fullysetMask = 0;
	} // end for chunkIndex!=partCount
	
	writeAlignmentScans();
	
	std::vector<std::string> params;
	std::stringstream paramStr;
	paramStr << "timeavg=" << timeAvgFactor << ",freqavg=" << freqAvgFactor << ",windowSize=" << (_mwaConfig.Header().nScans/partCount);
	params.push_back(paramStr.str());
	_writer->WriteHistoryItem(_commandLine, "Cotter MWA preprocessor", params);
	
	delete _writer;
	_writer = 0;
	delete _reader;
	_reader = 0;
	
	if(_collectStatistics) {
		std::cout << "Writing statistics to file...\n";
		_flagger->WriteStatistics(*_statistics, outputFilename);
	}
	
	std::cout << "Writing MWA fields to file...\n";
	writeMWAFields(outputFilename, _mwaConfig.Header().nScans/partCount);
}

void Cotter::createReader(const std::vector< std::string >& curFileset)
{
	delete _reader; // might be 0, but that's ok.
	_reader = new GPUFileReader();

	for(std::vector<std::string>::const_iterator i=curFileset.begin(); i!=curFileset.end(); ++i)
		_reader->AddFile(i->c_str());
	
	_reader->Initialize();
	
	if(_reader->AntennaCount() != _mwaConfig.NAntennae())
		throw std::runtime_error("nAntennae in GPU files do not match nAntennae in config");
	if(_reader->ChannelCount() != _mwaConfig.Header().nChannels)
		throw std::runtime_error("nChannels in GPU files do not match nChannels in config");
}

void Cotter::initializeReader()
{
	const size_t antennaCount = _mwaConfig.NAntennae();
	
	for(size_t i=0; i!=_mwaConfig.Header().nInputs; ++i)
		_reader->SetCorrInputToOutput(i, _mwaConfig.Input(i).antennaIndex, _mwaConfig.Input(i).polarizationIndex);
	
	// Initialize buffers of reader
	for(size_t antenna1=0;antenna1!=antennaCount;++antenna1)
	{
		for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
		{
			ImageSet &imageSet = *_imageSetBuffers.find(std::pair<size_t, size_t>(antenna1, antenna2))->second;
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
	const size_t nChannels = _mwaConfig.Header().nChannels;
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
	
	_writer->AddRows(antennaCount*(antennaCount+1)/2);
	
	double cosAngles[nChannels], sinAngles[nChannels];
	initializeWeights(_outputWeights);
	for(size_t antenna1=0; antenna1!=antennaCount; ++antenna1)
	{
		for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
		{
			// TODO these statements should be more efficient
			const ImageSet &imageSet = *_imageSetBuffers.find(std::pair<size_t, size_t>(antenna1, antenna2))->second;
			const FlagMask &flagMask = *_flagBuffers.find(std::pair<size_t, size_t>(antenna1, antenna2))->second;
			
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
			
			_writer->WriteRow(dateMJD*86400.0, dateMJD*86400.0, antenna1, antenna2, u, v, w, _mwaConfig.Header().integrationTime, _outputData, _outputFlags, _outputWeights);
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
	QualityStatistics threadStatistics =
		_flagger->MakeQualityStatistics(&_scanTimes[_curChunkStart], _curChunkEnd-_curChunkStart, &_channelFrequenciesHz[0], _channelFrequenciesHz.size(), 4);
	
	boost::mutex::scoped_lock lock(_mutex);
	while(!_baselinesToProcess.empty())
	{
		std::pair<size_t, size_t> baseline = _baselinesToProcess.front();
		_baselinesToProcess.pop();
		lock.unlock();
		
		processBaseline(baseline.first, baseline.second, threadStatistics);
		std::cout << '.' << std::flush;
		lock.lock();
	}
	
	// Mutex needs to be still locked
	if(_statistics == 0)
		_statistics = new QualityStatistics(threadStatistics);
	else
		(*_statistics) += threadStatistics;
}

void Cotter::processBaseline(size_t antenna1, size_t antenna2, QualityStatistics &statistics)
{
	ImageSet *imageSet = _imageSetBuffers.find(std::pair<size_t,size_t>(antenna1, antenna2))->second;
	const MWAInput
		&input1X = _mwaConfig.AntennaXInput(antenna1),
		&input1Y = _mwaConfig.AntennaXInput(antenna1),
		&input2X = _mwaConfig.AntennaXInput(antenna2),
		&input2Y = _mwaConfig.AntennaYInput(antenna2);
		
	// Correct conjugated baselines
	if(_reader->IsConjugated(antenna1, antenna2, 0, 0)) {
		correctConjugated(*imageSet, 1);
	}
	if(_reader->IsConjugated(antenna1, antenna2, 0, 1)) {
		correctConjugated(*imageSet, 3);
	}
	if(_reader->IsConjugated(antenna1, antenna2, 1, 0)) {
		correctConjugated(*imageSet, 5);
	}
	if(_reader->IsConjugated(antenna1, antenna2, 1, 1)) {
		correctConjugated(*imageSet, 7);
	}
	
	reorderSubbands(*imageSet);
	
	// Correct cable delay
	correctCableLength(*imageSet, 0, input2X.cableLenDelta - input1X.cableLenDelta);
	correctCableLength(*imageSet, 1, input2Y.cableLenDelta - input1X.cableLenDelta);
	correctCableLength(*imageSet, 2, input2X.cableLenDelta - input1Y.cableLenDelta);
	correctCableLength(*imageSet, 3, input2Y.cableLenDelta - input1Y.cableLenDelta);
	
	// Correct passband
	for(size_t i=0; i!=8; ++i)
	{
		const size_t channelsPerSubband = imageSet->Height()/_subbandCount;
		for(size_t sb=0; sb!=_subbandCount; ++sb)
		{
			for(size_t ch=0; ch!=channelsPerSubband; ++ch)
			{
				float *channelPtr = imageSet->ImageBuffer(i) + (ch+sb*channelsPerSubband) * imageSet->HorizontalStride();
				const float correctionFactor = _subbandCorrectionFactors[i/2][ch] * _subbandGainCorrection[sb];
				for(size_t x=0; x!=imageSet->Width(); ++x)
				{
					*channelPtr *= correctionFactor;
					++channelPtr;
				}
			}
		}
	}
	
	// Perform RFI detection, if baseline is not flagged.
	bool is1Flagged = input1X.isFlagged || input1Y.isFlagged;
	bool is2Flagged = input2X.isFlagged || input2Y.isFlagged;
	bool skipFlagging = is1Flagged || is2Flagged;

	FlagMask *flagMask, *correlatorMask;
	if(skipFlagging)
	{
		flagMask = new FlagMask(*_fullysetMask);
		correlatorMask = _fullysetMask;
	}
	else 
	{
		if(_rfiDetection && (antenna1 != antenna2))
			flagMask = new FlagMask(_flagger->Run(*_strategy, *imageSet));
		else
			flagMask = new FlagMask(_flagger->MakeFlagMask(_curChunkEnd-_curChunkStart, _reader->ChannelCount(), false));
		correlatorMask = _correlatorMask;
		
		flagBadCorrelatorSamples(*flagMask);
	}
	
	_flagBuffers.find(std::pair<size_t, size_t>(antenna1, antenna2))->second = flagMask;
	
	// Collect statistics
	if(_collectStatistics)
		_flagger->CollectStatistics(statistics, *imageSet, *flagMask, *correlatorMask, antenna1, antenna2);
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
	
	std::vector<MSWriter::AntennaInfo> antennae;
	for(size_t i=0; i!=_mwaConfig.NAntennae(); ++i)
	{
		const MWAAntenna &mwaAnt = _mwaConfig.Antenna(i);
		MSWriter::AntennaInfo antennaInfo;
		antennaInfo.name = mwaAnt.name;
		antennaInfo.station = "MWA";
		antennaInfo.type = "GROUND-BASED";
		antennaInfo.mount = "ALT-AZ"; // TODO should be "FIXED", but Casa does not like
		antennaInfo.x = mwaAnt.position[0] + arrayX;
		antennaInfo.y = mwaAnt.position[1] + arrayY;
		antennaInfo.z = mwaAnt.position[2] + arrayZ;
		antennaInfo.diameter = 4; /** TODO can probably give more exact size! */
		antennaInfo.flag = false;
		
		antennae.push_back(antennaInfo);
	}
	_writer->WriteAntennae(antennae, _mwaConfig.Header().dateFirstScanMJD*86400.0);
}

void Cotter::writeSPW()
{
	std::vector<MSWriter::ChannelInfo> channels(_mwaConfig.Header().nChannels);
	std::ostringstream str;
	str << "MWA_BAND_" << (round(_mwaConfig.Header().centralFrequencyMHz*10.0)/10.0);
	for(size_t ch=0;ch!=_mwaConfig.Header().nChannels;++ch)
	{
		MSWriter::ChannelInfo &channel = channels[ch];
		channel.chanFreq = _channelFrequenciesHz[ch];
		channel.chanWidth = _mwaConfig.Header().bandwidthMHz * 1000000.0 / _mwaConfig.Header().nChannels;
		channel.effectiveBW = channel.chanWidth;
		channel.resolution = channel.chanWidth;
	}
	_writer->WriteBandInfo(str.str(),
		channels,
		_mwaConfig.Header().centralFrequencyMHz*1000000.0,
		_mwaConfig.Header().bandwidthMHz*1000000.0,
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
	std::string
		observer = _mwaConfig.HeaderExt().observerName,
		project = _mwaConfig.HeaderExt().projectName;
	if(observer.empty())
		observer = "Unknown";
	if(project.empty())
		project = "Unknown";
	_writer->WriteObservation("MWA", _mwaConfig.Header().dateFirstScanMJD*86400.0, _mwaConfig.Header().GetDateLastScanMJD()*86400.0, observer, "MWA", project, 0, false);
}

void Cotter::readSubbandPassbandFile()
{
	std::ifstream passbandFile("subband-passband.txt");
	if(!passbandFile.good())
		throw std::runtime_error("Unable to read subband-passband.txt");
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
	std::cout << "Read subband passband, using " << channelsPerSubband << " gains to correct for " << subBandCount << " channels/subband.\n";
	if(subBandCount != _subbandCount)
		throw std::runtime_error("Unexpected number of channels in subband passband correction file");
}

void Cotter::readSubbandGainsFile()
{
	std::ifstream gainsFile("subband-gains.txt");
	
	std::map<double, double> subbandGainCorrection;
	while(gainsFile.good())
	{
		std::string line;
		std::getline(gainsFile, line);
		if(!line.empty() && line[0]!='#')
		{
			std::istringstream lineStr(line);
			size_t index;
			double centralFrequency, gain;
			lineStr >> index >> centralFrequency >> gain;
			if(lineStr.fail())
				throw std::runtime_error("Could not parse subband gains file, line: " + line);
			subbandGainCorrection.insert(std::pair<double,double>(centralFrequency, gain));
		}
	}
	std::cout << "Subband gain file contains " << subbandGainCorrection.size() << " entries, used: ";
	for(size_t sb=0; sb!=_subbandCount; ++sb)
	{
		double channelFreq = _channelFrequenciesHz[sb * _mwaConfig.Header().nChannels / _subbandCount];
		std::map<double, double>::const_iterator i = subbandGainCorrection.lower_bound(channelFreq/1000000.0-(63.0*0.01));
		if(sb != 0) std::cout << ',';
		if(i == subbandGainCorrection.end())
		{
			std::cout << '1';
			_subbandGainCorrection.push_back(1.0);
		}
		else
		{
			std::cout << i->second;
			_subbandGainCorrection.push_back(1.0/i->second);
		}
	}
	std::cout << '\n';
}

void Cotter::flagBadCorrelatorSamples(FlagMask &flagMask) const
{
	// Flag MWA side and centre channels
	size_t scanCount = _curChunkEnd - _curChunkStart;
	for(size_t sb=0; sb!=_subbandCount; ++sb)
	{
		bool *sbStart = flagMask.Buffer() + (sb*flagMask.Height()/_subbandCount)*flagMask.HorizontalStride();
		bool *channelPtr = sbStart;
		bool *endPtr = sbStart + flagMask.HorizontalStride();
		
		// Flag first channel of sb
		while(channelPtr != endPtr) { *channelPtr=true; ++channelPtr; }
		
		// Flag centre channel of sb
		size_t halfBand = flagMask.Height()/(_subbandCount*2);
		channelPtr = sbStart + halfBand*flagMask.HorizontalStride();
		endPtr = sbStart + halfBand*flagMask.HorizontalStride() + scanCount;
		while(channelPtr != endPtr) { *channelPtr=true; ++channelPtr; }
		
		// Flag last channel of sb
		channelPtr = sbStart + (flagMask.Height()/_subbandCount-1)*flagMask.HorizontalStride();
		endPtr = sbStart + (flagMask.Height()/_subbandCount-1)*flagMask.HorizontalStride() + scanCount;
		while(channelPtr != endPtr) { *channelPtr=true; ++channelPtr; }
	}
	
	// Drop first samples
	if(_quackSampleCount >= _curChunkStart)
	{
		for(size_t ch=0; ch!=flagMask.Height(); ++ch)
		{
			bool *channelPtr = flagMask.Buffer() + ch*flagMask.HorizontalStride();
			const size_t count = _quackSampleCount - _curChunkStart;
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

void Cotter::initializeWeights(float *outputWeights)
{
	// Weights are normalized so that default res of 10 kHz, 1s has weight of "1" per sample
	// Note that this only holds for numbers in the WEIGHTS_SPECTRUM column; WEIGHTS will hold the sum.
	double weightFactor = _mwaConfig.Header().integrationTime * (100.0*_mwaConfig.Header().bandwidthMHz/_mwaConfig.Header().nChannels);
	for(size_t sb=0; sb!=_subbandCount; ++sb)
	{
		size_t channelsPerSubband = _mwaConfig.Header().nChannels / _subbandCount;
		for(size_t ch=0; ch!=channelsPerSubband; ++ch)
		{
			for(size_t p=0; p!=4; ++p)
				outputWeights[(ch+channelsPerSubband*sb)*4 + p] = weightFactor / (_subbandCorrectionFactors[p][ch]);
		}
	}
}

void Cotter::reorderSubbands(ImageSet& imageSet) const
{
	float *temp = new float[imageSet.HorizontalStride() * imageSet.Height()];
	const size_t channelsPerSubband = imageSet.Height()/_subbandCount;
	const size_t valuesPerSubband = imageSet.HorizontalStride()*channelsPerSubband;
	for(size_t i=0;i!=8;++i)
	{
		memcpy(temp, imageSet.ImageBuffer(i), imageSet.HorizontalStride()*imageSet.Height()*sizeof(float));
		
		float *tempBfr = temp;
		for(size_t sb=0;sb!=_subbandCount;++sb)
		{
			float *destBfr = imageSet.ImageBuffer(i) + valuesPerSubband * _subbandOrder[sb];
			memcpy(destBfr, tempBfr, valuesPerSubband*sizeof(float));
				
			tempBfr += valuesPerSubband;
		}
	}
	delete[] temp;
}

void Cotter::initializeSbOrder(size_t centerSbNumber)
{
	if(_subbandCount != 24)
		throw std::runtime_error("Not 24 subbands: I only know mappings for 24 subbands.");
	if(centerSbNumber<=12 || centerSbNumber > 243)
		throw std::runtime_error("Center channel must be between 13 and 243");

	_subbandOrder.resize(24);
	size_t firstSb = centerSbNumber-12;
	size_t nbank1 = 0, nbank2 = 0;
	for(size_t i=firstSb; i!=firstSb+24; ++i)
	{
		if(i<=128)
			++nbank1;
		else
			++nbank2;
	}
	for(size_t i=0; i!=nbank1; ++i)
		_subbandOrder[i]=i;
	for(size_t i=0; i!=nbank2; ++i)
		_subbandOrder[i+nbank1] = 23-i;
	
	std::cout << "Subband order for centre subband " << centerSbNumber << ": " << _subbandOrder[0];
	for(size_t i=1; i!=24; ++i)
		std::cout << ',' << _subbandOrder[i];
	std::cout << '\n';
}

void Cotter::writeAlignmentScans()
{
	if(!_writer->IsTimeAligned(0, 0))
	{
		const size_t nChannels = _mwaConfig.Header().nChannels;
		const size_t antennaCount = _mwaConfig.NAntennae();
		std::cout << "Nr of timesteps did not match averaging size, last averaged sample will be downweighted" << std::flush;
		size_t timeIndex = _mwaConfig.Header().nScans;
		
		_outputFlags = new bool[nChannels*4];
		posix_memalign((void**) &_outputData, 16, nChannels*4*sizeof(std::complex<float>));
		posix_memalign((void**) &_outputWeights, 16, nChannels*4*sizeof(float));
		for(size_t ch=0; ch!=nChannels*4; ++ch)
		{
			_outputData[ch] = std::complex<float>(0.0, 0.0);
			_outputFlags[ch] = true;
			_outputWeights[ch] = 0.0;
		}
		while(!_writer->IsTimeAligned(0, 0))
		{
			_writer->AddRows(antennaCount*(antennaCount+1)/2);
			const double dateMJD = _mwaConfig.Header().dateFirstScanMJD + timeIndex * _mwaConfig.Header().integrationTime/86400.0;
			for(size_t antenna1=0;antenna1!=antennaCount;++antenna1)
			{
				for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
				{
					_writer->WriteRow(dateMJD*86400.0, dateMJD*86400.0, antenna1, antenna2, 0.0, 0.0, 0.0, _mwaConfig.Header().integrationTime, _outputData, _outputFlags, _outputWeights);
				}
			}
			++timeIndex;
			std::cout << '.' << std::flush;
		}
		free(_outputData);
		free(_outputWeights);
		delete[] _outputFlags;
		std::cout << '\n';
	}
}

void Cotter::writeMWAFields(const char *outputFilename, size_t flagWindowSize)
{
	MWAMS mwaMs(outputFilename);
	mwaMs.AddMWAFields();
	
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
	mwaMs.UpdateMWAObservationInfo(obsInfo);
	
	mwaMs.UpdateSpectralWindowInfo(_mwaConfig.CentreSubbandNumber());
	
	mwaMs.WriteMWATilePointingInfo(_mwaConfig.Header().GetStartDateMJD()*86400.0,
		_mwaConfig.Header().GetDateLastScanMJD()*86400.0, _mwaConfig.HeaderExt().delays);
	
	for(int i=0; i!=24; ++i)
		mwaMs.WriteMWASubbandInfo(i, sqrt(1.0/_subbandGainCorrection[i]), false);
	
	mwaMs.WriteMWAKeywords(_mwaConfig.HeaderExt().fibreFactor, 0);
}
