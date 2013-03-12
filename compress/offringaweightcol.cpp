#include "offringaweightcol.h"
#include "offringastman.h"
#include "weightencoder.h"
#include "bytepacker.h"

OffringaWeightColumn::~OffringaWeightColumn()
{
	delete _encoder;
}

void OffringaWeightColumn::setShapeColumn(const casa::IPosition& shape)
{
	_shape = shape;
	_symbolsPerCell = 1;
	for(casa::IPosition::const_iterator i = _shape.begin(); i!=_shape.end(); ++i)
		_symbolsPerCell *= *i;
	
	delete[] _packBuffer;
	_symbolBuffer.resize(_symbolsPerCell);
	_packBuffer = new unsigned char[Stride()];
	_dataCopyBuffer.resize(_symbolsPerCell);
	
	recalculateStride();
	std::cout << "OffringaWeightColumn::setShapeColumn() : symbols per cell=" << _symbolsPerCell << ", Stride()=" << Stride() << '\n';
}

void OffringaWeightColumn::getArrayfloatV(casa::uInt rowNr, casa::Array<float>* dataPtr)
{
	readCompressedData(rowNr, _packBuffer);
	
	BytePacker::unpack6(&_symbolBuffer[0], _packBuffer + sizeof(float), _symbolsPerCell);
	
	_encoder->Decode(_dataCopyBuffer, *reinterpret_cast<float*>(_packBuffer), _symbolBuffer);
	
	std::vector<float>::const_iterator j = _dataCopyBuffer.begin();
	for(casa::Array<float>::contiter i = dataPtr->cbegin(); i!= dataPtr->cend(); ++i)
	{
		*i = *j;
		++j;
	}
}

void OffringaWeightColumn::putArrayfloatV(casa::uInt rowNr, const casa::Array<float>* dataPtr)
{
	std::vector<float>::iterator j = _dataCopyBuffer.begin();
	for(casa::Array<float>::const_contiter i = dataPtr->cbegin(); i!= dataPtr->cend(); ++i)
	{
		*j = *i;
		++j;
	}
	_encoder->Encode(*reinterpret_cast<float*>(_packBuffer), _symbolBuffer, _dataCopyBuffer);
	
	BytePacker::pack6(_packBuffer + sizeof(float), &_symbolBuffer[0], _symbolsPerCell);
	
	writeCompressedData(rowNr, _packBuffer);
}

void OffringaWeightColumn::Prepare()
{
	std::cout << "OffringaWeightColumn::Prepare()\n";
	delete _encoder;
	_encoder = new WeightEncoder<float>(1<<_bitsPerSymbol);
}
