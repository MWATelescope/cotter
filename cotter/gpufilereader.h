#include <string>
#include <vector>

#include <fitsio.h>
#include <stdexcept>

#include "baselinebuffer.h"

/**
 * The GPU file reader, that can read the files produced by the MWA correlator.
 * Format based on reader in build_lfiles.c by sord.
 * 
 * To use this class:
 * - Construct it
 * - Add all GPU files to the reader that belong to the observation with equal time range
 *   by calling AddFile()
 * - Then, call Initialize() to read the required metadata, such as the antenna count.
 * - Allocate destination buffers and set them with SetDestBaselineBuffer()
 * - Finally, call Read() to start reading all data.
 * 
 * The MWA correlator produces multiple GPU files that need to be combined. Each
 * GPU file contains a selected number of channels of each scan.
 * The elements inside each scan are ordered by
 * polarization2, polarization1, station2, station1, channel, where polarization2
 * is the index that is moving fastest.
 * station2 is the non-conjugated station within a baseline.
 * 
 * @author André Offringa
 */
class GPUFileReader
{
	public:
		GPUFileReader() : _isOpen(false), _nAntenna(32), _nChannelsInTotal(3072), _bufferSize(0) { }
		~GPUFileReader() { closeFiles(); }
		
		void AddFile(const char *filename) { _filenames.push_back(std::string(filename)); }
		
		void Initialize() { _buffers.resize(_nAntenna * _nAntenna); }
		
		size_t AntennaCount() { return _nAntenna; }
		size_t ChannelCount() { return _nChannelsInTotal; }
		
		void SetDestBaselineBuffer(size_t antenna1, size_t antenna2, const BaselineBuffer &buffer)
		{
			if(_bufferSize != buffer.nElementsPerRow)
			{
				if(_bufferSize != 0)
					throw std::runtime_error("Given baseline buffers are not all of the same size");
				_bufferSize = buffer.nElementsPerRow;
			}
			getBuffer(antenna1, antenna2) = buffer;
		}
		
		bool Read(size_t &bufferPos);
	private:
		GPUFileReader(const GPUFileReader &) { }
		void operator=(const GPUFileReader &) { }
		void checkStatus(int status);
		void throwError(int status, const std::string &msg = std::string());
		void openFiles();
		void closeFiles();
		void findStopHDU();
		BaselineBuffer &getBuffer(size_t antenna1, size_t antenna2)
		{
			return _buffers[_nAntenna*antenna1 + antenna2];
		}
		
		bool _isOpen;
		size_t _nAntenna, _nChannelsInTotal, _bufferSize, _currentHDU, _stopHDU;
		std::vector<std::string> _filenames;
		std::vector<size_t> _fitsHDUCounts;
		std::vector<fitsfile *> _fitsFiles;
		
		std::vector<BaselineBuffer> _buffers;
};
