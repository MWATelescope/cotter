#include "mwams.h"

#include <ms/MeasurementSets/MeasurementSet.h>

#include <tables/Tables/ArrayColumn.h>
#include <tables/Tables/ArrColDesc.h>
#include <tables/Tables/ScalarColumn.h>
#include <tables/Tables/ScaColDesc.h>
#include <tables/Tables/SetupNewTab.h>

#include <measures/TableMeasures/TableMeasDesc.h>

#include <measures/Measures/MEpoch.h>

#include <stdexcept>
#include <sstream>

using namespace casa;

struct MWAMSData
{
	MWAMSData(const std::string &filename) :
		_measurementSet(filename, Table::Update)
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
	"MWA_GPS_TIME", "MWA_FILENAME", "MWA_OBSERVATION_MODE",
	"MWA_FLAG_WINDOW_SIZE", "MWA_DATE_REQUESTED",
	
	/* Spectral window */
	"MWA_CENTRE_SUBBAND_NR",
	
	/* MWA_TILE_POINTING */
	"INTERVAL", "DELAYS", "DIRECTION",
	
	/* MWA_SUBBAND */
	"NUMBER", "GAIN", "FLAG_ROW"
};

const std::string MWAMS::_keywordNames[] = {
	"MWA_FIBER_VEL_FACTOR", "MWA_METADATA_VERSION"
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
	ArrayColumnDesc<double> cableLengthCD =
		ArrayColumnDesc<double>(columnName(MWAMSEnums::MWA_CABLE_LENGTH));
	
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
	MSObservation obsTable = _data->_measurementSet.observation();
	
	ScalarColumnDesc<double> gpsTimeCD =
		ScalarColumnDesc<double>(columnName(MWAMSEnums::MWA_GPS_TIME));
	ScalarColumnDesc<casa::String> filenameCD =
		ScalarColumnDesc<casa::String>(columnName(MWAMSEnums::MWA_FILENAME));
	ScalarColumnDesc<casa::String> obsModeCD =
		ScalarColumnDesc<casa::String>(columnName(MWAMSEnums::MWA_OBSERVATION_MODE));
	ScalarColumnDesc<int> flagWindowSizeCD =
		ScalarColumnDesc<int>(columnName(MWAMSEnums::MWA_FLAG_WINDOW_SIZE));
		
	ScalarColumnDesc<double> dateRequestedCD =
		ScalarColumnDesc<double>(columnName(MWAMSEnums::MWA_DATE_REQUESTED));
	
	obsTable.addColumn(gpsTimeCD);
	obsTable.addColumn(filenameCD);
	obsTable.addColumn(obsModeCD);
	obsTable.addColumn(flagWindowSizeCD);
	obsTable.addColumn(dateRequestedCD);
	
	casa::Vector<Unit> unitVec(1);
	unitVec[0] = Unit("s");
	TableMeasRefDesc measRef(MEpoch::DEFAULT);
	TableMeasValueDesc measVal(columnName(MWAMSEnums::MWA_DATE_REQUESTED));
	TableMeasDesc<MEpoch> intervalColMeas(measVal, measRef, unitVec);
	intervalColMeas.write(obsTable);
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
	TableDesc tilePointingTableDesc(tableName(MWAMSEnums::MWA_TILE_POINTING_TABLE), TableDesc::Scratch);
	
	ArrayColumnDesc<double> intervalCD =
		ArrayColumnDesc<double>(columnName(MWAMSEnums::INTERVAL));
	
	ArrayColumnDesc<int> delaysCD =
		ArrayColumnDesc<int>(columnName(MWAMSEnums::DELAYS));
	ArrayColumnDesc<double> directionCD =
		ArrayColumnDesc<double>(columnName(MWAMSEnums::DIRECTION));
		
	tilePointingTableDesc.addColumn(intervalCD);
	tilePointingTableDesc.addColumn(delaysCD);
	tilePointingTableDesc.addColumn(directionCD);
	
	casa::Vector<Unit> unitVec(1);
	unitVec[0] = Unit("s");
	TableMeasRefDesc measRef(MEpoch::DEFAULT);
	TableMeasValueDesc measVal(tilePointingTableDesc, columnName(MWAMSEnums::INTERVAL));
	TableMeasDesc<MEpoch> intervalColMeas(measVal, measRef, unitVec);
	intervalColMeas.write(tilePointingTableDesc);
	
	SetupNewTable newTilePointingTable(_filename + '/' + tableName(MWAMSEnums::MWA_TILE_POINTING_TABLE), tilePointingTableDesc, Table::New);
	Table tilePointingTable(newTilePointingTable);
	_data->_measurementSet.rwKeywordSet().defineTable(tableName(MWAMSEnums::MWA_TILE_POINTING_TABLE), tilePointingTable);
}

void MWAMS::addMWASubbandFields()
{
	TableDesc subbandTableDesc(tableName(MWAMSEnums::MWA_SUBBAND_TABLE), TableDesc::Scratch);
	
	ScalarColumnDesc<int> numberCD =
		ScalarColumnDesc<int>(columnName(MWAMSEnums::NUMBER));
	ScalarColumnDesc<double> gainCD =
		ScalarColumnDesc<double>(columnName(MWAMSEnums::GAIN));
	ScalarColumnDesc<bool> flagRowCD =
		ScalarColumnDesc<bool>(columnName(MWAMSEnums::FLAG_ROW));
		
	subbandTableDesc.addColumn(numberCD);
	subbandTableDesc.addColumn(gainCD);
	subbandTableDesc.addColumn(flagRowCD);
	
	SetupNewTable newSubbandTable(_filename + '/' + tableName(MWAMSEnums::MWA_SUBBAND_TABLE), subbandTableDesc, Table::New);
	Table subbandTable(newSubbandTable);
	_data->_measurementSet.rwKeywordSet().defineTable(tableName(MWAMSEnums::MWA_SUBBAND_TABLE), subbandTable);
}

void MWAMS::UpdateMWAAntennaInfo(size_t antennaIndex, const MWAMS::MWAAntennaInfo& info)
{
	MSAntenna antTable = _data->_measurementSet.antenna();
	
	ArrayColumn<int> inputCol =
		ArrayColumn<int>(antTable, columnName(MWAMSEnums::MWA_INPUT));
	ScalarColumn<int> tileNrCol =
		ScalarColumn<int>(antTable, columnName(MWAMSEnums::MWA_TILE_NR));
	ScalarColumn<int> receiverCol =
		ScalarColumn<int>(antTable, columnName(MWAMSEnums::MWA_RECEIVER));
	ArrayColumn<int> slotCol =
		ArrayColumn<int>(antTable, columnName(MWAMSEnums::MWA_SLOT));
	ArrayColumn<double> cableLengthCol =
		ArrayColumn<double>(antTable, columnName(MWAMSEnums::MWA_CABLE_LENGTH));
	
	casa::Vector<int> inputVec(2);
	inputVec[0] = info.inputX;
	inputVec[1] = info.inputY;
	inputCol.put(antennaIndex, inputVec);
	
	tileNrCol.put(antennaIndex, info.tileNr);
	receiverCol.put(antennaIndex, info.receiver);
	
	casa::Vector<int> slotVec(2);
	slotVec[0] = info.slotX;
	slotVec[1] = info.slotY;
	slotCol.put(antennaIndex, slotVec);
	
	casa::Vector<double> cableLengthVec(2);
	cableLengthVec[0] = info.cableLengthX;
	cableLengthVec[1] = info.cableLengthY;
	cableLengthCol.put(antennaIndex, cableLengthVec);
}

void MWAMS::UpdateMWAFieldInfo(bool hasCalibrator)
{
	MSField fieldTable = _data->_measurementSet.field();
	
	if(fieldTable.nrow() != 1) {
		std::stringstream s;
		s << "The field table of a MWA MS should have exactly one row, but in " << _filename << " it has " << fieldTable.nrow() << " rows.";
		throw std::runtime_error(s.str());
	}
	
	ScalarColumn<bool> hasCalCol =
		ScalarColumn<bool>(fieldTable, columnName(MWAMSEnums::MWA_HAS_CALIBRATOR));
	hasCalCol.put(0, hasCalibrator);
}

void MWAMS::UpdateMWAObservationInfo(const MWAMS::MWAObservationInfo& info)
{
	MSObservation obsTable = _data->_measurementSet.observation();
	
	if(obsTable.nrow() != 1) {
		std::stringstream s;
		s << "The observation table of a MWA MS should have exactly one row, but in " << _filename << " it has " << obsTable.nrow() << " rows.";
		throw std::runtime_error(s.str());
	}
	
	ScalarColumn<double> gpsTimeCol =
		ScalarColumn<double>(obsTable, columnName(MWAMSEnums::MWA_GPS_TIME));
	ScalarColumn<casa::String> filenameCol =
		ScalarColumn<casa::String>(obsTable, columnName(MWAMSEnums::MWA_FILENAME));
	ScalarColumn<casa::String> obsModeCol =
		ScalarColumn<casa::String>(obsTable, columnName(MWAMSEnums::MWA_OBSERVATION_MODE));
	ScalarColumn<int> flagWindowSizeCol =
		ScalarColumn<int>(obsTable, columnName(MWAMSEnums::MWA_FLAG_WINDOW_SIZE));
	ScalarColumn<double> dateRequestedCol =
		ScalarColumn<double>(obsTable, columnName(MWAMSEnums::MWA_DATE_REQUESTED));
		
	gpsTimeCol.put(0, info.gpsTime);
	filenameCol.put(0, info.filename);
	obsModeCol.put(0, info.observationMode);
	flagWindowSizeCol.put(0, info.flagWindowSize);
	dateRequestedCol.put(0, info.dateRequested);
}

void MWAMS::UpdateSpectralWindowInfo(int mwaCentreSubbandNr)
{
	MSSpectralWindow spwTable = _data->_measurementSet.spectralWindow();
	
	if(spwTable.nrow() != 1) {
		std::stringstream s;
		s << "The spectralwindow table of a MWA MS should have exactly one row, but in " << _filename << " it has " << spwTable.nrow() << " rows.";
		throw std::runtime_error(s.str());
	}
	
	ScalarColumn<int> centreSubbandNrCol =
		ScalarColumn<int>(spwTable, columnName(MWAMSEnums::MWA_CENTRE_SUBBAND_NR));

	centreSubbandNrCol.put(0, mwaCentreSubbandNr);
}

void MWAMS::WriteMWATilePointingInfo(double start, double end, const int* delays, double directionRA, double directionDec)
{
	Table tilePntTable(_data->_measurementSet.rwKeywordSet().asTable(tableName(MWAMSEnums::MWA_TILE_POINTING_TABLE)));
	
	ArrayColumn<double> intervalCol =
		ArrayColumn<double>(tilePntTable, columnName(MWAMSEnums::INTERVAL));
	ArrayColumn<int> delaysCol =
		ArrayColumn<int>(tilePntTable, columnName(MWAMSEnums::DELAYS));
	ArrayColumn<double> directionCol =
		ArrayColumn<double>(tilePntTable, columnName(MWAMSEnums::DIRECTION));
		
	size_t index = tilePntTable.nrow();
	tilePntTable.addRow();
		
	casa::Vector<double> intervalVec(2);
	intervalVec[0] = start;
	intervalVec[1] = end;
	intervalCol.put(index, intervalVec);
	
	casa::Vector<int> delaysVec(16);
	for(size_t i=0; i!=16; ++i) delaysVec[i] = delays[i];
	delaysCol.put(index, delaysVec);
	
	casa::Vector<double> dirVec(2);
	dirVec[0] = directionRA; dirVec[1] = directionDec;
	directionCol.put(index, dirVec);
}

void MWAMS::WriteMWASubbandInfo(int number, double gain, bool isFlagged)
{
	Table sbTable(_data->_measurementSet.rwKeywordSet().asTable(tableName(MWAMSEnums::MWA_SUBBAND_TABLE)));
	
	ScalarColumn<int> numberCol =
		ScalarColumn<int>(sbTable, columnName(MWAMSEnums::NUMBER));
	ScalarColumn<double> gainCol =
		ScalarColumn<double>(sbTable, columnName(MWAMSEnums::GAIN));
	ScalarColumn<bool> flagRowCol =
		ScalarColumn<bool>(sbTable, columnName(MWAMSEnums::FLAG_ROW));
		
	size_t index = sbTable.nrow();
	sbTable.addRow();
		
	numberCol.put(index, number);
	gainCol.put(index, gain);
	flagRowCol.put(index, isFlagged);
}

void MWAMS::WriteMWAKeywords(double fibreVelFactor, int metaDataVersion)
{
	_data->_measurementSet.rwKeywordSet().define(keywordName(MWAMSEnums::MWA_FIBER_VEL_FACTOR), fibreVelFactor);
	_data->_measurementSet.rwKeywordSet().define(keywordName(MWAMSEnums::MWA_METADATA_VERSION), metaDataVersion);
}
