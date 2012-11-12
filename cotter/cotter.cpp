#include "cotter.h"

#include "baselinebuffer.h"
#include "geometry.h"
#include "mswriter.h"

#include <aoflagger.h>

#include <boost/thread/thread.hpp>

#include <fstream>
#include <iostream>
#include <map>
#include <cmath>
#include <complex>

#define SPEED_OF_LIGHT 299792458.0        // speed of light in m/s

using namespace aoflagger;

Cotter::Cotter() :
	_writer(0),
	_reader(0),
	_flagger(0),
	_strategy(0),
	_threadCount(1),
	_subbandCount(24),
	_quackSampleCount(4),
	_rfiDetection(true),
	_collectStatistics(true),
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
	
	_mwaConfig.ReadHeader("header.txt", lockPointing);
	_mwaConfig.ReadInputConfig("instr_config.txt");
	_mwaConfig.ReadAntennaPositions("antenna_locations.txt");
	_mwaConfig.CheckSetup();
	
	_channelFrequenciesHz.resize(_mwaConfig.Header().nChannels);
	for(size_t ch=0; ch!=_mwaConfig.Header().nChannels; ++ch)
		_channelFrequenciesHz[ch] = _mwaConfig.ChannelFrequencyHz(ch);
	
	readSubbandPassbandFile();
	readSubbandGainsFile();
	initializeSbOrder(_mwaConfig.CentreSubbandNumber());
	
	_flagger = new AOFlagger();
	
	// Initialize buffers
	const size_t antennaCount = _mwaConfig.NAntennae();
	for(size_t antenna1=0;antenna1!=antennaCount;++antenna1)
	{
		for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
		{
			/// TODO calculate available memory and use that for the buffers
			ImageSet *imageSet = new ImageSet(_flagger->MakeImageSet(_mwaConfig.Header().nScans, _mwaConfig.Header().nChannels, 8));
			_imageSetBuffers.insert(std::pair<std::pair<size_t,size_t>, ImageSet*>(
				std::pair<size_t,size_t>(antenna1, antenna2), imageSet
			));
		}
	}
	
	if(freqAvgFactor == 1 && timeAvgFactor == 1)
		_writer = new MSWriter(outputFilename);
	else
		_writer = new AveragingMSWriter(outputFilename, timeAvgFactor, freqAvgFactor, *this);
	writeAntennae();
	writeSPW();
	writeField();
	_writer->WritePolarizationForLinearPols(false);
	_writer->WriteObservation("MWA", _mwaConfig.Header().dateFirstScanMJD*86400.0, _mwaConfig.Header().GetDateLastScanMJD()*86400.0, "Unknown", "MWA", "Unknown", 0, false);

	std::vector<std::vector<std::string> >::const_iterator currentFileSetPtr = _fileSets.begin();
	
	size_t bufferPos = 0;
	bool continueWithNextFile;
	do {
		_reader = new GPUFileReader();
		
		for(std::vector<std::string>::const_iterator i=currentFileSetPtr->begin(); i!=currentFileSetPtr->end(); ++i)
			_reader->AddFile(i->c_str());
		
		_reader->Initialize();
		if(_reader->AntennaCount() != antennaCount)
			throw std::runtime_error("nAntennae in GPU files do not match nAntennae in config");
		if(_reader->ChannelCount() != _mwaConfig.Header().nChannels)
			throw std::runtime_error("nChannels in GPU files do not match nChannels in config");
		
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
		
		bool moreAvailableInCurrentFile = _reader->Read(bufferPos);
		
		if(!moreAvailableInCurrentFile && bufferPos < _mwaConfig.Header().nScans)
		{
			// Go to the next set of GPU files and add them to the buffer
			++currentFileSetPtr;
			continueWithNextFile = (currentFileSetPtr!=_fileSets.end());
			if(continueWithNextFile)
				delete _reader;
		} else {
			continueWithNextFile = false;
		}
	} while(continueWithNextFile);
	
	_scanTimes.resize(_mwaConfig.Header().nScans);
	for(size_t t=0; t!=_mwaConfig.Header().nScans; ++t)
	{
		double dateMJD = _mwaConfig.Header().dateFirstScanMJD * 86400.0 + t * _mwaConfig.Header().integrationTime;
		_scanTimes[t] = dateMJD;
	}
	
	_strategy = new Strategy(_flagger->MakeStrategy(MWA_TELESCOPE));
	_fullysetMask = new FlagMask(_flagger->MakeFlagMask(_mwaConfig.Header().nScans, _reader->ChannelCount(), true));
	_correlatorMask = new FlagMask(_flagger->MakeFlagMask(_mwaConfig.Header().nScans, _reader->ChannelCount(), false));
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
	const size_t nChannels = _mwaConfig.Header().nChannels;
	double antU[antennaCount], antV[antennaCount], antW[antennaCount];
	for(size_t t=0; t!=_mwaConfig.Header().nScans; ++t)
	{
		double dateMJD = _mwaConfig.Header().dateFirstScanMJD + t * _mwaConfig.Header().integrationTime/86400.0;
		
		Geometry::UVWTimestepInfo uvwInfo;
		Geometry::PrepareTimestepUVW(uvwInfo, dateMJD, _mwaConfig.ArrayLongitudeRad(), _mwaConfig.ArrayLattitudeRad(), _mwaConfig.Header().raHrs, _mwaConfig.Header().decDegs);
		
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
		std::complex<float> outputData[nChannels*4];
		bool outputFlags[nChannels*4];
		float outputWeights[nChannels*4];
		initializeWeights(outputWeights);
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
					
				for(size_t p=0; p!=4; ++p)
				{
					const float
						*realPtr = imageSet.ImageBuffer(p*2)+t,
						*imagPtr = imageSet.ImageBuffer(p*2+1)+t;
					const bool *flagPtr = flagMask.Buffer()+t;
					std::complex<float> *outDataPtr = &outputData[p];
					bool *outputFlagPtr = &outputFlags[p];
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
				
				_writer->WriteRow(dateMJD*86400.0, dateMJD*86400.0, antenna1, antenna2, u, v, w, _mwaConfig.Header().integrationTime, t, outputData, outputFlags, outputWeights);
			}
		}
		std::cout << '.' << std::flush;
	}
	std::cout << '\n';
	
	delete _writer;
	_writer = 0;
	delete _correlatorMask;
	_correlatorMask = 0;
	delete _fullysetMask;
	_fullysetMask = 0;
	delete _reader;
	_reader = 0;
	
	if(_collectStatistics)
		_flagger->WriteStatistics(*_statistics, outputFilename);
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
		_flagger->MakeQualityStatistics(&_scanTimes[0], _mwaConfig.Header().nScans, &_channelFrequenciesHz[0], _channelFrequenciesHz.size(), 4);
	
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
			flagMask = new FlagMask(_flagger->MakeFlagMask(_mwaConfig.Header().nScans, _reader->ChannelCount(), false));
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
	str << "MWA_BAND_" << (round(_mwaConfig.Header().centralFrequency*10.0)/10.0);
	for(size_t ch=0;ch!=_mwaConfig.Header().nChannels;++ch)
	{
		MSWriter::ChannelInfo &channel = channels[ch];
		channel.chanFreq = _channelFrequenciesHz[ch];
		channel.chanWidth = _mwaConfig.Header().bandwidth / _mwaConfig.Header().nChannels;
		channel.effectiveBW = channel.chanWidth;
		channel.resolution = channel.chanWidth;
	}
	_writer->WriteBandInfo(str.str(),
		channels,
		_mwaConfig.Header().centralFrequency*1000000.0,
		_mwaConfig.Header().bandwidth*1000000.0,
		false
	);
}

void Cotter::writeField()
{
	const MWAHeader &header = _mwaConfig.Header();
	MSWriter::FieldInfo field;
	field.name = header.fieldName;
	field.code = std::string();
	field.time = Geometry::GetMJD(
		header.year, header.month, header.day, 
		header.refHour, header.refMinute, header.refSecond);
	field.numPoly = 0;
	field.delayDirRA = header.raHrs * (M_PI/12.0);
	field.delayDirDec = header.decDegs * (M_PI/180.0);
	field.phaseDirRA = field.delayDirRA;
	field.phaseDirDec = field.phaseDirDec;
	field.referenceDirRA = field.referenceDirRA;
	field.referenceDirDec = field.referenceDirDec;
	field.sourceId = 0;
	field.flagRow = false;
	_writer->WriteField(field);
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
		size_t index;
		double centralFrequency, gain;
		gainsFile >> index >> centralFrequency >> gain;
		if(gainsFile.fail()) break;
		subbandGainCorrection.insert(std::pair<double,double>(centralFrequency, gain));
	}
	std::cout << "Subband gain file contains " << subbandGainCorrection.size() << " entries, used: ";
	for(size_t sb=0; sb!=_subbandCount; ++sb)
	{
		double channelFreq = _channelFrequenciesHz[sb * _mwaConfig.Header().nChannels / _subbandCount];
		std::map<double, double>::const_iterator i = subbandGainCorrection.lower_bound(channelFreq/1000000.0);
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
	for(size_t sb=0; sb!=_subbandCount; ++sb)
	{
		bool *sbStart = flagMask.Buffer() + (sb*flagMask.Height()/_subbandCount)*_mwaConfig.Header().nScans;
		bool *channelPtr = sbStart;
		bool *endPtr = sbStart + flagMask.HorizontalStride();
		
		// Flag first channel of sb
		while(channelPtr != endPtr) { *channelPtr=true; ++channelPtr; }
		
		// Flag centre channel of sb
		size_t halfBand = flagMask.Height()/(_subbandCount*2);
		channelPtr = sbStart + halfBand*_mwaConfig.Header().nScans;
		endPtr = sbStart + (halfBand + 1)*_mwaConfig.Header().nScans;
		while(channelPtr != endPtr) { *channelPtr=true; ++channelPtr; }
		
		// Flag last channel of sb
		channelPtr = sbStart + (flagMask.Height()/_subbandCount-1)*_mwaConfig.Header().nScans;
		endPtr = sbStart + (flagMask.Height()/_subbandCount)*_mwaConfig.Header().nScans;
		while(channelPtr != endPtr) { *channelPtr=true; ++channelPtr; }
	}
	
	// Drop first samples
	for(size_t ch=0; ch!=flagMask.Height(); ++ch)
	{
		bool *channelPtr = flagMask.Buffer() + ch*flagMask.HorizontalStride();
		for(size_t x=0; x!=_quackSampleCount;++x)
		{
			*channelPtr = true;
			++channelPtr;
		}
	}
}

void Cotter::initializeWeights(float *outputWeights)
{
	// Weights are normalized so that default res of 10 kHz, 1s has weight of "1" per sample
	size_t weightFactor = _mwaConfig.Header().integrationTime * (100.0*_mwaConfig.Header().bandwidth/_mwaConfig.Header().nChannels);
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
