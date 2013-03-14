#ifndef RMS_TABLE_H
#define RMS_TABLE_H

#include <cstring>
#include <fstream>

namespace offringastman {

/**
 * Table for RMS values. Contains an RMS value per baseline, and is
 * a 3-dimensional matrix index on antenna1, antenna2 and fieldId.
 * Spectral windows will not be distinguished, so all spectral windows
 * will have a single RMS for each baseline.
 * @author André Offringa
 */
class RMSTable
{
	public:
		/** Type used for representing RMS values. */
		typedef float rms_t;
		/** Constructs an empty table. */
		RMSTable()
		: _rmsTable(0), _antennaCount(0), _fieldCount(0)
		{
		}
		/**
		 * Construct an RMS table with given dimensions.
		 * @param antennaCount Number of antennas
		 * @param fieldCount Number of fields
		 * @param initialValue Value to which all RMS values will be initialized.
		 */
		RMSTable(size_t antennaCount, size_t fieldCount, rms_t initialValue = 0.0)
		: _antennaCount(antennaCount), _fieldCount(fieldCount)
		{
			initialize();
			SetAll(initialValue);
		}
		/** Copy constructor. */
		RMSTable(const RMSTable& source)
		: _antennaCount(source._antennaCount), _fieldCount(source._fieldCount)
		{ 
			initialize();
			copyValuesFrom(source);
		}
		
		/** Destructor. */
		~RMSTable()
		{
			destruct();
		}
		
		/**
		 * Assign another table to this table.
		 * @param source Table to assign from.
		 */
		void operator=(const RMSTable& source)
		{ 
			destruct();
			_antennaCount = source._antennaCount;
			_fieldCount = source._fieldCount;
			initialize();
			copyValuesFrom(source);
		}
		
		/**
		 * Get a constant RMS value for a given baseline.
		 * @param antenna1 Index of antenna1 of baseline.
		 * @param antenna2 Index of antenna2 of baseline.
		 * @param fieldId Id of field of baseline.
		 */
		const rms_t &Value(size_t antenna1, size_t antenna2, size_t fieldId) const
		{ return _rmsTable[fieldId][antenna2][antenna1]; }
		
		/**
		 * Get an RMS value for a given baseline.
		 * @param antenna1 Index of antenna1 of baseline.
		 * @param antenna2 Index of antenna2 of baseline.
		 * @param fieldId Id of field of baseline.
		 */
		rms_t &Value(size_t antenna1, size_t antenna2, size_t fieldId)
		{ return _rmsTable[fieldId][antenna2][antenna1]; }
		
		/**
		 * Whether the given baseline is within the dimensions of this table.
		 * @param antenna1 Index of antenna1 of baseline.
		 * @param antenna2 Index of antenna2 of baseline.
		 * @param fieldId Id of field of baseline.
		 */
		bool IsInTable(size_t antenna1, size_t antenna2, size_t fieldId) const
		{
			return fieldId < _fieldCount && antenna1 < _antennaCount && antenna2 < _antennaCount;
		}
		
		/**
		 * Set all RMS values to a particular value.
		 * @param rms New value
		 */
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
		
		/**
		 * Whether this RMS table is empty.
		 * @returns true when AntennaCount()==0 || FieldCount()==0
		 */
		bool Empty() const {
			return _antennaCount==0 || _fieldCount==0;
		}
		
		/** Get number of antennas. */
		size_t AntennaCount() const { return _antennaCount; }
		
		/** Get number of fields. */
		size_t FieldCount() const { return _fieldCount; }
		
		/**
		 * Write this table to a file. Will write SizeInBytes() bytes
		 * to current write position.
		 * @param str stream to write to.
		 */
		void Write(std::fstream &str);
		
		/**
		 * Read this table from a file. Will read SizeInBytes() bytes
		 * from current read position.
		 * @param str stream to read from.
		 */
		void Read(std::fstream &str);
		
		/**
		 * Number of bytes that Write() and Read() will process.
		 * @returns Size in bytes.
		 */
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
