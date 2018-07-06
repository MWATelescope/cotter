#ifndef SOLUTION_FILE_H
#define SOLUTION_FILE_H

#include <cstring>
#include <string>
#include <fstream>
#include <complex>
#include <stdexcept>
#include <vector>

#include <stdint.h>

/**
 * The solution file is used for storing calibration Jones matrices. The format is as follows:
 *  Bytes |  Description 
 * -------+---------------
 *  0- 7  |  string intro ; 8-byte null terminated string "MWAOCAL"
 *  8-11  |  int fileType ; always 0, reserved for indicating something other than complex Jones solutions
 * 12-15  |  int structureType ; always 0, reserved for indicating different ordering
 * 16-19  |  int intervalCount ; Number of solution intervals in file
 * 20-23  |  int antennaCount ; Number of antennas that were in the measurement set (but were not necessary all solved for)
 * 24-27  |  int channelCount ; Number of channels in the measurement set
 * 28-31  |  int polarizationCount ; Number of polarizations solved for -- always four.
 * 32-39  |  double startTime ; Start time of solutions (AIPS time)
 * 40-47  |  double endTime ; End time of solutions (AIPS time)
 * -------+-------------------
 * After the header follow 2 x nSolution doubles, with
 * 
 * nSolutions = nIntervals * nAntennas * nChannels * nPols 
 * 
 * Ordered in the way as given, so:
 * double 0 : real of interval 0, antenna 0, channel 0, pol 0
 * double 1 : imaginary of interval 0, antenna 0, channel 0, pol 0
 * double 2 : real of interval 0, antenna 0, channel 0, pol 1
 * ...
 * double 8 : real of interval 0, antenna 0, channel 1, pol 0
 * double nChannel x 8 : real of interval 0, antenna 1, channel 0, pol 0
 * etc.
 * 
 * here, ints are always 32 bits unsigned integers, doubles are IEEE double precision 64 bit floating points.
 * If a solution is not available, either because no data were selected during calibration for this interval
 * or because the calibration diverged, a "NaN" will be stored in the doubles belonging to that solution.
 */
class SolutionFile
{
 public:
	/** Empty constructor. After constructing, either @ref OpenForReading() should be called or the parameters should
	 * be initialized and @ref OpenForWriting() or @ref OpenInMemory() should be called.
	 */
  SolutionFile() : _outputStream(0), _inputStream(0), _readPointer(nullptr)
  {
    strcpy(_header.intro, "MWAOCAL");
    _header.fileType = 0; // Complex jones solutions
    _header.structureType = 0; // ordered real/imag, polarization, channel, antenna, time
    _header.intervalCount = 1;
  }

  /** Destructor. */
  ~SolutionFile() {
    delete _outputStream;
    delete _inputStream;
  }

  /** Number of antennas stored in file. */
  size_t AntennaCount() const { return _header.antennaCount; }
  void SetAntennaCount(size_t antennaCount) {
    _header.antennaCount = antennaCount;
  }

  /** Number of frequency channels in solution file. */
  size_t ChannelCount() const { return _header.channelCount; }
  void SetChannelCount(size_t channelCount) {
    _header.channelCount = channelCount;
  }

  /** Number of polarizations solved for. This should always be four for now, indicating that
	 * full Jones matrices were solved for. */
  size_t PolarizationCount() const { return _header.polarizationCount; }
  void SetPolarizationCount(size_t polarizationCount) {
    _header.polarizationCount = polarizationCount;
  }
  
  /** Number of intervals in solution file. */
  size_t IntervalCount() const { return _header.intervalCount; }
  void SetIntervalCount(size_t intervalCount) {
		_header.intervalCount = intervalCount;
	}

	/** Open a new file on disk for writing. After calling this method,
	 * data can be appended with the @ref WriteSolution() method.
	 * @param filename Name of file to write.
	 */
  void OpenForWriting(const char *filename)
  {
		delete _outputStream;
		_outputStream = new std::ofstream(filename);    
		_data.clear();
		
		_outputStream->write(reinterpret_cast<const char*>(&_header), sizeof(_header));
		double timeStart = 0.0, timeEnd = 0.0;
		_outputStream->write(reinterpret_cast<const char*>(&timeStart), sizeof(timeStart));
		_outputStream->write(reinterpret_cast<const char*>(&timeEnd), sizeof(timeEnd)); 
  }
  
  /** Open a file for writing and reading. This allows a calibration algorithm to use this class
	 * as buffer, without having to store the solutions to disk. After calling this method,
	 * @ref WriteSolution() should be used to store all solutions in the memory. After that, the
	 * @ref ReadNextSolution() method can be called to read all the solutions.
	 */
  void OpenInMemory()
	{
		delete _outputStream;
		_outputStream = 0;
		
		_data.resize(_header.intervalCount * _header.antennaCount * _header.channelCount * _header.polarizationCount);
		_readPointer = &_data[0];
	}

	/** Open a file for reading from disk. This method will read the header of the file. After calling this method,
	 * the settings of this class (@ref IntervalCount(), etc.) are initialized and can be used to determine the number
	 * of reads required. After that, the @ref ReadNextSolution() can be used to read the solutions one by one.
	 * @param filename File to read.
	 */
	void OpenForReading(const char *filename)
	{
		delete _inputStream;
		_inputStream = new std::ifstream(filename);
		if(_inputStream->bad())
			throw std::runtime_error("Error reading input solutions file");
		_inputStream->read(reinterpret_cast<char*>(&_header), sizeof(_header));
		double timeStart, timeEnd;
		_inputStream->read(reinterpret_cast<char*>(&timeStart), sizeof(timeStart));
		_inputStream->read(reinterpret_cast<char*>(&timeEnd), sizeof(timeEnd)); 
		if(_inputStream->bad())
			throw std::runtime_error("Error reading header from solutions file");
	}

	/** Read a complex solution from the file.
	 * This method should be called nIntervals * nAntennas * nChannels * nPols(=4) times. Four reads 
	 * will give one Jones matrix.
	 */
  std::complex<double> ReadNextSolution() {
		if(_inputStream == 0)
		{
			std::complex<double> val = *_readPointer;
			++_readPointer;
			return val;
		}
		else {
			std::complex<double> val;
			_inputStream->read(reinterpret_cast<char*>(&val), sizeof(val));
			return val;
		}
  }

  /** Write a solution to disk.
	 * This method can be called in any order, but all values should be written to before closing the file.
	 */
  void WriteSolution(const std::complex<double> &val, size_t interval, size_t antenna, size_t channel, size_t polarization)
  {
		size_t index = ((interval * _header.antennaCount + antenna) * _header.channelCount + channel) * _header.polarizationCount + polarization;
		if(_outputStream == 0)
		{
			_data[index] = val;
		}
		else {
			size_t offset = sizeof(_header) + sizeof(double)*2;
			_outputStream->seekp(offset + sizeof(val) * index, std::ios::beg);
			_outputStream->write(reinterpret_cast<const char*>(&val), sizeof(val));
		}
  }

 private:
  struct {
    char intro[8];
    uint32_t fileType;
    uint32_t structureType;
    uint32_t intervalCount, antennaCount, channelCount, polarizationCount;
  } _header;
  std::ofstream *_outputStream;
  std::ifstream *_inputStream;
	std::vector<std::complex<double> > _data;
	std::complex<double>* _readPointer;
};

#endif
