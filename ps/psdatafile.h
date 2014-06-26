#ifndef PS_DATA_FILE_H
#define PS_DATA_FILE_H

#include "polarizationenum.h"

#include "aocommon/uvector.h"

#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>

class PSDataFile
{
public:
	struct ChannelData
	{
		double frequency, bandwidth;
		PolarizationEnum polarization;
		double xPerpStart, xPerpStep;
		ao::uvector<double> values;
		ao::uvector<double> weights;
		
		ChannelData() :
			frequency(0.0), bandwidth(0.0),
			polarization(Polarization::StokesI),
			xPerpStart(0.0), xPerpStep(0.0)
			{ }
	};
	
	PSDataFile(const std::string& filename) : _filename(filename)
	{
		if(boost::filesystem::exists(filename))
		{
			std::cout << "Reading " << filename << "..." << std::flush;
			std::ifstream file(filename);
			readHeader(file, _header);
			if(!file.good())
				throw std::runtime_error("Read error");
			std::cout << " (" << _header.channelCount << " channels" << std::flush;
			_channels.resize(_header.channelCount);
			for(size_t i=0; i!=_header.channelCount; ++i)
			{
				readChannelData(file, _channels[i]);
				if(!file.good())
					throw std::runtime_error("Read error");
			}
			double frequencyLow, frequencyHigh;
			GetFrequencyRange(frequencyLow, frequencyHigh);
			std::cout << ", bandwidth of " << (1e-6*(frequencyHigh-frequencyLow)) << " MHz centered on " << (frequencyLow+frequencyHigh)*0.5e-6 << " MHz)\n";
		}
		else {
			std::cout << "File " << filename << " not found, initializing empty ps...\n";
		}
	}
	
	void Save()
	{
		Save(_filename);
	}
	
	void Save(const std::string& filename)
	{
		std::cout << "Saving " << filename << "... " << std::flush;
		trim();
		std::ofstream file(filename);
		writeHeader(file, _header);
		
		for(size_t i=0; i!=_header.channelCount; ++i)
			writeChannelData(file, _channels[i]);
		
		std::cout << '\n';
	}
	
	size_t ChannelCount() const { return _header.channelCount; }
	bool HasCentre() const { return std::isfinite(_header.centreRA) && std::isfinite(_header.centreDec); }
	void SetCentre(double ra, double dec) { _header.centreRA = ra; _header.centreDec = dec; }
	double CentreRA() const { return _header.centreRA; }
	double CentreDec() const { return _header.centreDec; }
	size_t DefaultPerpGridSize() const { return _header.defaultPerpGridSize; }
	
	ChannelData& NewChannel(size_t gridSize, double frequency, double bandwidth, PolarizationEnum polarization, double xPerpStart, double xPerpStep)
	{
		_channels.emplace_back();
		++_header.channelCount;
		_channels.back().frequency = frequency;
		_channels.back().bandwidth = bandwidth;
		_channels.back().polarization = polarization;
		_channels.back().xPerpStart = xPerpStart;
		_channels.back().xPerpStep = xPerpStep;
		_channels.back().values.assign(gridSize, 0.0);
		_channels.back().weights.assign(gridSize, 0.0);
		return _channels.back();
	}
	
	ChannelData& Channel(size_t index) { return _channels[index]; }
	
	bool ChannelIndex(double frequency, size_t &index)
	{
		for(size_t i=0; i!=_channels.size(); ++i)
		{
			if(_channels[i].frequency == frequency)
			{
				index = i;
				return true;
			}
		}
		return false;
	}
	
	void GetFrequencyRange(double& lowest, double& highest)
	{
		lowest = std::numeric_limits<double>::max();
		highest = 0.0;
		for(size_t i=0; i!=_channels.size(); ++i)
		{
			double
				lowFreq = _channels[i].frequency - 0.5*_channels[i].bandwidth,
				hiFreq = _channels[i].frequency + 0.5*_channels[i].bandwidth;
			if(lowFreq < lowest) lowest = lowFreq;
			if(hiFreq > highest) highest = hiFreq;
		}
		if(lowest > highest) lowest = highest;
	}
	
private:
	struct Header
	{
		char magic[4];
		uint16_t versionHi, versionLo;
		uint64_t channelCount, defaultPerpGridSize;
		double centreRA, centreDec;
		
		Header() :
			versionHi(1), versionLo(0),
			channelCount(0), defaultPerpGridSize(16*1024),
			centreRA(std::numeric_limits<double>::quiet_NaN()), centreDec(std::numeric_limits<double>::quiet_NaN())
			{
				magic[0]='A'; magic[1]='O';
				magic[2]='P'; magic[3]='S';
			}
	};
	
	static void readHeader(std::ifstream& file, Header& header)
	{
		file.read(reinterpret_cast<char*>(&header), sizeof(Header));
	}
	
	static void writeHeader(std::ofstream& file, Header& header)
	{
		file.write(reinterpret_cast<char*>(&header), sizeof(Header));
	}
	
	static void readChannelData(std::ifstream& file, ChannelData& data)
	{
		data.frequency = readDouble(file);
		data.bandwidth = readDouble(file);
		std::string polStr = readString(file);
		data.polarization = Polarization::ParseString(polStr);
		data.xPerpStep = readDouble(file);
		data.xPerpStart = readDouble(file);
		size_t valueCount = readUInt64(file);
		data.values.resize(valueCount);
		data.weights.resize(valueCount);
		file.read(reinterpret_cast<char*>(data.values.data()), sizeof(double)*valueCount);
		file.read(reinterpret_cast<char*>(data.weights.data()), sizeof(double)*valueCount);
	}
	
	static void writeChannelData(std::ofstream& file, ChannelData& data)
	{
		writeDouble(file, data.frequency);
		writeDouble(file, data.bandwidth);
		writeString(file, Polarization::TypeToShortString(data.polarization));
		writeDouble(file, data.xPerpStep);
		writeDouble(file, data.xPerpStart);
		writeUInt64(file, data.values.size());
		file.write(reinterpret_cast<char*>(data.values.data()), sizeof(double)*data.values.size());
		file.write(reinterpret_cast<char*>(data.weights.data()), sizeof(double)*data.weights.size());
	}

	static uint64_t readUInt64(std::ifstream& file)
	{
		uint64_t v;
		file.read(reinterpret_cast<char*>(&v), sizeof(uint64_t));
		return v;
	}
	
	static void writeUInt64(std::ofstream& file, uint64_t val)
	{
		file.write(reinterpret_cast<char*>(&val), sizeof(val));
	}
	
	static double readDouble(std::ifstream& file)
	{
		double v;
		file.read(reinterpret_cast<char*>(&v), sizeof(double));
		return v;
	}
	
	static void writeDouble(std::ofstream& file, double val)
	{
		file.write(reinterpret_cast<char*>(&val), sizeof(val));
	}
	
	static std::string readString(std::ifstream& file)
	{
		std::ostringstream s;
		char c;
		bool end = false;
		do
		{
			file.read(&c, 1);
			end = (c == 0);
			if(!end)
				s << c;
		} while(!end);
		return s.str();
	}
	
	static void writeString(std::ofstream& file, const std::string& val)
	{
		file.write(val.c_str(), val.size()+1);
	}
	
	void trim()
	{
		size_t requiredSize = 0;
		for(size_t ch=0; ch!=_channels.size(); ++ch)
			requiredSize = std::max(minimumSize(_channels[ch]), requiredSize);
		std::cout << "Resizing perpendicular grid to " << requiredSize << "... " << std::flush;
		for(size_t ch=0; ch!=_channels.size(); ++ch)
		{
			_channels[ch].values.resize(requiredSize);
			_channels[ch].weights.resize(requiredSize);
		}
	}
	
	static size_t minimumSize(PSDataFile::ChannelData& data)
	{
		size_t lastValue = data.values.size()-1;
		while(lastValue>0) {
			if(data.values[lastValue] != 0.0)
				return lastValue + 1;
			--lastValue;
		}
		return 0.0;
	}
	
	const std::string _filename;
	Header _header;
	std::vector<ChannelData> _channels;
};

#endif
