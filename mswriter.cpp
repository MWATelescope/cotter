#include "mswriter.h"

#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#include <casacore/tables/DataMan/DataManager.h>
#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ArrColDesc.h>
#include <casacore/tables/Tables/ScalarColumn.h>
#include <casacore/tables/Tables/SetupNewTab.h>
#include <casacore/tables/Tables/TableRecord.h>

#include <casacore/casa/Containers/Record.h>

#include <casacore/measures/TableMeasures/TableMeasDesc.h>

#include <casacore/measures/Measures/MFrequency.h>

using namespace casacore;

class MSWriterData
{
	public:
		MeasurementSet _ms;
		
		ScalarColumn<double> _timeCol, _timeCentroidCol;
		ScalarColumn<int> _antenna1Col, _antenna2Col;
		ScalarColumn<int> _dataDescIdCol;
		ArrayColumn<double> _uvwCol;
		ScalarColumn<double> _intervalCol, _exposureCol;
		ScalarColumn<int> _processorIdCol, _scanNumberCol, _stateIdCol;
		ArrayColumn<std::complex<float> > _dataCol;
		ArrayColumn<bool> _flagCol;
		ArrayColumn<float> _sigmaCol;
		ArrayColumn<float> _weightCol;
		ArrayColumn<float> _weightSpectrumCol;

		size_t _dyscoDataBitRate, _dyscoWeightBitRate;
		std::string _dyscoDistribution, _dyscoNormalization;
		double _dyscoDistTruncation;
		
		MSWriterData() = default;
		void GetDyscoSpec(casacore::Record& record) const;
		
private:
	MSWriterData(const MSWriterData&) = delete;
	MSWriterData& operator=(const MSWriterData&) = delete;
};

MSWriter::MSWriter(const std::string& filename) :
	_data(new MSWriterData()),
	_isInitialized(false),
	_rowIndex(0),
	_filename(filename),
	_useDysco(false)
{
}

MSWriter::~MSWriter()
{
	if(!_isInitialized)
		initialize();
	delete _data;
}

void MSWriter::EnableCompression(size_t dataBitRate, size_t weightBitRate, const std::string& distribution, double distTruncation, const std::string& normalization)
{
	_useDysco = true;
	_data->_dyscoDataBitRate = dataBitRate;
	_data->_dyscoWeightBitRate = weightBitRate;
	_data->_dyscoDistribution = distribution;
	_data->_dyscoNormalization = normalization;
	_data->_dyscoDistTruncation = distTruncation;
}

void MSWriter::initialize()
{
	_isInitialized = true;
	
	TableDesc tableDesc = MS::requiredTableDesc();
	
	DataManagerCtor dyscoConstructor = 0;
	Record dyscoSpec;
	if(_useDysco) {
		_data->GetDyscoSpec(dyscoSpec);
		dyscoConstructor = DataManager::getCtor("DyscoStMan");
	}
	
	SetupNewTable newTab(_filename, tableDesc, Table::New);
	_data->_ms = MeasurementSet(newTab);
	MeasurementSet &ms = _data->_ms;
	ms.createDefaultSubtables(Table::New);
	
	ArrayColumnDesc<std::complex<float> > dataColumnDesc = ArrayColumnDesc<std::complex<float> >(MS::columnName(casacore::MSMainEnums::DATA));
	casacore::IPosition dataShape(2, 4, _bandInfo.channels.size());
	if (_useDysco && _data->_dyscoDataBitRate != 0) {
		dataColumnDesc.setShape(dataShape);
		dataColumnDesc.setOptions(ColumnDesc::Direct | ColumnDesc::FixedShape);

		// Add DATA column using Dysco stman.
		std::unique_ptr<DataManager> dyscoStMan(dyscoConstructor("DyscoData", dyscoSpec));
		ms.addColumn(dataColumnDesc, *dyscoStMan);
	}
	else {
		dataColumnDesc.setShape(dataShape);
		dataColumnDesc.setOptions(ColumnDesc::FixedShape);
		ms.addColumn(dataColumnDesc);
	}
	
	ArrayColumnDesc<float> weightSpectrumColumnDesc = ArrayColumnDesc<float>(MS::columnName(casacore::MSMainEnums::WEIGHT_SPECTRUM));
	if (_useDysco && _data->_dyscoWeightBitRate != 0) {
		weightSpectrumColumnDesc.setShape(dataShape);
		weightSpectrumColumnDesc.setOptions(ColumnDesc::Direct | ColumnDesc::FixedShape);
		std::unique_ptr<DataManager> dyscoStMan(dyscoConstructor("DyscoWeight", dyscoSpec));
		ms.addColumn(weightSpectrumColumnDesc, *dyscoStMan);
	}
	else {
		weightSpectrumColumnDesc.setShape(dataShape);
		weightSpectrumColumnDesc.setOptions(ColumnDesc::FixedShape);
		ms.addColumn(weightSpectrumColumnDesc);
	}
	
	TableDesc sourceTableDesc = MSSource::requiredTableDesc();
	casacore::ArrayColumnDesc<double> restFrequencyColumnDesc = ArrayColumnDesc<double>(MSSource::columnName(MSSourceEnums::REST_FREQUENCY));
	sourceTableDesc.addColumn(restFrequencyColumnDesc);
	
	TableMeasRefDesc measRef(MFrequency::DEFAULT);
	TableMeasValueDesc measVal(sourceTableDesc, MSSource::columnName(MSSourceEnums::REST_FREQUENCY));
	TableMeasDesc<MFrequency> restFreqColMeas(measVal, measRef);
	// write makes the Measure column persistent.
	restFreqColMeas.write(sourceTableDesc);

	SetupNewTable newSourceTable(ms.sourceTableName(), sourceTableDesc, Table::New);
	MSSource sourceTable(newSourceTable);
	ms.rwKeywordSet().defineTable(MS::keywordName(casacore::MSMainEnums::SOURCE), sourceTable);
	
	_data->_timeCol = ScalarColumn<double>(ms, MS::columnName(casacore::MSMainEnums::TIME));
	_data->_timeCentroidCol = ScalarColumn<double>(ms, MS::columnName(casacore::MSMainEnums::TIME_CENTROID));
	_data->_antenna1Col = ScalarColumn<int>(ms, MS::columnName(casacore::MSMainEnums::ANTENNA1));
	_data->_antenna2Col = ScalarColumn<int>(ms, MS::columnName(casacore::MSMainEnums::ANTENNA2));
	_data->_dataDescIdCol = ScalarColumn<int>(ms, MS::columnName(casacore::MSMainEnums::DATA_DESC_ID));
	_data->_uvwCol = ArrayColumn<double>(ms, MS::columnName(casacore::MSMainEnums::UVW));
	_data->_intervalCol = ScalarColumn<double>(ms, MS::columnName(casacore::MSMainEnums::INTERVAL));
	_data->_exposureCol = ScalarColumn<double>(ms, MS::columnName(casacore::MSMainEnums::EXPOSURE));
	_data->_processorIdCol = ScalarColumn<int>(ms, MS::columnName(casacore::MSMainEnums::PROCESSOR_ID));
	_data->_scanNumberCol = ScalarColumn<int>(ms, MS::columnName(casacore::MSMainEnums::SCAN_NUMBER));
	_data->_stateIdCol = ScalarColumn<int>(ms, MS::columnName(casacore::MSMainEnums::STATE_ID));
	_data->_dataCol = ArrayColumn<std::complex<float> >(ms, MS::columnName(casacore::MSMainEnums::DATA));
	_data->_sigmaCol = ArrayColumn<float>(ms, MS::columnName(casacore::MSMainEnums::SIGMA));
	_data->_weightCol = ArrayColumn<float>(ms, MS::columnName(casacore::MSMainEnums::WEIGHT));
	_data->_weightSpectrumCol = ArrayColumn<float>(ms, MS::columnName(casacore::MSMainEnums::WEIGHT_SPECTRUM));
	_data->_flagCol = ArrayColumn<bool>(ms, MS::columnName(casacore::MSMainEnums::FLAG));
	
	writeBandInfo();
	writeAntennae();
	writePolarizationForLinearPols();
	writeField();
	writeSource();
	writeObservation();
	writeHistoryItem();
}

void MSWriterData::GetDyscoSpec(casacore::Record& dyscoSpec) const
{
	dyscoSpec.define ("distribution", _dyscoDistribution);
	dyscoSpec.define ("normalization", _dyscoNormalization);
	dyscoSpec.define ("distributionTruncation", _dyscoDistTruncation);
	// a bitrate of 0 is used to disable compression of the data/weight column.
	// However, Dysco does not allow the data or weight bitrates to be set to 0,
	// so we set the values to something different. The values are not actually used.
	uint dataBitRate = _dyscoDataBitRate;
	if(dataBitRate == 0)
		dataBitRate = 16;
	dyscoSpec.define ("dataBitCount", dataBitRate);
	uint weightBitRate = _dyscoWeightBitRate;
	if(weightBitRate == 0)
		weightBitRate = 16;
	dyscoSpec.define ("weightBitCount", weightBitRate);
}

void MSWriter::WriteBandInfo(const std::string& name, const std::vector<ChannelInfo>& channels, double refFreq, double totalBandwidth, bool flagRow)
{
	_bandInfo.name = name;
	_bandInfo.channels = channels;
	_bandInfo.refFreq = refFreq;
	_bandInfo.totalBandwidth = totalBandwidth;
	_bandInfo.flagRow = flagRow;
}

void MSWriter::WriteAntennae(const std::vector<AntennaInfo>& antennae, double time)
{
	_antennae = antennae;
	_antennaDate = time;
}

void MSWriter::WritePolarizationForLinearPols(bool flagRow)
{
	_flagPolarizationRow = flagRow;
}

void MSWriter::WriteField(const FieldInfo& field)
{
	_field = field;
}

void MSWriter::WriteSource(const SourceInfo &source)
{
	_source = source;
}

void MSWriter::WriteObservation(const ObservationInfo& observation)
{
	_observation = observation;
}

void MSWriter::WriteHistoryItem(const std::string &commandLine, const std::string &application, const std::vector<std::string> &params)
{
	_historyCommandLine = commandLine;
	_historyApplication = application;
	_historyParams = params;
}

void MSWriter::writeBandInfo()
{
	MeasurementSet &ms = _data->_ms;
	MSSpectralWindow spwTable = ms.spectralWindow();
	
	ScalarColumn<int> numChanCol = ScalarColumn<int>(spwTable, spwTable.columnName(MSSpectralWindowEnums::NUM_CHAN));
	ScalarColumn<casacore::String> nameCol = ScalarColumn<casacore::String>(spwTable, spwTable.columnName(MSSpectralWindowEnums::NAME));
	ScalarColumn<double> refFreqCol = ScalarColumn<double>(spwTable, spwTable.columnName(MSSpectralWindowEnums::REF_FREQUENCY));
	
	ArrayColumn<double> chanFreqCol = ArrayColumn<double>(spwTable, spwTable.columnName(MSSpectralWindowEnums::CHAN_FREQ));
	ArrayColumn<double> chanWidthCol = ArrayColumn<double>(spwTable, spwTable.columnName(MSSpectralWindowEnums::CHAN_WIDTH));
	ScalarColumn<int> measFreqRefCol = ScalarColumn<int>(spwTable, spwTable.columnName(MSSpectralWindowEnums::MEAS_FREQ_REF));
	ArrayColumn<double> effectiveBWCol = ArrayColumn<double>(spwTable, spwTable.columnName(MSSpectralWindowEnums::EFFECTIVE_BW));
	ArrayColumn<double> resolutionCol = ArrayColumn<double>(spwTable, spwTable.columnName(MSSpectralWindowEnums::RESOLUTION));
	ScalarColumn<double> totalBWCol = ScalarColumn<double>(spwTable, spwTable.columnName(MSSpectralWindowEnums::TOTAL_BANDWIDTH));
	ScalarColumn<bool> flagRowCol = ScalarColumn<bool>(spwTable, spwTable.columnName(MSSpectralWindowEnums::FLAG_ROW));

	const size_t nChannels = _bandInfo.channels.size();
	size_t rowIndex = spwTable.nrow();
	spwTable.addRow();
	numChanCol.put(rowIndex, nChannels);
	nameCol.put(rowIndex, _bandInfo.name);
	refFreqCol.put(rowIndex, _bandInfo.refFreq);
	
	casacore::Vector<double>
		chanFreqVec(nChannels), chanWidthVec(nChannels),
		effectiveBWVec(nChannels), resolutionVec(nChannels);
	for(size_t ch=0; ch!=_bandInfo.channels.size(); ++ch)
	{
		chanFreqVec[ch] = _bandInfo.channels[ch].chanFreq;
		chanWidthVec[ch] = _bandInfo.channels[ch].chanWidth;
		effectiveBWVec[ch] = _bandInfo.channels[ch].effectiveBW;
		resolutionVec[ch] = _bandInfo.channels[ch].resolution;
	}
	chanFreqCol.put(rowIndex, chanFreqVec);
	chanWidthCol.put(rowIndex, chanWidthVec);
	measFreqRefCol.put(rowIndex, 5); // 5 means "TOPO"
	effectiveBWCol.put(rowIndex, effectiveBWVec);
	resolutionCol.put(rowIndex, resolutionVec);
	
	totalBWCol.put(rowIndex, _bandInfo.totalBandwidth);
	flagRowCol.put(rowIndex, _bandInfo.flagRow);
	
	writeDataDescEntry(rowIndex, 0, false);
}

void MSWriter::writeAntennae()
{
	MeasurementSet &ms = _data->_ms;
	MSAntenna antTable = ms.antenna();
	ScalarColumn<casacore::String> nameCol = ScalarColumn<casacore::String>(antTable, antTable.columnName(MSAntennaEnums::NAME));
	ScalarColumn<casacore::String> stationCol = ScalarColumn<casacore::String>(antTable, antTable.columnName(MSAntennaEnums::STATION));
	ScalarColumn<casacore::String> typeCol = ScalarColumn<casacore::String>(antTable, antTable.columnName(MSAntennaEnums::TYPE));
	ScalarColumn<casacore::String> mountCol = ScalarColumn<casacore::String>(antTable, antTable.columnName(MSAntennaEnums::MOUNT));
	ArrayColumn<double> positionCol = ArrayColumn<double>(antTable, antTable.columnName(MSAntennaEnums::POSITION));
	ScalarColumn<double> dishDiameterCol = ScalarColumn<double>(antTable, antTable.columnName(MSAntennaEnums::DISH_DIAMETER));
	
	size_t rowIndex = antTable.nrow();
	antTable.addRow(_antennae.size());
	for(std::vector<AntennaInfo>::const_iterator antPtr=_antennae.begin(); antPtr!=_antennae.end(); ++antPtr)
	{
		nameCol.put(rowIndex, antPtr->name);
		stationCol.put(rowIndex, antPtr->station);
		typeCol.put(rowIndex, antPtr->type);
		mountCol.put(rowIndex, antPtr->mount);
		Vector<double> posArr(3);
		posArr[0] = antPtr->x; posArr[1] = antPtr->y; posArr[2] = antPtr->z;
		positionCol.put(rowIndex, posArr);
		dishDiameterCol.put(rowIndex, antPtr->diameter);
		++rowIndex;
	}
	
	writeFeedEntries();
}

void MSWriter::writePolarizationForLinearPols()
{
	MeasurementSet &ms = _data->_ms;
	MSPolarization polTable = ms.polarization();
	ScalarColumn<int> numCorrCol = ScalarColumn<int>(polTable, polTable.columnName(MSPolarizationEnums::NUM_CORR));
	ArrayColumn<int> corrTypeCol = ArrayColumn<int>(polTable, polTable.columnName(MSPolarizationEnums::CORR_TYPE));
	ArrayColumn<int> corrProductCol = ArrayColumn<int>(polTable, polTable.columnName(MSPolarizationEnums::CORR_PRODUCT));
	ScalarColumn<bool> flagRowCol = ScalarColumn<bool>(polTable, polTable.columnName(MSPolarizationEnums::FLAG_ROW));
	
	size_t rowIndex = polTable.nrow();
	polTable.addRow(1);
	numCorrCol.put(rowIndex, 4);
	
	casacore::Vector<int> cTypeVec(4);
	cTypeVec[0] = 9; cTypeVec[1] = 10; cTypeVec[2] = 11; cTypeVec[3] = 12;
	corrTypeCol.put(rowIndex, cTypeVec);
	
	casacore::Array<int> cProdArr(IPosition(2, 2, 4));
	casacore::Array<int>::iterator i=cProdArr.begin();
	*i = 0; ++i; *i = 0; ++i;
	*i = 0; ++i; *i = 1; ++i;
	*i = 1; ++i; *i = 0; ++i;
	*i = 1; ++i; *i = 1;
	corrProductCol.put(rowIndex, cProdArr);
	
	flagRowCol.put(rowIndex, false);
}

void MSWriter::writeSource()
{
	MeasurementSet &ms = _data->_ms;
	MSSource sourceTable = ms.keywordSet().asTable(MS::keywordName(MS::SOURCE));
	ScalarColumn<int> sourceIdCol = ScalarColumn<int>(sourceTable, MSSource::columnName(MSSourceEnums::SOURCE_ID));
	ScalarColumn<double> timeCol = ScalarColumn<double>(sourceTable, MSSource::columnName(MSSourceEnums::TIME));
	ScalarColumn<double> intervalCol = ScalarColumn<double>(sourceTable, MSSource::columnName(MSSourceEnums::INTERVAL));
	ScalarColumn<int> spectralWindowIdCol = ScalarColumn<int>(sourceTable, MSSource::columnName(MSSourceEnums::SPECTRAL_WINDOW_ID));
	ScalarColumn<int> numLinesCol = ScalarColumn<int>(sourceTable, MSSource::columnName(MSSourceEnums::NUM_LINES));
	ScalarColumn<casacore::String> nameCol = ScalarColumn<casacore::String>(sourceTable, MSSource::columnName(MSSourceEnums::NAME));
	ScalarColumn<int> calibrationGroupCol = ScalarColumn<int>(sourceTable, MSSource::columnName(MSSourceEnums::CALIBRATION_GROUP));
	ScalarColumn<casacore::String> codeCol = ScalarColumn<casacore::String>(sourceTable, MSSource::columnName(MSSourceEnums::CODE));
	ArrayColumn<double> directionCol = ArrayColumn<double>(sourceTable, MSSource::columnName(MSSourceEnums::DIRECTION));
	ArrayColumn<double> properMotionCol = ArrayColumn<double>(sourceTable, MSSource::columnName(MSSourceEnums::PROPER_MOTION));
	
	size_t rowIndex = sourceTable.nrow();
	sourceTable.addRow();
	
	sourceIdCol.put(rowIndex, _source.sourceId);
	timeCol.put(rowIndex, _source.time);
	intervalCol.put(rowIndex, _source.interval);
	spectralWindowIdCol.put(rowIndex, _source.spectralWindowId);
	numLinesCol.put(rowIndex, _source.numLines);
	nameCol.put(rowIndex, _source.name);
	calibrationGroupCol.put(rowIndex, _source.calibrationGroup);
	codeCol.put(rowIndex, _source.code);
	casacore::Vector<double> direction(2);
	direction[0] = _source.directionRA; direction[1] = _source.directionDec;
	directionCol.put(rowIndex, direction);
	casacore::Vector<double> properMotion(2);
	properMotion[0] = _source.properMotion[0]; properMotion[1] = _source.properMotion[1];
	properMotionCol.put(rowIndex, properMotion);
}

void MSWriter::writeField()
{
	MeasurementSet &ms = _data->_ms;
	MSField fieldTable = ms.field();
	ScalarColumn<casacore::String> nameCol = ScalarColumn<casacore::String>(fieldTable, fieldTable.columnName(MSFieldEnums::NAME));
	ScalarColumn<casacore::String> codeCol = ScalarColumn<casacore::String>(fieldTable, fieldTable.columnName(MSFieldEnums::CODE));
	ScalarColumn<double> timeCol = ScalarColumn<double>(fieldTable, fieldTable.columnName(MSFieldEnums::TIME));
	ScalarColumn<int> numPolyCol = ScalarColumn<int>(fieldTable, fieldTable.columnName(MSFieldEnums::NUM_POLY));
	ArrayColumn<double> delayDirCol = ArrayColumn<double>(fieldTable, fieldTable.columnName(MSFieldEnums::DELAY_DIR));
	ArrayColumn<double> phaseDirCol = ArrayColumn<double>(fieldTable, fieldTable.columnName(MSFieldEnums::PHASE_DIR));
	ArrayColumn<double> refDirCol = ArrayColumn<double>(fieldTable, fieldTable.columnName(MSFieldEnums::REFERENCE_DIR));
	ScalarColumn<int> sourceIdCol = ScalarColumn<int>(fieldTable, fieldTable.columnName(MSFieldEnums::SOURCE_ID));
	ScalarColumn<bool> flagRowCol = ScalarColumn<bool>(fieldTable, fieldTable.columnName(MSFieldEnums::FLAG_ROW));
	
	size_t index = fieldTable.nrow();
	fieldTable.addRow();
	
	nameCol.put(index, _field.name);
	codeCol.put(index, _field.code);
	timeCol.put(index, _field.time);
	numPolyCol.put(index, _field.numPoly);
	
	casacore::Array<double> arr(IPosition(2, 2, 1));
	casacore::Array<double>::iterator raPtr = arr.begin(), decPtr = raPtr; decPtr++;
	*raPtr = _field.delayDirRA; *decPtr = _field.delayDirDec;
	delayDirCol.put(index, arr);
	
	*raPtr = _field.phaseDirRA; *decPtr = _field.phaseDirDec;
	phaseDirCol.put(index, arr);
	
	*raPtr = _field.referenceDirRA; *decPtr = _field.referenceDirDec;
	refDirCol.put(index, arr);
	
	sourceIdCol.put(index, _field.sourceId);
	flagRowCol.put(index, _field.flagRow);
}

void MSWriter::writeDataDescEntry(size_t spectralWindowId, size_t polarizationId, bool flagRow)
{
	MeasurementSet &ms = _data->_ms;
	MSDataDescription dataDescTable = ms.dataDescription();
	ScalarColumn<int> spectWindowIdCol(dataDescTable, dataDescTable.columnName(MSDataDescriptionEnums::SPECTRAL_WINDOW_ID));
	ScalarColumn<int> polIdCol(dataDescTable, dataDescTable.columnName(MSDataDescriptionEnums::POLARIZATION_ID));
	ScalarColumn<bool> flagRowCol(dataDescTable, dataDescTable.columnName(MSDataDescriptionEnums::FLAG_ROW));
	
	size_t index = dataDescTable.nrow();
	dataDescTable.addRow();
	
	spectWindowIdCol.put(index, spectralWindowId);
	polIdCol.put(index, polarizationId);
	flagRowCol.put(index, flagRow);
}

void MSWriter::writeFeedEntries()
{
	MeasurementSet &ms = _data->_ms;
	MSFeed feedTable = ms.feed();
	ScalarColumn<int> antennaIdCol(feedTable, feedTable.columnName(MSFeedEnums::ANTENNA_ID));
	ScalarColumn<int> feedIdCol(feedTable, feedTable.columnName(MSFeedEnums::FEED_ID));
	ScalarColumn<int> spectralWindowIdCol(feedTable, feedTable.columnName(MSFeedEnums::SPECTRAL_WINDOW_ID));
	ScalarColumn<double> timeCol(feedTable, feedTable.columnName(MSFeedEnums::TIME));
	ScalarColumn<int> numReceptorsCol(feedTable, feedTable.columnName(MSFeedEnums::NUM_RECEPTORS));
	ScalarColumn<int> beamIdCol(feedTable, feedTable.columnName(MSFeedEnums::BEAM_ID));
	ArrayColumn<double> beamOffsetCol(feedTable, feedTable.columnName(MSFeedEnums::BEAM_OFFSET));
	ArrayColumn<casacore::String> polarizationTypeCol(feedTable, feedTable.columnName(MSFeedEnums::POLARIZATION_TYPE));
	ArrayColumn<std::complex<float> > polResponseCol(feedTable, feedTable.columnName(MSFeedEnums::POL_RESPONSE));
	ArrayColumn<double> positionCol(feedTable, feedTable.columnName(MSFeedEnums::POSITION));
	ArrayColumn<double> receptorAngleCol(feedTable, feedTable.columnName(MSFeedEnums::RECEPTOR_ANGLE));
	
	size_t rowIndex = feedTable.nrow();
	feedTable.addRow(_antennae.size());
	for(size_t antIndex=0; antIndex!=_antennae.size(); ++antIndex)
	{
		antennaIdCol.put(rowIndex, antIndex);
		feedIdCol.put(rowIndex, 0);
		spectralWindowIdCol.put(rowIndex, -1);
		timeCol.put(rowIndex, _antennaDate);
		numReceptorsCol.put(rowIndex, 2);
		beamIdCol.put(rowIndex, -1);
		
		casacore::Array<double> beamOffset(IPosition(2, 2, 2));
		casacore::Array<double>::iterator i = beamOffset.begin();
		*i = 0.0; ++i; *i = 0.0; ++i; *i = 0.0; ++i; *i = 0.0;
		beamOffsetCol.put(rowIndex, beamOffset);
		
		casacore::Vector<casacore::String> polType(2);
		polType[0] = 'X'; polType[1] = 'Y';
		polarizationTypeCol.put(rowIndex, polType);
		
		casacore::Array<std::complex<float> > polResponse(IPosition(2, 2, 2));
		casacore::Array<std::complex<float> >::iterator piter = polResponse.begin();
		*piter = 1.0; ++piter; *piter = 0.0; ++piter; *piter = 0.0; ++piter; *piter = 1.0;
		polResponseCol.put(rowIndex, polResponse);
		
		casacore::Vector<double> position(3);
		position[0] = 0.0; position[1] = 0.0; position[2] = 0.0;
		positionCol.put(rowIndex, position);
		
		casacore::Vector<double> receptorAngle(2);
		receptorAngle[0] = 0.0; receptorAngle[1] = M_PI*0.5;
		receptorAngleCol.put(rowIndex, receptorAngle);
		
		++rowIndex;
	}
}

void MSWriter::writeObservation()
{
	MeasurementSet &ms = _data->_ms;
	MSObservation obsTable = ms.observation();
	ScalarColumn<casacore::String> telescopeNameCol(obsTable, obsTable.columnName(MSObservationEnums::TELESCOPE_NAME));
	ArrayColumn<double> timeRangeCol(obsTable, obsTable.columnName(MSObservationEnums::TIME_RANGE));
	ScalarColumn<casacore::String> observerCol(obsTable, obsTable.columnName(MSObservationEnums::OBSERVER));
	ScalarColumn<casacore::String> scheduleTypeCol(obsTable, obsTable.columnName(MSObservationEnums::SCHEDULE_TYPE));
	ScalarColumn<casacore::String> projectCol(obsTable, obsTable.columnName(MSObservationEnums::PROJECT));
	ScalarColumn<double> releaseDateCol(obsTable, obsTable.columnName(MSObservationEnums::RELEASE_DATE));
	ScalarColumn<bool> flagRowCol(obsTable, obsTable.columnName(MSObservationEnums::FLAG_ROW));
	
	size_t rowIndex = obsTable.nrow();
	obsTable.addRow();
	
	telescopeNameCol.put(rowIndex, _observation.telescopeName);
	casacore::Vector<double> timeRange(2);
	timeRange[0] = _observation.startTime; timeRange[1] = _observation.endTime;
	timeRangeCol.put(rowIndex, timeRange);
	observerCol.put(rowIndex, _observation.observer);
	scheduleTypeCol.put(rowIndex, _observation.scheduleType);
	projectCol.put(rowIndex, _observation.project);
	releaseDateCol.put(rowIndex, _observation.releaseDate);
	flagRowCol.put(rowIndex, _observation.flagRow);
}

void MSWriter::AddRows(size_t count)
{
	if(!_isInitialized)
		initialize();
	_data->_ms.addRow(count);
}

void MSWriter::WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, const std::complex<float>* data, const bool* flags, const float *weights)
{
	_data->_timeCol.put(_rowIndex, time);
	_data->_timeCentroidCol.put(_rowIndex, timeCentroid);
	_data->_antenna1Col.put(_rowIndex, antenna1);
	_data->_antenna2Col.put(_rowIndex, antenna2);
	_data->_dataDescIdCol.put(_rowIndex, 0);
	
	casacore::Vector<double> uvwVec(3);
	uvwVec[0] = u; uvwVec[1] = v; uvwVec[2] = w;
	_data->_uvwCol.put(_rowIndex, uvwVec);
	
	_data->_intervalCol.put(_rowIndex, interval);
	_data->_exposureCol.put(_rowIndex, interval);
	_data->_processorIdCol.put(_rowIndex, -1);
	_data->_scanNumberCol.put(_rowIndex, 1);
	_data->_stateIdCol.put(_rowIndex, -1);
	
	size_t nPol = 4;
	
	casacore::Vector<float> sigmaArr(nPol);
	for(size_t p=0; p!=nPol; ++p) sigmaArr[p] = 1.0;
	_data->_sigmaCol.put(_rowIndex, sigmaArr);
	
	size_t valCount = _bandInfo.channels.size() * nPol;
	casacore::IPosition shape(2, nPol, _bandInfo.channels.size());
	casacore::Array<std::complex<float> > dataArr(shape);
	casacore::Array<bool> flagArr(shape);
	casacore::Array<float> weightSpectrumArr(shape);
	
	// Fill the casa arrays
	casacore::Array<std::complex<float> >::contiter dataPtr = dataArr.cbegin();
	casacore::Array<bool>::contiter flagPtr = flagArr.cbegin();
	casacore::Array<float>::contiter weightSpectrumPtr = weightSpectrumArr.cbegin();
	for(size_t i=0; i!=valCount; ++i)
	{
		*dataPtr = data[i]; ++dataPtr;
		*flagPtr = flags[i]; ++flagPtr;
		*weightSpectrumPtr = weights[i]; ++weightSpectrumPtr;
	}
	
	casacore::Vector<float> weightsArr(nPol);
	for(size_t p=0; p!=nPol; ++p) weightsArr[p] = 0.0;
	for(size_t ch=0; ch!=_bandInfo.channels.size(); ++ch)
	{
		for(size_t p=0; p!=nPol; ++p)
			weightsArr[p] += weights[ch*nPol + p];
	}
	
	_data->_dataCol.put(_rowIndex, dataArr);
	_data->_flagCol.put(_rowIndex, flagArr);
	_data->_weightCol.put(_rowIndex, weightsArr);
	_data->_weightSpectrumCol.put(_rowIndex, weightSpectrumArr);
	
	++_rowIndex;
}

void MSWriter::writeHistoryItem()
{
	MeasurementSet &ms = _data->_ms;
	MSHistory hisTable = ms.history();
	casacore::ScalarColumn<double> timeCol(hisTable, MSHistory::columnName(MSHistoryEnums::TIME));
	casacore::ScalarColumn<int> obsIdCol(hisTable, MSHistory::columnName(MSHistoryEnums::OBSERVATION_ID));
	casacore::ScalarColumn<casacore::String> messageCol(hisTable, MSHistory::columnName(MSHistoryEnums::MESSAGE));
	casacore::ScalarColumn<casacore::String> applicationCol(hisTable, MSHistory::columnName(MSHistoryEnums::APPLICATION));
	casacore::ScalarColumn<casacore::String> priorityCol(hisTable, MSHistory::columnName(MSHistoryEnums::PRIORITY));
	casacore::ScalarColumn<casacore::String> originCol(hisTable, MSHistory::columnName(MSHistoryEnums::ORIGIN));
	casacore::ArrayColumn<casacore::String> parmsCol(hisTable, MSHistory::columnName(MSHistoryEnums::APP_PARAMS));
	casacore::ArrayColumn<casacore::String> cliCol(hisTable, MSHistory::columnName(MSHistoryEnums::CLI_COMMAND));

	casacore::Vector<casacore::String> appParamsVec(_historyParams.size());
	for(size_t i=0; i!=_historyParams.size(); ++i)
		appParamsVec[i] = _historyParams[i];
	casacore::Vector<casacore::String> cliVec(1);
	cliVec[0] = _historyCommandLine;
	
	size_t rowIndex = hisTable.nrow();
	hisTable.addRow();
	timeCol.put(rowIndex, casacore::Time().modifiedJulianDay()*24.0*3600.0);
	obsIdCol.put(rowIndex, 0);
	messageCol.put(rowIndex, "Preprocessed & AOFlagged");
	applicationCol.put(rowIndex, _historyApplication);
	priorityCol.put(rowIndex, "NORMAL");
	originCol.put(rowIndex, "standalone");
	parmsCol.put(rowIndex, appParamsVec);
	cliCol.put(rowIndex, cliVec);
}
