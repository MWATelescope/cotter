#ifndef RMS_TABLE_H
#define RMS_TABLE_H

#include <cstring>
#include <fstream>

namespace offringastman {
	
class RMSTable
{
	public:
		typedef float rms_t;
		RMSTable()
		: _rmsTable(0), _antennaCount(0), _fieldCount(0)
		{
		}
		RMSTable(size_t antennaCount, size_t fieldCount, rms_t initialValue = 0.0)
		: _antennaCount(antennaCount), _fieldCount(fieldCount)
		{
			initialize();
			SetAll(initialValue);
		}
		~RMSTable()
		{
			destruct();
		}
		const rms_t &Value(size_t antenna1, size_t antenna2, size_t fieldId) const
		{ return _rmsTable[fieldId][antenna2][antenna1]; }
		rms_t &Value(size_t antenna1, size_t antenna2, size_t fieldId)
		{ return _rmsTable[fieldId][antenna2][antenna1]; }
		bool IsInTable(size_t antenna1, size_t antenna2, size_t fieldId) const
		{
			return fieldId < _fieldCount && antenna1 < _antennaCount && antenna2 < _antennaCount;
		}
		
		void SetAll(rms_t rms)
		{
			for(size_t f=0; f!=_fieldCount; ++f)
			{
				for(size_t a1=0; a1!=_antennaCount; ++a1)
				{
					for(size_t a2=0; a2!=_antennaCount; ++a2)
						_rmsTable[f][a1][a2] = rms;
				}
			}
		}
		
		RMSTable(const RMSTable& source)
		: _antennaCount(source._antennaCount), _fieldCount(source._fieldCount)
		{ 
			initialize();
			copyValuesFrom(source);
		}
		
		bool Empty() const {
			return _antennaCount==0 || _fieldCount==0;
		}
		
		void operator=(const RMSTable& source)
		{ 
			destruct();
			_antennaCount = source._antennaCount;
			_fieldCount = source._fieldCount;
			initialize();
			copyValuesFrom(source);
		}
		
		size_t AntennaCount() const { return _antennaCount; }
		
		size_t FieldCount() const { return _fieldCount; }
		
		void Write(std::fstream &str);
		
		void Read(std::fstream &str);
		
		size_t SizeInBytes() const {
			return _antennaCount * _antennaCount * _fieldCount * sizeof(RMSTable::rms_t);
		}
	private:
		void initialize()
		{
			_rmsTable = new rms_t**[_fieldCount];
			for(size_t f=0; f!=_fieldCount; ++f)
			{
				_rmsTable[f] = new rms_t*[_antennaCount];
				for(size_t a1=0; a1!=_antennaCount; ++a1)
				{
					_rmsTable[f][a1] = new rms_t[_antennaCount];
				}
			}
		}
		void destruct()
		{
			for(size_t f=0; f!=_fieldCount; ++f)
			{
				for(size_t a1=0; a1!=_antennaCount; ++a1)
					delete[] _rmsTable[f][a1];
				delete[] _rmsTable[f];
			}
			delete[] _rmsTable;
		}
		void copyValuesFrom(const RMSTable& source)
		{
			for(size_t f=0; f!=_fieldCount; ++f)
			{
				for(size_t a1=0; a1!=_antennaCount; ++a1)
				{
					for(size_t a2=0; a2!=_antennaCount; ++a2)
						_rmsTable[f][a1][a2] = source._rmsTable[f][a1][a2];
				}
			}
		}
		rms_t ***_rmsTable;
		size_t _antennaCount, _fieldCount;
};

} // end of namespace

#endif
