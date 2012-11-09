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
	_statistics(0)
{
}

Cotter::~Cotter()
{
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
	
	readSubbandPassbandFile();
	
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
	
	_channelFrequenciesHz.resize(_mwaConfig.Header().nChannels);
	for(size_t ch=0; ch!=_mwaConfig.Header().nChannels; ++ch)
		_channelFrequenciesHz[ch] = _mwaConfig.ChannelFrequencyHz(ch);
	
	if(freqAvgFactor == 1 && timeAvgFactor == 1)
		_writer = new MSWriter(outputFilename);
	else
		_writer = new AveragingMSWriter(outputFilename, timeAvgFactor, freqAvgFactor, *this);
	writeAntennae();
	writeSPW();
	writeField();
	_writer->WritePolarizationForLinearPols(false);

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
			delete _reader;
			++currentFileSetPtr;
			continueWithNextFile = (currentFileSetPtr!=_fileSets.end());
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
	boost::thread_group threadGroup;
	for(size_t i=0; i!=_threadCount; ++i)
		threadGroup.create_thread(boost::bind(&Cotter::baselineProcessThreadFunc, this));
	threadGroup.join_all();
	
	if(_mwaConfig.Header().geomCorrection)
		std::cout << "Will apply geometric delay correction.\n";
	
	const size_t nChannels = _mwaConfig.Header().nChannels;
	double antU[antennaCount], antV[antennaCount], antW[antennaCount];
	for(size_t t=0; t!=_mwaConfig.Header().nScans; ++t)
	{
		double dateMJD = _mwaConfig.Header().dateFirstScanMJD + t * _mwaConfig.Header().integrationTime/86400.0;
		
		Geometry::UVWTimestepInfo uvwInfo;
		Geometry::PrepareTimestepUVW(uvwInfo, dateMJD, _mwaConfig.ArrayLongitudeRad(), _mwaConfig.ArrayLattitudeRad(), _mwaConfig.Header().raHrs, _mwaConfig.Header().decDegs);
		
		std::cout << uvwInfo.lmst << '\n';
 
		for(size_t antenna=0; antenna!=antennaCount; ++antenna)
		{
			const double
				x = _mwaConfig.Antenna(antenna).position[0],
				y = _mwaConfig.Antenna(antenna).position[1],
				z = _mwaConfig.Antenna(antenna).position[2];
			Geometry::CalcUVW(uvwInfo, x, y, z, antU[antenna],antV[antenna], antW[antenna]);
			if(antenna==5)
			std::cout << "Pos: " << x << ',' << y << ',' << z <<
				", Uvw: " << antU[antenna] << ',' << antV[antenna] << ',' << antW[antenna] << '\n';
		}
		
		double cosAngles[nChannels], sinAngles[nChannels];
		std::complex<float> outputData[nChannels*4];
		bool outputFlags[nChannels*4];
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
							if((ch == nChannels/2 || ch ==0) && antenna1==0 && antenna2==1)
								std::cout << "Rotation=" << (angle/(2.0*M_PI)) << " for freq " << _channelFrequenciesHz[ch] << ",w=" << w << ",lambda=" << (SPEED_OF_LIGHT/_channelFrequenciesHz[ch]) << '\n';
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
				/// TODO average
				
				_writer->WriteRow(dateMJD*86400.0, antenna1, antenna2, u, v, w, outputData, outputFlags);
			}
		}
	}
	
	delete _writer;
	_writer = 0;
	
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
		std::cout << "Conjugating " << antenna1 << 'x' << antenna2 << '\n';
		correctConjugated(*imageSet, 1);
	}
	if(_reader->IsConjugated(antenna1, antenna2, 0, 1)) {
		std::cout << "Conjugating " << antenna1 << 'x' << antenna2 << '\n';
		correctConjugated(*imageSet, 3);
	}
	if(_reader->IsConjugated(antenna1, antenna2, 1, 0)) {
		std::cout << "Conjugating " << antenna1 << 'x' << antenna2 << '\n';
		correctConjugated(*imageSet, 5);
	}
	if(_reader->IsConjugated(antenna1, antenna2, 1, 1)) {
		std::cout << "Conjugating " << antenna1 << 'x' << antenna2 << '\n';
		correctConjugated(*imageSet, 7);
	}
	
	// Correct cable delay
	std::cout << "Correcting cable length for " << antenna1 << " x " << antenna2 << '\n';
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
				const float correctionFactor = _subbandCorrectionFactors[i/2][ch];
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
	if(skipFlagging)
		std::cout << "Skipping " << antenna1 << " x " << antenna2 << '\n';
	else
		std::cout << "Flagging " << antenna1 << " x " << antenna2 << '\n';

	FlagMask *flagMask, *correlatorMask;
	if(skipFlagging)
	{
		flagMask = new FlagMask(_flagger->MakeFlagMask(_mwaConfig.Header().nScans, _reader->ChannelCount(), true));
		correlatorMask = new FlagMask(*flagMask);
	}
	else 
	{
		flagMask = new FlagMask(_flagger->Run(*_strategy, *imageSet));
		correlatorMask = new FlagMask(_flagger->MakeFlagMask(_mwaConfig.Header().nScans, _reader->ChannelCount(), false));
		flagBadCorrelatorSamples(*flagMask);
		flagBadCorrelatorSamples(*correlatorMask);
	}
	
	_flagBuffers.find(std::pair<size_t, size_t>(antenna1, antenna2))->second = flagMask;
	
	// Collect statistics
	_flagger->CollectStatistics(statistics, *imageSet, *flagMask, *correlatorMask, antenna1, antenna2);
}	

void Cotter::correctConjugated(ImageSet& imageSet, size_t imgImageIndex)
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

void Cotter::correctCableLength(ImageSet& imageSet, size_t polarization, double cableDelay)
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
	std::vector<MSWriter::AntennaInfo> antennae;
	for(size_t i=0; i!=_mwaConfig.NAntennae(); ++i)
	{
		const MWAAntenna &mwaAnt = _mwaConfig.Antenna(i);
		MSWriter::AntennaInfo antennaInfo;
		antennaInfo.name = mwaAnt.name;
		antennaInfo.station = "MWA";
		antennaInfo.type = "GROUND-BASED";
		antennaInfo.mount = "FIXED";
		antennaInfo.x = mwaAnt.position[0];
		antennaInfo.y = mwaAnt.position[1];
		antennaInfo.z = mwaAnt.position[2];
		antennaInfo.diameter = 4; /** TODO can probably give more exact size! */
		antennaInfo.flag = false;
		
		antennae.push_back(antennaInfo);
	}
	_writer->WriteAntennae(antennae);
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
	std::cout << "Read subband passband, using " << channelsPerSubband << " gains to correct for " << subBandCount << " channels/subband\n";
	if(subBandCount != _subbandCount)
		throw std::runtime_error("Unexpected number of channels in subband passband correction file");
}

void Cotter::flagBadCorrelatorSamples(FlagMask &flagMask)
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