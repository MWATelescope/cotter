#ifndef FLAG_WRITER_H
#define FLAG_WRITER_H

#include "writer.h"

#include <stdint.h>

#include <fstream>
#include <iostream>

class FlagWriter : public Writer
{
	public:
		FlagWriter(const std::string &filename, int gpsTime) :
		_channelCount(0),
		_antennaCount(0),
		_polarizationCount(0),
		_rowStride(0),
		_rowCount(0),
		_gpsTime(0),
		_file(filename.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc)
		{
		}
		
		~FlagWriter()
		{
		}
		
		void WriteBandInfo(const std::string &name, const std::vector<Writer::ChannelInfo> &channels, double refFreq, double totalBandwidth, bool flagRow)
		{
			_channelCount = channels.size();
			setStride();
		}
		
		void WriteAntennae(const std::vector<Writer::AntennaInfo> &antennae, double time)
		{
			_antennaCount = antennae.size();
			setStride();
		}
		
		void WritePolarizationForLinearPols(bool flagRow)
		{
			_polarizationCount = 4;
			setStride();
		}
		
		void WriteSource(const Writer::SourceInfo &source)
		{
		}
		
		void WriteField(const Writer::FieldInfo& field)
		{
		}
		
		void WriteObservation(const std::string& telescopeName, double startTime, double endTime, const std::string& observer, const std::string& scheduleType, const std::string& project, double releaseDate, bool flagRow)
		{
		}
		
		void AddRows(size_t rowCount)
		{
			if(_rowCount == 0)
				writeHeader();
			_rowCount += rowCount;
		}
		
		void WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, const std::complex<float>* data, const bool* flags, const float *weights)
		{
			writeRow(antenna1, antenna2, flags);
		}
		
		void WriteHistoryItem(const std::string &commandLine, const std::string &application, const std::vector<std::string> &params)
		{
		}
		
		bool IsTimeAligned(size_t antenna1, size_t antenna2)
		{
			return true;
		}
	private:
		void writeHeader();
		void writeRow(size_t antenna1, size_t antenna2, const bool* flags);
		void setStride()
		{
			// we assume we write only one polarization here
			_rowStride = (_channelCount + 7) / 8;
			
			_singlePolBuffer.resize(_channelCount);
			_packBuffer.resize(_rowStride);
		}
		static void pack(unsigned char *output, const unsigned char *input, size_t count);
		struct Header
		{
			uint16_t versionMinor, versionMajor;
			uint32_t antennaCount, channelCount, polarizationCount;
			uint64_t gpsTime;
			char baselineSelection;
		};
		
		size_t _channelCount, _antennaCount, _polarizationCount;
		size_t _rowStride, _rowCount;
		int _gpsTime;
		std::ofstream _file;
		
		const static uint16_t VERSION_MINOR, VERSION_MAJOR;
		
		std::vector<unsigned char> _singlePolBuffer, _packBuffer;
};

#endif
