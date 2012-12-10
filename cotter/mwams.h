#ifndef MWA_MS
#define MWA_MS

#include <string>

struct MWAMSEnums
{
	enum MWATables
	{
		MWA_TILE_POINTING_TABLE, MWA_SUBBAND_TABLE
	};
	enum MWAColumns
	{
		/* Antenna */
		MWA_INPUT, MWA_TILE_NR, MWA_RECEIVER, MWA_SLOT, MWA_CABLE_LENGTH,
		
		/* Field */
		MWA_HAS_CALIBRATOR,
		
		/* Observation */
		MWA_GPS_TIME, MWA_FILENAME, MWA_OBSERVATION_MODE, MWA_RAW_FILE_CREATION_DATE,
		MWA_FLAG_WINDOW_SIZE,
		
		/* Spectral window */
		MWA_CENTRE_SUBBAND_NR,
		
		/* MWA_TILE_POINTING */
		INTERVAL, DELAYS,
		
		/* MWA_SUBBAND */
		NUMBER, GAIN, FLAG_ROW
	};
};

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
		~MWAMS();
		
		void AddMWAFields()
		{
			addMWAAntennaFields();
			addMWAFieldFields();
			addMWAObservationFields();
			addMWASpectralWindowFields();
			addMWATilePointingFields();
		}
		
		void WriteMWAAntennaInfo(size_t antennaIndex, const MWAAntennaInfo &info);
		
		void WriteMWAObservationInfo(const MWAObservationInfo &info);
		
		void WriteSpectralWindowInfo(size_t spwIndex, int mwaCentreSubbandNr);
		
		void WriteMWATilePointingInfo(double start, double end, const int *delays);
		
		void WriteMWASubbandInfo(int number, double gain, bool isFlagged);
		
		void WriteMWAKeywords(double fibreVelFactor, double rawFileCreationDate, int metaDataVersion, bool hasCalibrator);
		
	private:
		void addMWAAntennaFields();
		void addMWAFieldFields();
		void addMWAObservationFields();
		void addMWASpectralWindowFields();
		void addMWATilePointingFields();
		
		const std::string &columnName(enum MWAMSEnums::MWAColumns column)
		{
			return _columnNames[(int) column];
		}
		
		const std::string &tableName(enum MWAMSEnums::MWATables table)
		{
			return _tableName[(int) table];
		}
		
		static const std::string _columnNames[], _tableNames[];
		
		const std::string _filename;
		struct MWAMSData *_data;
};

#endif
