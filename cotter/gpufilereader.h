#include <string>
#include <vector>
#include <ctime>
#include <complex>

#include <fitsio.h>
#include <stdexcept>

#include "baselinebuffer.h"
#include "fitsuser.h"
#include "lane.h"

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
 * - Set the input-to-antenna mapping for all inputs with SetCorrInputToOutput().
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
class GPUFileReader : private FitsUser
{
	public:
		GPUFileReader(size_t nAntenna, size_t nChannelsInTotal, size_t threadCount) :
			_shuffleTasks(threadCount),
			_availableGPUMatrixBuffers(threadCount),
			_isOpen(false),
			_nAntenna(nAntenna),
			_nChannelsInTotal(nChannelsInTotal),
			_bufferSize(0),
			_currentHDU(0),
			_stopHDU(0),
			_threadCount(threadCount)
		{ }
		~GPUFileReader() { closeFiles(); }
		
		void AddFile(const char *filename) { _filenames.push_back(std::string(filename)); }
		
		void Initialize() {
			_buffers.resize(_nAntenna * _nAntenna);
			_mappedBuffers.resize(_nAntenna * _nAntenna);
			_corrInputToOutput.resize(_nAntenna*2);
		}
		
		size_t AntennaCount() { return _nAntenna; }
		size_t ChannelCount() { return _nChannelsInTotal; }
		
		void ResetBuffers()
		{
			_bufferSize = 0;
		}
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
		void SetCorrInputToOutput(size_t input, size_t outputAnt, size_t outputPol)
		{
			_corrInputToOutput[input] = outputAnt*2 + outputPol;
		}
		
		bool Read(size_t &bufferPos, size_t count);
		bool IsConjugated(size_t ant1, size_t ant2, size_t pol1, size_t pol2) const
		{
			return _isConjugated[(ant1 * 2 + pol1) * _nAntenna * 2 + (ant2 * 2 + pol2)];
		}
		std::time_t StartTime() const { return _startTime; }
	private:
		struct ShuffleTask
		{
			size_t iFile, channelsInFile, fileBufferPos;
			std::complex<float> *gpuMatrix;
		};
		lane<ShuffleTask> _shuffleTasks;
		lane<std::complex<float> *> _availableGPUMatrixBuffers;
		
		const static int single_pfb_output_to_input[64];
		std::vector<int> pfb_output_to_input;
		
		GPUFileReader(const GPUFileReader &) : _shuffleTasks(0), _availableGPUMatrixBuffers(0) { }
		void operator=(const GPUFileReader &) { }
		void openFiles();
		void closeFiles();
		void findStopHDU();
		void initMapping();
		void initializePFBMapping();
		void shuffleThreadFunc();
		void shuffleBuffer(size_t iFile, size_t channelsInFile, size_t fileBufferPos, const std::complex<float> *gpuMatrix);
		BaselineBuffer &getBuffer(size_t antenna1, size_t antenna2)
		{
			return _buffers[_nAntenna*antenna1 + antenna2];
		}
		BaselineBuffer &getMappedBuffer(size_t antenna1, size_t antenna2)
		{
			return _mappedBuffers[_nAntenna*antenna1 + antenna2];
		}
		
		bool _isOpen;
		size_t _nAntenna, _nChannelsInTotal, _bufferSize, _currentHDU, _stopHDU;
		std::vector<std::string> _filenames;
		std::vector<size_t> _fitsHDUCounts;
		std::vector<fitsfile *> _fitsFiles;
		
		std::vector<BaselineBuffer> _buffers;
		std::vector<BaselineBuffer> _mappedBuffers;
		std::vector<size_t> _corrInputToOutput;
		std::vector<bool> _isConjugated;
		std::time_t _startTime;
		size_t _threadCount;
};
