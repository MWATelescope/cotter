#include "mwams.h"

#include <ms/MeasurementSets/MeasurementSet.h>

#include <tables/Tables/ArrayColumn.h>
#include <tables/Tables/ArrColDesc.h>
#include <tables/Tables/ScalarColumn.h>
#include <tables/Tables/ScaColDesc.h>
#include <tables/Tables/SetupNewTab.h>

using namespace casa;

struct MWAMSData
{
	MWAMSData(const std::string &filename) :
		_measurementSet(filename)
	{
	}
	
	MeasurementSet _measurementSet;
};

const std::string MWAMS::_tableNames[] = {
	"MWA_TILE_POINTING_TABLE", "MWA_SUBBAND_TABLE"
};

const std::string MWAMS::_columnNames[] = {
	/* Antenna */
	"MWA_INPUT", "MWA_TILE_NR", "MWA_RECEIVER", "MWA_SLOT", "MWA_CABLE_LENGTH",
	
	/* Field */
	"MWA_HAS_CALIBRATOR",
	
	/* Observation */
	"MWA_GPS_TIME", "MWA_FILENAME", "MWA_OBSERVATION_MODE", "MWA_RAW_FILE_CREATION_DATE",
	"MWA_FLAG_WINDOW_SIZE",
	
	/* Spectral window */
	"MWA_CENTRE_SUBBAND_NR",
	
	/* MWA_TILE_POINTING */
	"INTERVAL", "DELAYS",
	
	/* MWA_SUBBAND */
	"NUMBER", "GAIN", "FLAG_ROW"
};

MWAMS::MWAMS(const string& filename) : _filename(filename),
	_data(new MWAMSData(filename))
{
}

MWAMS::~MWAMS()
{
	delete _data;
}

void MWAMS::addMWAAntennaFields()
{
	ArrayColumnDesc<int> inputCD =
		ArrayColumnDesc<int>(columnName(MWAMSEnums::MWA_INPUT));
	ScalarColumnDesc<int> tileNrCD =
		ScalarColumnDesc<int>(columnName(MWAMSEnums::MWA_TILE_NR));
	ScalarColumnDesc<int> receiverCD =
		ScalarColumnDesc<int>(columnName(MWAMSEnums::MWA_RECEIVER));
	ArrayColumnDesc<int> slotCD =
		ArrayColumnDesc<int>(columnName(MWAMSEnums::MWA_SLOT));
	ScalarColumnDesc<double> cableLengthCD =
		ScalarColumnDesc<double>(columnName(MWAMSEnums::MWA_CABLE_LENGTH));
	
	MSAntenna antennaTable = _data->_measurementSet.antenna();
	antennaTable.addColumn(inputCD);
	antennaTable.addColumn(tileNrCD);
	antennaTable.addColumn(receiverCD);
	antennaTable.addColumn(slotCD);
	antennaTable.addColumn(cableLengthCD);
}

void MWAMS::addMWAFieldFields()
{
	ScalarColumnDesc<bool> hasCalibratorCD =
		ScalarColumnDesc<bool>(columnName(MWAMSEnums::MWA_HAS_CALIBRATOR));
	
	MSField fieldTable = _data->_measurementSet.field();
	fieldTable.addColumn(hasCalibratorCD);
}

void MWAMS::addMWAObservationFields()
{
	ScalarColumnDesc<double> gpsTimeCD =
		ScalarColumnDesc<double>(columnName(MWAMSEnums::MWA_GPS_TIME));
	ScalarColumnDesc<casa::String> filenameCD =
		ScalarColumnDesc<casa::String>(columnName(MWAMSEnums::MWA_FILENAME));
	ScalarColumnDesc<casa::String> obsModeCD =
		ScalarColumnDesc<casa::String>(columnName(MWAMSEnums::MWA_OBSERVATION_MODE));
	ScalarColumnDesc<double> rawFileCreationDateCD =
		ScalarColumnDesc<double>(columnName(MWAMSEnums::MWA_RAW_FILE_CREATION_DATE));
	ScalarColumnDesc<int> flagWindowSizeCD =
		ScalarColumnDesc<int>(columnName(MWAMSEnums::MWA_FLAG_WINDOW_SIZE));
	
	MSObservation obsTable = _data->_measurementSet.observation();
	obsTable.addColumn(gpsTimeCD);
	obsTable.addColumn(filenameCD);
	obsTable.addColumn(obsModeCD);
	obsTable.addColumn(rawFileCreationDateCD);
	obsTable.addColumn(flagWindowSizeCD);
}

void MWAMS::addMWASpectralWindowFields()
{
	ScalarColumnDesc<int> centreSubbandNrCD =
		ScalarColumnDesc<int>(columnName(MWAMSEnums::MWA_CENTRE_SUBBAND_NR));
		
	MSSpectralWindow spwTable = _data->_measurementSet.spectralWindow();
	spwTable.addColumn(centreSubbandNrCD);
}

void MWAMS::addMWATilePointingFields()
{
	TableDesc tilePointingTableDesc(tableName(MWAMSEnums::MWA_TILE_POINTING_TABLE), TableDesc::New);
	
	casa::ArrayColumnDesc<double> restFrequencyColumnDesc =
		ArrayColumnDesc<double>(MSSource::columnName(MSSourceEnums::REST_FREQUENCY));
		
	tilePointingTableDesc.addColumn(restFrequencyColumnDesc);
	
	SetupNewTable newTilePointingTable(tableName(MWAMSEnums::MWA_TILE_POINTING_TABLE), tilePointingTableDesc, Table::New);
	MSSource sourceTable(newSourceTable);
	ms.rwKeywordSet().defineTable(MS::keywordName(casa::MSMainEnums::SOURCE), sourceTable);
	
}
