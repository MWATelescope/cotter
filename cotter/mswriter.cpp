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
		
		ScalarColumn<double> *_timeCol;
		ScalarColumn<int> *_antenna1Col, *_antenna2Col;
		ScalarColumn<int> *_dataDescIdCol;
		ArrayColumn<double> *_uvwCol;
		ArrayColumn<std::complex<float> > *_dataCol;
		ArrayColumn<bool> *_flagCol;
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
	_data->_antenna1Col = new ScalarColumn<int>(ms, MS::columnName(casa::MSMainEnums::ANTENNA1));
	_data->_antenna2Col = new ScalarColumn<int>(ms, MS::columnName(casa::MSMainEnums::ANTENNA2));
	_data->_dataDescIdCol = new ScalarColumn<int>(ms, MS::columnName(casa::MSMainEnums::DATA_DESC_ID));
	_data->_uvwCol = new ArrayColumn<double>(ms, MS::columnName(casa::MSMainEnums::UVW));
	_data->_dataCol = new ArrayColumn<std::complex<float> >(ms, MS::columnName(casa::MSMainEnums::DATA));
	_data->_weightCol = new ArrayColumn<float>(ms, MS::columnName(casa::MSMainEnums::WEIGHT));
	_data->_weightSpectrumCol = new ArrayColumn<float>(ms, MS::columnName(casa::MSMainEnums::WEIGHT_SPECTRUM));
	_data->_flagCol = new ArrayColumn<bool>(ms, MS::columnName(casa::MSMainEnums::FLAG));
}

MSWriter::~MSWriter()
{
	delete _data->_timeCol;
	delete _data->_antenna1Col;
	delete _data->_antenna2Col;
	delete _data->_dataDescIdCol;
	delete _data->_uvwCol;
	delete _data->_dataCol;
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
}

void MSWriter::WriteAntennae(const std::vector<AntennaInfo> &antennae)
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

void MSWriter::WriteRow(double time, size_t antenna1, size_t antenna2, double u, double v, double w, const std::complex<float>* data, const bool* flags, const float *weights)
{
	_data->ms->addRow();
	_data->_timeCol->put(_rowIndex, time);
	_data->_antenna1Col->put(_rowIndex, antenna1);
	_data->_antenna2Col->put(_rowIndex, antenna2);
	_data->_dataDescIdCol->put(_rowIndex, 0);
	
	casa::Vector<double> uvwVec(3);
	uvwVec[0] = u; uvwVec[1] = v; uvwVec[2] = w;
	_data->_uvwCol->put(_rowIndex, uvwVec);
	
	size_t nPol = 4;
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
