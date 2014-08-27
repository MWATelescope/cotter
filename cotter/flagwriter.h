#ifndef FLAG_WRITER_H
#define FLAG_WRITER_H

#include "writer.h"
#include "fitsuser.h"

#include <stdint.h>

#include <iostream>
#include <fitsio.h>

class FlagWriter : public Writer, private FitsUser
{
	public:
		FlagWriter(const std::string &filename, int gpsTime, size_t timestepCount, size_t sbStart, size_t sbEnd, const std::vector<size_t>& subbandToGPUBoxFileIndex);
		
		~FlagWriter();
		
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
			if(_rowsAdded == 0)
				writeHeader();
			_rowsAdded += rowCount;
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
		
		virtual void SetOffsetsPerGPUBox(const std::vector<int>& offsets);
	private:
		void writeHeader();
		void writeRow(size_t antenna1, size_t antenna2, const bool* flags);
		void setStride();
		struct Header
		{
			char fileIdentifier[4];
			uint16_t versionMinor, versionMajor;
			uint32_t timestepCount, antennaCount, channelCount, polarizationCount, gpuBoxIndex;
			uint64_t gpsTime;
			char baselineSelection;
		};
		
		void updateIntKey(size_t i, const char* keywordName, int value)
		{
			int status = 0;
			fits_update_key(_files[i], TINT, keywordName, &value, NULL, &status);
			checkStatus(status);
		}
		
		size_t _timestepCount, _antennaCount, _channelCount, _channelsPerGPUBox, _polarizationCount;
		//size_t _rowStride;
		size_t _rowsAdded, _rowsWritten, _sbStart, _sbEnd;
		int _gpsTime;
		std::vector<fitsfile*> _files;
		
		const static uint16_t VERSION_MINOR, VERSION_MAJOR;
		
		std::vector<size_t> _subbandToGPUBoxFileIndex;
		std::vector<unsigned char> _singlePolBuffer;
		std::vector<int> _hduOffsets;
		//std::vector<unsigned char> _packBuffer;
};

#endif
