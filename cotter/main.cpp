#include <aoflagger.h>

#include <iostream>
#include <map>
#include <cmath>
#include <complex>

#include "gpufilereader.h"
#include "baselinebuffer.h"
#include "mwaconfig.h"
#include "geometry.h"
#include "mswriter.h"

#define SPEED_OF_LIGHT 299792458.0        // speed of light in m/s

using namespace aoflagger;

void correctCableLength(ImageSet& imageSet, size_t polarization, double cableDelay, const double *channelFrequencies);
void writeAntennae(const MWAConfig &mwaConfig, MSWriter &msWriter);
void writeSPW(const MWAConfig &mwaConfig, MSWriter &msWriter, const double *channelFrequencies);
void writeField(const MWAConfig &mwaConfig, MSWriter &msWriter);

int main(int argc, char **argv)
{
	std::cout << "Running Cotter MWA preprocessing pipeline, converting to table.\n";
	
	std::vector<std::vector<std::string> > fileSets;
	int argi = 1;
	while(argi!=argc)
	{
		if(strcmp(argv[argi], "-i") == 0)
		{
			++argi;
			fileSets.push_back(std::vector<std::string>());
			std::vector<std::string> &newSet = *fileSets.rbegin();
			while(argi!=argc && argv[argi][0]!='-')
			{
				newSet.push_back(argv[argi]);
				++argi;
			}
		}
		else {
			++argi;
		}
	}
	
	if(argc == 1 || fileSets.empty())
	{
		std::cout << "usage: cotter -i <gpufile1> [<gpufile2> ..] [-i <gpufile1> <gpufile2> ..\n";
		return -1;
	}
	
	bool lockPointing = false;
	
	MWAConfig mwaConfig;
	
	mwaConfig.ReadHeader("header.txt", lockPointing);
	mwaConfig.ReadInputConfig("instr_config.txt");
	mwaConfig.ReadAntennaPositions("antenna_locations.txt");
	mwaConfig.CheckSetup();
	
	GPUFileReader reader;
	std::vector<std::vector<std::string> >::const_iterator currentFileSetPtr = fileSets.begin();
	for(std::vector<std::string>::const_iterator i=currentFileSetPtr->begin(); i!=currentFileSetPtr->end(); ++i)
	{
		reader.AddFile(i->c_str());
	}
	reader.Initialize();
	
	AOFlagger flagger;
	
	size_t antennaCount = reader.AntennaCount();
	double channelFrequencies[mwaConfig.Header().nChannels];
	for(size_t ch=0; ch!=mwaConfig.Header().nChannels; ++ch)
		channelFrequencies[ch] = mwaConfig.ChannelFrequency(ch);
	
	/// TODO initialize casa measurement set
	MSWriter msWriter("flagged.ms");
	writeAntennae(mwaConfig, msWriter);
	writeSPW(mwaConfig, msWriter, channelFrequencies);
	writeField(mwaConfig, msWriter);
	msWriter.WritePolarizationForLinearPols(false);
	
	/// TODO calculate available memory and use that for the buffers
	
	// Initialize buffers
	std::map<std::pair<size_t, size_t>, ImageSet*> buffers;
	for(size_t antenna1=0;antenna1!=antennaCount;++antenna1)
	{
		for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
		{
			ImageSet *imageSet = new ImageSet(flagger.MakeImageSet(mwaConfig.Header().nScans, reader.ChannelCount(), 8));
			buffers.insert(std::pair<std::pair<size_t,size_t>, ImageSet*>(
				std::pair<size_t,size_t>(antenna1, antenna2), imageSet
			));
			
			BaselineBuffer buffer;
			buffer.realXX = imageSet->ImageBuffer(0);
			buffer.imagXX = imageSet->ImageBuffer(1);
			buffer.realXY = imageSet->ImageBuffer(2);
			buffer.imagXY = imageSet->ImageBuffer(3);
			buffer.realYX = imageSet->ImageBuffer(4);
			buffer.imagYX = imageSet->ImageBuffer(5);
			buffer.realYY = imageSet->ImageBuffer(6);
			buffer.imagYY = imageSet->ImageBuffer(7);
			buffer.nElementsPerRow = imageSet->HorizontalStride();
			reader.SetDestBaselineBuffer(antenna1, antenna2, buffer);
		}
	}
	
	size_t bufferPos = 0;
	bool moreAvailable = reader.Read(bufferPos);
	
	std::map<std::pair<size_t, size_t>, FlagMask*> flagBuffers;
	Strategy strategy = flagger.MakeStrategy(MWA_TELESCOPE);
	for(size_t antenna1=0;antenna1!=antennaCount;++antenna1)
	{
		for(size_t antenna2=antenna1; antenna2!=antennaCount; ++antenna2)
		{
			ImageSet *imageSet = buffers.find(std::pair<size_t,size_t>(antenna1, antenna2))->second;
			const MWAInput
				&input1X = mwaConfig.AntennaXInput(antenna1),
				&input1Y = mwaConfig.AntennaXInput(antenna1),
				&input2X = mwaConfig.AntennaXInput(antenna2),
				&input2Y = mwaConfig.AntennaYInput(antenna2);
			
			// Correct cable delay
			std::cout << "Correcting cable length for " << antenna1 << " x " << antenna2 << '\n';
			correctCableLength(*imageSet, 0, input2X.cableLenDelta - input1X.cableLenDelta, channelFrequencies);
			correctCableLength(*imageSet, 1, input2Y.cableLenDelta - input1X.cableLenDelta, channelFrequencies);
			correctCableLength(*imageSet, 2, input2X.cableLenDelta - input1Y.cableLenDelta, channelFrequencies);
			correctCableLength(*imageSet, 3, input2Y.cableLenDelta - input1Y.cableLenDelta, channelFrequencies);
			
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
				flagMask = new FlagMask(flagger.MakeFlagMask(mwaConfig.Header().nScans, reader.ChannelCount(), true));
			
			flagBuffers.insert(std::pair<std::pair<size_t, size_t>, FlagMask*>(
					std::pair<size_t,size_t>(antenna1, antenna2), flagMask));
			
			/// TODO collect statistics
			
			/// TODO flag MWA side and centre channels
		}
	}
	
	const size_t nChannels = mwaConfig.Header().nChannels;
	double antU[antennaCount], antV[antennaCount], antW[antennaCount];
	for(size_t t=0; t!=mwaConfig.Header().nScans; ++t)
	{
		double dateMJD = mwaConfig.Header().dateFirstScanMJD + t * mwaConfig.Header().integrationTime/86400.0;
		
		Geometry::UVWTimestepInfo uvwInfo;
		Geometry::PrepareTimestepUVW(uvwInfo, dateMJD, mwaConfig.ArrayLongitudeRad(), mwaConfig.ArrayLattitudeRad(), mwaConfig.Header().raHrs, mwaConfig.Header().decDegs);
		
		std::cout << uvwInfo.lmst << '\n';
 
		for(size_t antenna=0; antenna!=antennaCount; ++antenna)
		{
			const double
				x = mwaConfig.Antenna(antenna).position[0],
				y = mwaConfig.Antenna(antenna).position[1],
				z = mwaConfig.Antenna(antenna).position[2];
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
					
				// Calculate rotation coefficients for geometric phase delay correction
				for(size_t ch=0; ch!=nChannels; ++ch)
				{
					double angle = -2.0*M_PI*w*SPEED_OF_LIGHT / channelFrequencies[ch];
					sincos(angle, &sinAngles[ch], &cosAngles[ch]);
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
						float rtmp = *realPtr, itmp = *imagPtr;
						*outDataPtr = std::complex<float>(
							cosAngles[ch] * rtmp - sinAngles[ch] * itmp,
							sinAngles[ch] * rtmp + cosAngles[ch] * itmp
						);
						*outputFlagPtr = *flagPtr;
						
						realPtr += stride;
						imagPtr += stride;
						flagPtr += flagStride;
						outDataPtr += 4;
						outputFlagPtr += 4;
					}
				}
				/// TODO average
				
				/// TODO write to casa file.
				msWriter.WriteRow(dateMJD, antenna1, antenna2, outputData, outputFlags);
			}
		}
	}
	
	
	return 0;
}

void correctCableLength(ImageSet& imageSet, size_t polarization, double cableDelay, const double *channelFrequencies)
{
	float *reals = imageSet.ImageBuffer(polarization*2);
	float *imags = imageSet.ImageBuffer(polarization*2+1);
	
	for(size_t y=0; y!=imageSet.Height(); ++y)
	{
		double angle = -2.0 * M_PI * cableDelay * SPEED_OF_LIGHT / channelFrequencies[y];
		float rotSin = sin(angle), rotCos = cos(angle);
		
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

void writeAntennae(const MWAConfig &mwaConfig, MSWriter &msWriter)
{
	std::vector<MSWriter::AntennaInfo> antennae;
	for(size_t i=0; i!=mwaConfig.NAntennae(); ++i)
	{
		const MWAAntenna &mwaAnt = mwaConfig.Antenna(i);
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
	msWriter.WriteAntennae(antennae);
}

void writeSPW(const MWAConfig &mwaConfig, MSWriter &msWriter, const double *channelFrequencies)
{
	std::vector<MSWriter::ChannelInfo> channels(mwaConfig.Header().nChannels);
	std::ostringstream str;
	str << "MWA_BAND_" << (round(mwaConfig.Header().centralFrequency*10.0)/10.0);
	for(size_t ch=0;ch!=mwaConfig.Header().nChannels;++ch)
	{
		MSWriter::ChannelInfo &channel = channels[ch];
		channel.chanFreq = channelFrequencies[ch]*1000000.0;
		channel.chanWidth = mwaConfig.Header().bandwidth*1000000.0 / mwaConfig.Header().nChannels;
		channel.effectiveBW = channel.chanWidth;
		channel.resolution = channel.chanWidth;
	}
	msWriter.WriteBandInfo(str.str(),
		channels,
		mwaConfig.Header().centralFrequency*1000000.0,
		mwaConfig.Header().bandwidth*1000000.0,
		false
	);
}

void writeField(const MWAConfig &mwaConfig, MSWriter &msWriter)
{
	const MWAHeader &header = mwaConfig.Header();
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
	msWriter.WriteField(field);
}

