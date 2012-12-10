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
			int inputX, inputY, tileNr, receiver, slotX, slotY;
			double cableLength;
		};
		
		struct MWAObservationInfo {
			double gpsTime;
			std::string filename, observationMode;
			double rawFileCreationDate;
			int flagWindowSize;
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
			addMWASubbandFields();
		}
		
		void UpdateMWAAntennaInfo(size_t antennaIndex, const MWAAntennaInfo &info);
		
		void UpdateMWAFieldInfo(bool hasCalibrator);
		
		void UpdateMWAObservationInfo(const MWAObservationInfo &info);
		
		void UpdateSpectralWindowInfo(int mwaCentreSubbandNr);
		
		void WriteMWATilePointingInfo(double start, double end, const int *delays);
		
		void WriteMWASubbandInfo(int number, double gain, bool isFlagged);
		
		void WriteMWAKeywords(double fibreVelFactor, int metaDataVersion);
		
	private:
		void addMWAAntennaFields();
		void addMWAFieldFields();
		void addMWAObservationFields();
		void addMWASpectralWindowFields();
		void addMWATilePointingFields();
		void addMWASubbandFields();
		
		const std::string &columnName(enum MWAMSEnums::MWAColumns column)
		{
			return _columnNames[(int) column];
		}
		
		const std::string &tableName(enum MWAMSEnums::MWATables table)
		{
			return _tableNames[(int) table];
		}
		
		static const std::string _columnNames[], _tableNames[];
		
		const std::string _filename;
		struct MWAMSData *_data;
};

#endif