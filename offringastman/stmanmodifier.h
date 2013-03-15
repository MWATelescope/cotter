#ifndef STMAN_MODIFIER_H
#define STMAN_MODIFIER_H

#include "offringastman.h"
#include "rmscollector.h"
#include "rmstable.h"

#include <ms/MeasurementSets/MeasurementSet.h>

#include <tables/Tables/ArrColDesc.h>
#include <tables/Tables/ArrayColumn.h>

#include <iostream>

namespace offringastman {

/**
 * Modify the storage manager of columns in a measurement set.
 * Used by the compress and decompress tools.
 * @author André Offringa
 */
class StManModifier
{
public:
	/**
	 * Constructor.
	 * @param ms Measurement set on which to operate.
	 */
	StManModifier(casa::MeasurementSet& ms) : _ms(ms)
	{
	}
	
	/**
	 * Get the name of a storage manager for a given
	 * column and columntype. The column should be an arraycolumn.
	 * @tparam T The type of the column.
	 * @param columnName Name of the column.
	 */
	template<typename T>
	std::string GetStorageManager(const std::string& columnName)
	{
		casa::ArrayColumn<T> dataCol(_ms, columnName);
		return dataCol.columnDesc().dataManagerType();
	}

	/**
	 * Rename the old column and create a new column with the default storage manager.
	 * After all columns have been renamed, the operation should be finished by a call
	 * to MoveColumnData() if @c true was returned.
	 * @param columnName Name of column.
	 * @param isWeight true if a weight (float) column should be created, or false to create
	 * a complex column.
	 * @returns @c true if the storage manager was changed.
	 */
	bool InitColumnWithDefaultStMan(const std::string& columnName, bool isWeight)
	{
		std::string dataManager;
		if(isWeight)
			dataManager = GetStorageManager<float>(columnName);
		else
			dataManager = GetStorageManager<casa::Complex>(columnName);
		std::cout << "Current data manager of " + columnName + " column: " << dataManager << '\n';
		
		if(dataManager == "OffringaStMan")
		{
			std::string tempName = std::string("TEMP_") + columnName;
			std::cout << "Renaming old " + columnName + " column...\n";
			_ms.renameColumn(tempName, columnName);
			
			if(isWeight)
			{
				casa::ArrayColumn<float> oldColumn(_ms, tempName);
				casa::ArrayColumnDesc<float> columnDesc(columnName, oldColumn.shape(0));
				_ms.addColumn(columnDesc);
			} else {
				casa::ArrayColumn<casa::Complex> oldColumn(_ms, tempName);
				casa::ArrayColumnDesc<casa::Complex> columnDesc(columnName, oldColumn.shape(0));
				_ms.addColumn(columnDesc);
			}
			return true;
		} else {
			return false;
		}
	}

	/**
	 * Rename the old complex column and create a new column with the OffringaStMan.
	 * After all columns have been renamed, the operation should be finished by a call
	 * to MoveColumnData() if @c true was returned.
	 * @param columnName Name of column.
	 * @param bitsPerComplex If a OffringaStMan needs to be constructed, set this number
	 * of bits per data value.
	 * @param bitsPerWeight If a OffringaStMan needs to be constructed, set this number
	 * of bits per weight value.
	 * @returns @c true if the storage manager was changed.
	 */
	bool InitComplexColumnWithOffringaStMan(const std::string& columnName, unsigned bitsPerComplex, unsigned bitsPerWeight)
	{
		const std::string dataManager = GetStorageManager<casa::Complex>(columnName);
		std::cout << "Current data manager of " + columnName + " column: " << dataManager << '\n';
		
		if(dataManager != "OffringaStMan")
		{
			RMSCollector rmsCollector(_ms.antenna().nrow(), _ms.field().nrow());
			rmsCollector.CollectRMS(_ms, columnName);
			rmsCollector.Summarize();
			
			std::string tempName = std::string("TEMP_") + columnName;
			std::cout << "Renaming old " + columnName + " column...\n";
			_ms.renameColumn(tempName, columnName);
			
			casa::ArrayColumn<casa::Complex> oldColumn(_ms, tempName);
				
			createOffringaStManColumn<casa::Complex>(columnName, oldColumn.shape(0), rmsCollector.GetRMSTable(), bitsPerComplex, bitsPerWeight);
			return true;
		} else {
			return false;
		}
	}

	/**
	 * Rename the old float array column and create a new column with the OffringaStMan.
	 * After all columns have been renamed, the operation should be finished by a call
	 * to MoveColumnData() if @c true was returned.
	 * @param columnName Name of column.
	 * @param bitsPerComplex If a OffringaStMan needs to be constructed, set this number
	 * of bits per data value.
	 * @param bitsPerWeight If a OffringaStMan needs to be constructed, set this number
	 * of bits per weight value.
	 * @returns @c true if the storage manager was changed.
	 */
	bool InitWeightColumnWithOffringaStMan(const std::string& columnName, unsigned bitsPerComplex, unsigned bitsPerWeight)
	{
		const std::string dataManager = GetStorageManager<float>(columnName);
		std::cout << "Current data manager of " + columnName + " column: " << dataManager << '\n';
		
		if(dataManager != "OffringaStMan")
		{
			std::string tempName = std::string("TEMP_") + columnName;
			std::cout << "Renaming old " + columnName + " column...\n";
			_ms.renameColumn(tempName, columnName);
			
			casa::ArrayColumn<float> oldColumn(_ms, tempName);
				
			createOffringaStManColumn<float>(columnName, oldColumn.shape(0), RMSTable(), bitsPerComplex, bitsPerWeight);
			return true;
		} else {
			return false;
		}
	}

	/**
	 * Finish changing the storage manager for a column.
	 * This should be called for each changed column after all calls to the Init..()
	 * method have been performed.
	 * @tparam T Type of array column
	 * @param columnName Name of column.
	 */
	template<typename T>
	void MoveColumnData(const std::string& columnName)
	{
		std::cout << "Copying values for " << columnName << " ...\n";
		std::string tempName = std::string("TEMP_") + columnName;
		std::auto_ptr<casa::ArrayColumn<T> > oldColumn(new casa::ArrayColumn<T>(_ms, tempName));
		casa::ArrayColumn<T> newColumn(_ms, columnName);
		copyValues(newColumn, *oldColumn, _ms.nrow());
		oldColumn.reset();
		
		std::cout << "Removing old column...\n";
		_ms.removeColumn(tempName);
	}
	
	/**
	 * Perform a deep copy of the measurement set, and replace the old set
	 * with the new measurement set. This is useful to free unused space.
	 * When removing a column from a storage manager,
	 * the space occupied by the column is not freed. To make sure that any space
	 * occupied is freed, this copy is necessary.
	 * @param ms Pointer to old measurement set
	 * @param msLocation Path of the measurent set.
	 */
	static void Reorder(std::auto_ptr<casa::MeasurementSet>& ms, const std::string& msLocation)
	{
		std::cout << "Reordering ms...\n";
		std::string tempName = msLocation;
		while(*tempName.rbegin() == '/') tempName.resize(tempName.size()-1);
		tempName += "temp";
		ms->deepCopy(tempName, casa::Table::New, true);
		ms->markForDelete();

		// Destruct the old measurement set, and load new one
		ms.reset(new casa::MeasurementSet(tempName, casa::Table::Update));
		ms->rename(std::string(msLocation), casa::Table::New);
	}
	
private:
	casa::MeasurementSet &_ms;

	template<typename T>
	void copyValues(casa::ArrayColumn<T>& newColumn, casa::ArrayColumn<T>& oldColumn, size_t nrow)
	{
		casa::Array<T> values;
		for(size_t row=0; row!=nrow; ++row)
		{
			oldColumn.get(row, values);
			newColumn.put(row, values);
		}
	}
	
	template<typename T>
	void createOffringaStManColumn(const std::string& name, const casa::IPosition& shape, const RMSTable& rmsTable, unsigned bitsPerComplex, unsigned bitsPerWeight)
	{
		std::cout << "Constructing new column...\n";
		casa::ArrayColumnDesc<T> columnDesc(name, "", "OffringaStMan", "OffringaStMan", shape);
		columnDesc.setOptions(casa::ColumnDesc::Direct | casa::ColumnDesc::FixedShape);
		
		std::cout << "Querying storage manager...\n";
		bool isAlreadyUsed;
		try {
			_ms.findDataManager("OffringaStMan");
			isAlreadyUsed = true;
		} catch(std::exception &e)
		{
			std::cout << "Constructing storage manager...\n";
			OffringaStMan dataManager(rmsTable, bitsPerComplex, bitsPerWeight);
			std::cout << "Adding column...\n";
			_ms.addColumn(columnDesc, dataManager);
			isAlreadyUsed = false;
		}
		if(isAlreadyUsed)
		{
			std::cout << "Adding column with existing datamanager...\n";
			_ms.addColumn(columnDesc, "OffringaStMan", false);
		}
	}
};

}

#endif
