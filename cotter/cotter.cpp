#include "cotter.h"

#include "baselinebuffer.h"
#include "geometry.h"
#include "mswriter.h"

#include <aoflagger.h>

#include <iostream>
#include <map>
#include <cmath>
#include <complex>

#define SPEED_OF_LIGHT 299792458.0        // speed of light in m/s

using namespace aoflagger;

void Cotter::Run()
{
	bool lockPointing = false;
	
	_mwaConfig.ReadHeader("header.txt", lockPointing);
	_mwaConfig.ReadInputConfig("instr_config.txt");
	_mwaConfig.ReadAntennaPositions("antenna_locations.txt");
	_mwaConfig.CheckSetup();
	
	AOFlagger flagger;
	
	// Initialize buffers
	const size_t antennaCount = _mwaConfig.NAntennae();
	std::map<std::pair<size_t, size_t>, ImageSet*> buffers;
	for(size_t antenna1=0;antenna1!=antennaCount;++antenna1)
	{
		for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
		{
			/// TODO calculate available memory and use that for the buffers
			ImageSet *imageSet = new ImageSet(flagger.MakeImageSet(_mwaConfig.Header().nScans, _mwaConfig.Header().nChannels, 8));
			buffers.insert(std::pair<std::pair<size_t,size_t>, ImageSet*>(
				std::pair<size_t,size_t>(antenna1, antenna2), imageSet
			));
		}
	}
	
	double channelFrequenciesHz[_mwaConfig.Header().nChannels];
	for(size_t ch=0; ch!=_mwaConfig.Header().nChannels; ++ch)
		channelFrequenciesHz[ch] = _mwaConfig.ChannelFrequencyHz(ch);
		
	_msWriter = new MSWriter("flagged.ms");
	writeAntennae();
	writeSPW(channelFrequenciesHz);
	writeField();
	_msWriter->WritePolarizationForLinearPols(false);

	std::vector<std::vector<std::string> >::const_iterator currentFileSetPtr = _fileSets.begin();
	
	GPUFileReader *reader;
	size_t bufferPos = 0;
	bool continueWithNextFile;
	do {
		reader = new GPUFileReader();
		
		for(std::vector<std::string>::const_iterator i=currentFileSetPtr->begin(); i!=currentFileSetPtr->end(); ++i)
			reader->AddFile(i->c_str());
		
		reader->Initialize();
		if(reader->AntennaCount() != antennaCount)
			throw std::runtime_error("nAntennae in GPU files do not match nAntennae in config");
		if(reader->ChannelCount() != _mwaConfig.Header().nChannels)
			throw std::runtime_error("nChannels in GPU files do not match nChannels in config");
		
		for(size_t i=0; i!=_mwaConfig.Header().nInputs; ++i)
			reader->SetCorrInputToOutput(i, _mwaConfig.Input(i).antennaIndex, _mwaConfig.Input(i).polarizationIndex);
		
		// Initialize buffers of reader
		for(size_t antenna1=0;antenna1!=antennaCount;++antenna1)
		{
			for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
			{
				ImageSet &imageSet = *buffers.find(std::pair<size_t, size_t>(antenna1, antenna2))->second;
				BaselineBuffer buffer;
				for(size_t p=0; p!=4; ++p)
				{
					buffer.real[p] = imageSet.ImageBuffer(p*2);
					buffer.imag[p] = imageSet.ImageBuffer(p*2+1);
				}
				buffer.nElementsPerRow = imageSet.HorizontalStride();
				reader->SetDestBaselineBuffer(antenna1, antenna2, buffer);
			}
		}
		
		bool moreAvailableInCurrentFile = reader->Read(bufferPos);
		
		if(!moreAvailableInCurrentFile && bufferPos < _mwaConfig.Header().nScans)
		{
			// Go to the next set of GPU files and add them to the buffer
			delete reader;
			++currentFileSetPtr;
			continueWithNextFile = (currentFileSetPtr!=_fileSets.end());
		} else {
			continueWithNextFile = false;
		}
	} while(continueWithNextFile);
	
	std::map<std::pair<size_t, size_t>, FlagMask*> flagBuffers;
	Strategy strategy = flagger.MakeStrategy(MWA_TELESCOPE);
	for(size_t antenna1=0;antenna1!=antennaCount;++antenna1)
	{
		for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
		{
			ImageSet *imageSet = buffers.find(std::pair<size_t,size_t>(antenna1, antenna2))->second;
			const MWAInput
				&input1X = _mwaConfig.AntennaXInput(antenna1),
				&input1Y = _mwaConfig.AntennaXInput(antenna1),
				&input2X = _mwaConfig.AntennaXInput(antenna2),
				&input2Y = _mwaConfig.AntennaYInput(antenna2);
				
			// Correct conjugated baselines
			if(reader->IsConjugated(antenna1, antenna2, 0, 0)) {
				std::cout << "Conjugating " << antenna1 << 'x' << antenna2 << '\n';
				correctConjugated(*imageSet, 1);
			}
			if(reader->IsConjugated(antenna1, antenna2, 0, 1)) {
				std::cout << "Conjugating " << antenna1 << 'x' << antenna2 << '\n';
				correctConjugated(*imageSet, 3);
			}
			if(reader->IsConjugated(antenna1, antenna2, 1, 0)) {
				std::cout << "Conjugating " << antenna1 << 'x' << antenna2 << '\n';
				correctConjugated(*imageSet, 5);
			}
			if(reader->IsConjugated(antenna1, antenna2, 1, 1)) {
				std::cout << "Conjugating " << antenna1 << 'x' << antenna2 << '\n';
				correctConjugated(*imageSet, 7);
			}
			
			// Correct cable delay
			std::cout << "Correcting cable length for " << antenna1 << " x " << antenna2 << '\n';
			correctCableLength(*imageSet, 0, input2X.cableLenDelta - input1X.cableLenDelta, channelFrequenciesHz);
			correctCableLength(*imageSet, 1, input2Y.cableLenDelta - input1X.cableLenDelta, channelFrequenciesHz);
			correctCableLength(*imageSet, 2, input2X.cableLenDelta - input1Y.cableLenDelta, channelFrequenciesHz);
			correctCableLength(*imageSet, 3, input2Y.cableLenDelta - input1Y.cableLenDelta, channelFrequenciesHz);
			
			/// TODO correct passband
			
			// Perform RFI detection, if baseline is not flagged.
			bool is1Flagged = input1X.isFlagged || input1Y.isFlagged;
			bool is2Flagged = input2X.isFlagged || input2Y.isFlagged;
			if(is1Flagged || is2Flagged)
				std::cout << "Skipping " << antenna1 << " x " << antenna2 << '\n';
			else
				std::cout << "Flagging " << antenna1 << " x " << antenna2 << '\n';

			FlagMask *flagMask;
			if(antenna1 == 29) //testing
				flagMask = new FlagMask(flagger.Run(strategy, *imageSet));
			else
				flagMask = new FlagMask(flagger.MakeFlagMask(_mwaConfig.Header().nScans, reader->ChannelCount(), false));
			
			flagBuffers.insert(std::pair<std::pair<size_t, size_t>, FlagMask*>(
					std::pair<size_t,size_t>(antenna1, antenna2), flagMask));
			
			// Flag MWA side and centre channels
			const size_t nSubbands = 24;
			for(size_t sb=0; sb!=nSubbands; ++sb)
			{
				bool *sbStart = flagMask->Buffer() + (sb*flagMask->Height()/nSubbands)*_mwaConfig.Header().nScans;
				bool *channelPtr = sbStart;
				bool *endPtr = sbStart + flagMask->HorizontalStride();
				
				// Flag first channel of sb
				while(channelPtr != endPtr) { *channelPtr=true; ++channelPtr; }
				
				// Flag centre channel of sb
				size_t halfBand = flagMask->Height()/(nSubbands*2);
				channelPtr = sbStart + halfBand*_mwaConfig.Header().nScans;
				endPtr = sbStart + (halfBand + 1)*_mwaConfig.Header().nScans;
				while(channelPtr != endPtr) { *channelPtr=true; ++channelPtr; }
				
				// Flag last channel of sb
				channelPtr = sbStart + (flagMask->Height()/nSubbands-1)*_mwaConfig.Header().nScans;
				endPtr = sbStart + (flagMask->Height()/nSubbands)*_mwaConfig.Header().nScans;
				while(channelPtr != endPtr) { *channelPtr=true; ++channelPtr; }
			}
			
			/// TODO collect statistics
		}
	}
	
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
				const ImageSet &imageSet = *buffers.find(std::pair<size_t, size_t>(antenna1, antenna2))->second;
				const FlagMask &flagMask = *flagBuffers.find(std::pair<size_t, size_t>(antenna1, antenna2))->second;
				
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
						double angle = -2.0*M_PI*w*channelFrequenciesHz[ch] / SPEED_OF_LIGHT;
						double sinAng, cosAng;
						sincos(angle, &sinAng, &cosAng);
						sinAngles[ch] = sinAng; cosAngles[ch] = cosAng;
							if((ch == nChannels/2 || ch ==0) && antenna1==0 && antenna2==1)
								std::cout << "Rotation=" << (angle/(2.0*M_PI)) << " for freq " << channelFrequenciesHz[ch] << ",w=" << w << ",lambda=" << (SPEED_OF_LIGHT/channelFrequenciesHz[ch]) << '\n';
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
				
				_msWriter->WriteRow(dateMJD*86400.0, antenna1, antenna2, u, v, w, outputData, outputFlags);
			}
		}
	}
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

void Cotter::correctCableLength(ImageSet& imageSet, size_t polarization, double cableDelay, const double *channelFrequenciesHz)
{
	float *reals = imageSet.ImageBuffer(polarization*2);
	float *imags = imageSet.ImageBuffer(polarization*2+1);
	
	for(size_t y=0; y!=imageSet.Height(); ++y)
	{
		double angle = -2.0 * M_PI * cableDelay * channelFrequenciesHz[y] / SPEED_OF_LIGHT;
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
	_msWriter->WriteAntennae(antennae);
}

void Cotter::writeSPW(const double *channelFrequenciesHz)
{
	std::vector<MSWriter::ChannelInfo> channels(_mwaConfig.Header().nChannels);
	std::ostringstream str;
	str << "MWA_BAND_" << (round(_mwaConfig.Header().centralFrequency*10.0)/10.0);
	for(size_t ch=0;ch!=_mwaConfig.Header().nChannels;++ch)
	{
		MSWriter::ChannelInfo &channel = channels[ch];
		channel.chanFreq = channelFrequenciesHz[ch];
		channel.chanWidth = _mwaConfig.Header().bandwidth / _mwaConfig.Header().nChannels;
		channel.effectiveBW = channel.chanWidth;
		channel.resolution = channel.chanWidth;
	}
	_msWriter->WriteBandInfo(str.str(),
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
	_msWriter->WriteField(field);
}
