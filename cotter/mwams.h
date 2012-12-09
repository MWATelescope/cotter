#ifndef MWA_MS
#define MWA_MS

#include <string>

class MWAMS
{
	public:
		struct MWAAntennaInfo {
			int inputX, inputY, tileNr, receiver, slot;
			double cableLength;
		};
		
		struct MWAObservationInfo {
			double gpsTime;
			std::string filename, mode;
		};
		
		MWAMS(const std::string &filename);
		
		void AddMWAFields();
		
		void WriteMWAAntennaInfo(size_t antennaIndex, const MWAAntennaInfo &info);
		
		void WriteMWAObservationInfo(const MWAObservationInfo &info);
		
		void WriteSpectralWindowInfo(size_t spwIndex, int mwaCentreSubbandNr);
		
		void WriteMWATilePointingInfo(double start, double end, const int *delays);
		
		void WriteMWASubbandInfo(int number, double gain, bool isFlagged);
		
		void WriteMWAKeywords(double fibreVelFactor, double rawFileCreationDate, int metaDataVersion, bool hasCalibrator);
		
	private:
		const std::string _filename;
		class MWAMSData *_data;
};

#endif
