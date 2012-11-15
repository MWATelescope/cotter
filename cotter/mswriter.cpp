#include "mswriter.h"

#include <ms/MeasurementSets/MeasurementSet.h>

#include <tables/Tables/ArrayColumn.h>
#include <tables/Tables/ArrColDesc.h>
#include <tables/Tables/ScalarColumn.h>
#include <tables/Tables/SetupNewTab.h>

using namespace casa;

class MSWriterData
{
	public:
		MeasurementSet *ms;
		
		ScalarColumn<double> *_timeCol, *_timeCentroidCol;
		ScalarColumn<int> *_antenna1Col, *_antenna2Col;
		ScalarColumn<int> *_dataDescIdCol;
		ArrayColumn<double> *_uvwCol;
		ScalarColumn<double> *_intervalCol, *_exposureCol;
		ScalarColumn<int> *_processorIdCol, *_scanNumberCol, *_stateIdCol;
		ArrayColumn<std::complex<float> > *_dataCol;
		ArrayColumn<bool> *_flagCol;
		ArrayColumn<float> *_sigmaCol;
		ArrayColumn<float> *_weightCol;
		ArrayColumn<float> *_weightSpectrumCol;
};

MSWriter::MSWriter(const char* filename) :
	_data(new MSWriterData),
	_rowIndex(0),
	_nChannels(0)
{
	TableDesc tableDesc = MS::requiredTableDesc();
	ArrayColumnDesc<std::complex<float> > dataColumnDesc = ArrayColumnDesc<std::complex<float> >(MS::columnName(casa::MSMainEnums::DATA));
	tableDesc.addColumn(dataColumnDesc);
	ArrayColumnDesc<float> weightSpectrumColumnDesc = ArrayColumnDesc<float>(MS::columnName(casa::MSMainEnums::WEIGHT_SPECTRUM));
	tableDesc.addColumn(weightSpectrumColumnDesc);
	SetupNewTable newTab(filename, tableDesc, Table::New);
	_data->ms = new MeasurementSet(newTab);
	MeasurementSet &ms = *_data->ms;
	ms.createDefaultSubtables(Table::New);
	
	_data->_timeCol = new ScalarColumn<double>(ms, MS::columnName(casa::MSMainEnums::TIME));
	_data->_timeCentroidCol = new ScalarColumn<double>(ms, MS::columnName(casa::MSMainEnums::TIME_CENTROID));
	_data->_antenna1Col = new ScalarColumn<int>(ms, MS::columnName(casa::MSMainEnums::ANTENNA1));
	_data->_antenna2Col = new ScalarColumn<int>(ms, MS::columnName(casa::MSMainEnums::ANTENNA2));
	_data->_dataDescIdCol = new ScalarColumn<int>(ms, MS::columnName(casa::MSMainEnums::DATA_DESC_ID));
	_data->_uvwCol = new ArrayColumn<double>(ms, MS::columnName(casa::MSMainEnums::UVW));
	_data->_intervalCol = new ScalarColumn<double>(ms, MS::columnName(casa::MSMainEnums::INTERVAL));
	_data->_exposureCol = new ScalarColumn<double>(ms, MS::columnName(casa::MSMainEnums::EXPOSURE));
	_data->_processorIdCol = new ScalarColumn<int>(ms, MS::columnName(casa::MSMainEnums::PROCESSOR_ID));
	_data->_scanNumberCol = new ScalarColumn<int>(ms, MS::columnName(casa::MSMainEnums::SCAN_NUMBER));
	_data->_stateIdCol = new ScalarColumn<int>(ms, MS::columnName(casa::MSMainEnums::STATE_ID));
	_data->_dataCol = new ArrayColumn<std::complex<float> >(ms, MS::columnName(casa::MSMainEnums::DATA));
	_data->_sigmaCol = new ArrayColumn<float>(ms, MS::columnName(casa::MSMainEnums::SIGMA));
	_data->_weightCol = new ArrayColumn<float>(ms, MS::columnName(casa::MSMainEnums::WEIGHT));
	_data->_weightSpectrumCol = new ArrayColumn<float>(ms, MS::columnName(casa::MSMainEnums::WEIGHT_SPECTRUM));
	_data->_flagCol = new ArrayColumn<bool>(ms, MS::columnName(casa::MSMainEnums::FLAG));
}

MSWriter::~MSWriter()
{
	delete _data->_timeCol;
	delete _data->_timeCentroidCol;
	delete _data->_antenna1Col;
	delete _data->_antenna2Col;
	delete _data->_dataDescIdCol;
	delete _data->_uvwCol;
	delete _data->_intervalCol;
	delete _data->_exposureCol;
	delete _data->_processorIdCol;
	delete _data->_scanNumberCol;
	delete _data->_stateIdCol;
	delete _data->_dataCol;
	delete _data->_sigmaCol;
	delete _data->_weightCol;
	delete _data->_weightSpectrumCol;
	delete _data->_flagCol;
	delete _data->ms;
	delete _data;
}

void MSWriter::WriteBandInfo(const std::string &name, const std::vector<ChannelInfo> &channels, double refFreq, double totalBandwidth, bool flagRow)
{
	_nChannels = channels.size();
	MeasurementSet &ms = *_data->ms;
	MSSpectralWindow spwTable = ms.spectralWindow();
	
	ScalarColumn<int> numChanCol = ScalarColumn<int>(spwTable, spwTable.columnName(MSSpectralWindowEnums::NUM_CHAN));
	ScalarColumn<casa::String> nameCol = ScalarColumn<casa::String>(spwTable, spwTable.columnName(MSSpectralWindowEnums::NAME));
	ScalarColumn<double> refFreqCol = ScalarColumn<double>(spwTable, spwTable.columnName(MSSpectralWindowEnums::REF_FREQUENCY));
	
	ArrayColumn<double> chanFreqCol = ArrayColumn<double>(spwTable, spwTable.columnName(MSSpectralWindowEnums::CHAN_FREQ));
	ArrayColumn<double> chanWidthCol = ArrayColumn<double>(spwTable, spwTable.columnName(MSSpectralWindowEnums::CHAN_WIDTH));
	ArrayColumn<double> effectiveBWCol = ArrayColumn<double>(spwTable, spwTable.columnName(MSSpectralWindowEnums::EFFECTIVE_BW));
	ArrayColumn<double> resolutionCol = ArrayColumn<double>(spwTable, spwTable.columnName(MSSpectralWindowEnums::RESOLUTION));
	ScalarColumn<double> totalBWCol = ScalarColumn<double>(spwTable, spwTable.columnName(MSSpectralWindowEnums::TOTAL_BANDWIDTH));
	ScalarColumn<bool> flagRowCol = ScalarColumn<bool>(spwTable, spwTable.columnName(MSSpectralWindowEnums::FLAG_ROW));

	size_t rowIndex = spwTable.nrow();
	spwTable.addRow();
	numChanCol.put(rowIndex, _nChannels);
	nameCol.put(rowIndex, name);
	refFreqCol.put(rowIndex, refFreq);
	
	casa::Vector<double>
		chanFreqVec(_nChannels), chanWidthVec(_nChannels),
		effectiveBWVec(_nChannels), resolutionVec(_nChannels);
	for(size_t ch=0; ch!=channels.size(); ++ch)
	{
		chanFreqVec[ch] = channels[ch].chanFreq;
		chanWidthVec[ch] = channels[ch].chanWidth;
		effectiveBWVec[ch] = channels[ch].effectiveBW;
		resolutionVec[ch] = channels[ch].resolution;
	}
	chanFreqCol.put(rowIndex, chanFreqVec);
	chanWidthCol.put(rowIndex, chanWidthVec);
	effectiveBWCol.put(rowIndex, effectiveBWVec);
	resolutionCol.put(rowIndex, resolutionVec);
	
	totalBWCol.put(rowIndex, totalBandwidth);
	flagRowCol.put(rowIndex, flagRow);
	
	writeDataDescEntry(rowIndex, 0, false);
}

void MSWriter::WriteAntennae(const std::vector<AntennaInfo> &antennae, double time)
{
	MeasurementSet &ms = *_data->ms;
	MSAntenna antTable = ms.antenna();
	ScalarColumn<casa::String> nameCol = ScalarColumn<casa::String>(antTable, antTable.columnName(MSAntennaEnums::NAME));
	ScalarColumn<casa::String> stationCol = ScalarColumn<casa::String>(antTable, antTable.columnName(MSAntennaEnums::STATION));
	ScalarColumn<casa::String> typeCol = ScalarColumn<casa::String>(antTable, antTable.columnName(MSAntennaEnums::TYPE));
	ScalarColumn<casa::String> mountCol = ScalarColumn<casa::String>(antTable, antTable.columnName(MSAntennaEnums::MOUNT));
	ArrayColumn<double> positionCol = ArrayColumn<double>(antTable, antTable.columnName(MSAntennaEnums::POSITION));
	ScalarColumn<double> dishDiameterCol = ScalarColumn<double>(antTable, antTable.columnName(MSAntennaEnums::DISH_DIAMETER));
	
	size_t rowIndex = antTable.nrow();
	antTable.addRow(antennae.size());
	for(std::vector<AntennaInfo>::const_iterator antPtr=antennae.begin(); antPtr!=antennae.end(); ++antPtr)
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
	
	writeFeedEntries(antennae, time);
}

void MSWriter::WritePolarizationForLinearPols(bool flagRow)
{
	MeasurementSet &ms = *_data->ms;
	MSPolarization polTable = ms.polarization();
	ScalarColumn<int> numCorrCol = ScalarColumn<int>(polTable, polTable.columnName(MSPolarizationEnums::NUM_CORR));
	ArrayColumn<int> corrTypeCol = ArrayColumn<int>(polTable, polTable.columnName(MSPolarizationEnums::CORR_TYPE));
	ArrayColumn<int> corrProductCol = ArrayColumn<int>(polTable, polTable.columnName(MSPolarizationEnums::CORR_PRODUCT));
	ScalarColumn<bool> flagRowCol = ScalarColumn<bool>(polTable, polTable.columnName(MSPolarizationEnums::FLAG_ROW));
	
	size_t rowIndex = polTable.nrow();
	polTable.addRow(1);
	numCorrCol.put(rowIndex, 4);
	
	casa::Vector<int> cTypeVec(4);
	cTypeVec[0] = 9; cTypeVec[1] = 10; cTypeVec[2] = 11; cTypeVec[3] = 12;
	corrTypeCol.put(rowIndex, cTypeVec);
	
	casa::Array<int> cProdArr(IPosition(2, 2, 4));
	casa::Array<int>::iterator i=cProdArr.begin();
	*i = 0; ++i; *i = 0; ++i;
	*i = 0; ++i; *i = 1; ++i;
	*i = 1; ++i; *i = 0; ++i;
	*i = 1; ++i; *i = 1;
	corrProductCol.put(rowIndex, cProdArr);
	
	flagRowCol.put(rowIndex, flagRow);
}

void MSWriter::WriteField(const FieldInfo& field)
{
	MeasurementSet &ms = *_data->ms;
	MSField fieldTable = ms.field();
	ScalarColumn<casa::String> nameCol = ScalarColumn<casa::String>(fieldTable, fieldTable.columnName(MSFieldEnums::NAME));
	ScalarColumn<casa::String> codeCol = ScalarColumn<casa::String>(fieldTable, fieldTable.columnName(MSFieldEnums::CODE));
	ScalarColumn<double> timeCol = ScalarColumn<double>(fieldTable, fieldTable.columnName(MSFieldEnums::TIME));
	ScalarColumn<int> numPolyCol = ScalarColumn<int>(fieldTable, fieldTable.columnName(MSFieldEnums::NUM_POLY));
	ArrayColumn<double> delayDirCol = ArrayColumn<double>(fieldTable, fieldTable.columnName(MSFieldEnums::DELAY_DIR));
	ArrayColumn<double> phaseDirCol = ArrayColumn<double>(fieldTable, fieldTable.columnName(MSFieldEnums::PHASE_DIR));
	ArrayColumn<double> refDirCol = ArrayColumn<double>(fieldTable, fieldTable.columnName(MSFieldEnums::REFERENCE_DIR));
	ScalarColumn<int> sourceIdCol = ScalarColumn<int>(fieldTable, fieldTable.columnName(MSFieldEnums::SOURCE_ID));
	ScalarColumn<bool> flagRowCol = ScalarColumn<bool>(fieldTable, fieldTable.columnName(MSFieldEnums::FLAG_ROW));
	
	size_t index = fieldTable.nrow();
	fieldTable.addRow();
	
	nameCol.put(index, field.name);
	codeCol.put(index, field.code);
	timeCol.put(index, field.time);
	numPolyCol.put(index, field.numPoly);
	
	casa::Array<double> arr(IPosition(2, 2, 1));
	casa::Array<double>::iterator raPtr = arr.begin(), decPtr = raPtr; decPtr++;
	*raPtr = field.delayDirRA; *decPtr = field.delayDirDec;
	delayDirCol.put(index, arr);
	
	*raPtr = field.phaseDirRA; *decPtr = field.phaseDirDec;
	phaseDirCol.put(index, arr);
	
	*raPtr = field.referenceDirRA; *decPtr = field.referenceDirDec;
	refDirCol.put(index, arr);
	
	sourceIdCol.put(index, field.sourceId);
	flagRowCol.put(index, field.flagRow);
}

void MSWriter::writeDataDescEntry(size_t spectralWindowId, size_t polarizationId, bool flagRow)
{
	MeasurementSet &ms = *_data->ms;
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

void MSWriter::writeFeedEntries(const std::vector<Writer::AntennaInfo> &antennae, double time)
{
	MeasurementSet &ms = *_data->ms;
	MSFeed feedTable = ms.feed();
	ScalarColumn<int> antennaIdCol(feedTable, feedTable.columnName(MSFeedEnums::ANTENNA_ID));
	ScalarColumn<int> feedIdCol(feedTable, feedTable.columnName(MSFeedEnums::FEED_ID));
	ScalarColumn<int> spectralWindowIdCol(feedTable, feedTable.columnName(MSFeedEnums::SPECTRAL_WINDOW_ID));
	ScalarColumn<double> timeCol(feedTable, feedTable.columnName(MSFeedEnums::TIME));
	ScalarColumn<int> numReceptorsCol(feedTable, feedTable.columnName(MSFeedEnums::NUM_RECEPTORS));
	ScalarColumn<int> beamIdCol(feedTable, feedTable.columnName(MSFeedEnums::BEAM_ID));
	ArrayColumn<double> beamOffsetCol(feedTable, feedTable.columnName(MSFeedEnums::BEAM_OFFSET));
	ArrayColumn<casa::String> polarizationTypeCol(feedTable, feedTable.columnName(MSFeedEnums::POLARIZATION_TYPE));
	ArrayColumn<std::complex<float> > polResponseCol(feedTable, feedTable.columnName(MSFeedEnums::POL_RESPONSE));
	ArrayColumn<double> positionCol(feedTable, feedTable.columnName(MSFeedEnums::POSITION));
	ArrayColumn<double> receptorAngleCol(feedTable, feedTable.columnName(MSFeedEnums::RECEPTOR_ANGLE));
	
	size_t rowIndex = feedTable.nrow();
	feedTable.addRow(antennae.size());
	for(size_t antIndex=0; antIndex!=antennae.size(); ++antIndex)
	{
		antennaIdCol.put(rowIndex, antIndex);
		feedIdCol.put(rowIndex, 0);
		spectralWindowIdCol.put(rowIndex, -1);
		timeCol.put(rowIndex, time);
		numReceptorsCol.put(rowIndex, 2);
		beamIdCol.put(rowIndex, -1);
		
		casa::Array<double> beamOffset(IPosition(2, 2, 2));
		casa::Array<double>::iterator i = beamOffset.begin();
		*i = 0.0; ++i; *i = 0.0; ++i; *i = 0.0; ++i; *i = 0.0;
		beamOffsetCol.put(rowIndex, beamOffset);
		
		casa::Vector<casa::String> polType(2);
		polType[0] = 'X'; polType[1] = 'Y';
		polarizationTypeCol.put(rowIndex, polType);
		
		casa::Array<std::complex<float> > polResponse(IPosition(2, 2, 2));
		casa::Array<std::complex<float> >::iterator piter = polResponse.begin();
		*piter = 1.0; ++piter; *piter = 0.0; ++piter; *piter = 0.0; ++piter; *piter = 1.0;
		polResponseCol.put(rowIndex, polResponse);
		
		casa::Vector<double> position(3);
		position[0] = 0.0; position[1] = 0.0; position[2] = 0.0;
		positionCol.put(rowIndex, position);
		
		casa::Vector<double> receptorAngle(2);
		receptorAngle[0] = 0.0; receptorAngle[1] = M_PI*0.5;
		receptorAngleCol.put(rowIndex, receptorAngle);
		
		++rowIndex;
	}
}

void MSWriter::WriteObservation(const std::string& telescopeName, double startTime, double endTime, const std::string& observer, const std::string& scheduleType, const std::string& project, double releaseDate, bool flagRow)
{
	MeasurementSet &ms = *_data->ms;
	MSObservation obsTable = ms.observation();
	ScalarColumn<casa::String> telescopeNameCol(obsTable, obsTable.columnName(MSObservationEnums::TELESCOPE_NAME));
	ArrayColumn<double> timeRangeCol(obsTable, obsTable.columnName(MSObservationEnums::TIME_RANGE));
	ScalarColumn<casa::String> observerCol(obsTable, obsTable.columnName(MSObservationEnums::OBSERVER));
	ScalarColumn<casa::String> scheduleTypeCol(obsTable, obsTable.columnName(MSObservationEnums::SCHEDULE_TYPE));
	ScalarColumn<casa::String> projectCol(obsTable, obsTable.columnName(MSObservationEnums::PROJECT));
	ScalarColumn<double> releaseDateCol(obsTable, obsTable.columnName(MSObservationEnums::RELEASE_DATE));
	ScalarColumn<bool> flagRowCol(obsTable, obsTable.columnName(MSObservationEnums::FLAG_ROW));
	
	size_t rowIndex = obsTable.nrow();
	obsTable.addRow();
	
	telescopeNameCol.put(rowIndex, telescopeName);
	casa::Vector<double> timeRange(2);
	timeRange[0] = startTime; timeRange[1] = endTime;
	timeRangeCol.put(rowIndex, timeRange);
	observerCol.put(rowIndex, observer);
	scheduleTypeCol.put(rowIndex, scheduleType);
	projectCol.put(rowIndex, project);
	releaseDateCol.put(rowIndex, releaseDate);
	flagRowCol.put(rowIndex, flagRow);
}

void MSWriter::AddRows(size_t count)
{
	_data->ms->addRow(count);
}

void MSWriter::WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, const std::complex<float>* data, const bool* flags, const float *weights)
{
	_data->_timeCol->put(_rowIndex, time);
	_data->_timeCentroidCol->put(_rowIndex, timeCentroid);
	_data->_antenna1Col->put(_rowIndex, antenna1);
	_data->_antenna2Col->put(_rowIndex, antenna2);
	_data->_dataDescIdCol->put(_rowIndex, 0);
	
	casa::Vector<double> uvwVec(3);
	uvwVec[0] = u; uvwVec[1] = v; uvwVec[2] = w;
	_data->_uvwCol->put(_rowIndex, uvwVec);
	
	_data->_intervalCol->put(_rowIndex, interval);
	_data->_exposureCol->put(_rowIndex, interval);
	_data->_processorIdCol->put(_rowIndex, -1);
	_data->_scanNumberCol->put(_rowIndex, 1);
	_data->_stateIdCol->put(_rowIndex, -1);
	
	size_t nPol = 4;
	
	casa::Vector<float> sigmaArr(nPol);
	for(size_t p=0; p!=nPol; ++p) sigmaArr[p] = 1.0;
	_data->_sigmaCol->put(_rowIndex, sigmaArr);
	
	size_t valCount = _nChannels * nPol;
	casa::IPosition shape(2, nPol, _nChannels);
	casa::Array<std::complex<float> > dataArr(shape);
	casa::Array<bool> flagArr(shape);
	casa::Array<float> weightSpectrumArr(shape);
	
	// Fill the casa arrays
	casa::Array<std::complex<float> >::iterator dataPtr = dataArr.begin();
	casa::Array<bool>::iterator flagPtr = flagArr.begin();
	casa::Array<float>::iterator weightSpectrumPtr = weightSpectrumArr.begin();
	for(size_t i=0; i!=valCount; ++i)
	{
		*dataPtr = data[i]; ++dataPtr;
		*flagPtr = flags[i]; ++flagPtr;
		*weightSpectrumPtr = weights[i]; ++weightSpectrumPtr;
	}
	
	casa::Vector<float> weightsArr(nPol);
	for(size_t p=0; p!=nPol; ++p) weightsArr[p] = 0.0;
	for(size_t ch=0; ch!=_nChannels; ++ch)
	{
		for(size_t p=0; p!=nPol; ++p)
			weightsArr[p] += weights[ch*nPol + p];
	}
	
	_data->_dataCol->put(_rowIndex, dataArr);
	_data->_flagCol->put(_rowIndex, flagArr);
	_data->_weightCol->put(_rowIndex, weightsArr);
	_data->_weightSpectrumCol->put(_rowIndex, weightSpectrumArr);
	
	++_rowIndex;
}

MSWriter::AntennaInfo::AntennaInfo(const MSWriter::AntennaInfo& source) :
	name(source.name), station(source.station),
	type(source.type), mount(source.mount),
	x(source.x), y(source.y), z(source.z),
	diameter(source.diameter),
	flag(source.flag)
{ 
}

MSWriter::AntennaInfo& MSWriter::AntennaInfo::operator=(const MSWriter::AntennaInfo& source)
{
	name = source.name; station = source.station;
	type = source.type; mount = source.mount;
	x = source.x; y = source.y; z = source.z;
	diameter = source.diameter;
	flag = source.flag;
	return *this;
}

void MSWriter::WriteHistoryItem(const std::string &commandLine, const std::string &application)
{
	MeasurementSet &ms = *_data->ms;
	MSHistory hisTable = ms.history();
	casa::ScalarColumn<double> timeCol(hisTable, MSHistory::columnName(MSHistoryEnums::TIME));
	casa::ScalarColumn<int> obsIdCol(hisTable, MSHistory::columnName(MSHistoryEnums::OBSERVATION_ID));
	casa::ScalarColumn<casa::String> messageCol(hisTable, MSHistory::columnName(MSHistoryEnums::MESSAGE));
	casa::ScalarColumn<casa::String> applicationCol(hisTable, MSHistory::columnName(MSHistoryEnums::APPLICATION));
	casa::ScalarColumn<casa::String> priorityCol(hisTable, MSHistory::columnName(MSHistoryEnums::PRIORITY));
	casa::ScalarColumn<casa::String> originCol(hisTable, MSHistory::columnName(MSHistoryEnums::ORIGIN));
	casa::ArrayColumn<casa::String> parmsCol(hisTable, MSHistory::columnName(MSHistoryEnums::APP_PARAMS));
	casa::ArrayColumn<casa::String> cliCol(hisTable, MSHistory::columnName(MSHistoryEnums::CLI_COMMAND));

	casa::Vector<casa::String> appParamsVec(1);
	appParamsVec[0] = "";
	casa::Vector<casa::String> cliVec(1);
	cliVec[0] = commandLine;
	
	size_t rowIndex = hisTable.nrow();
	hisTable.addRow();
	timeCol.put(rowIndex, casa::Time().modifiedJulianDay()*24.0*3600.0);
	obsIdCol.put(rowIndex, 0);
	messageCol.put(rowIndex, "parameters");
	applicationCol.put(rowIndex, application);
	priorityCol.put(rowIndex, "NORMAL");
	originCol.put(rowIndex, "standalone");
	parmsCol.put(rowIndex, appParamsVec);
	cliCol.put(rowIndex, cliVec);
}
