#ifndef RMS_COLLECTOR_H
#define RMS_COLLECTOR_H

#include "rmstable.h"

#include <vector>

namespace casa {
	class MeasurementSet;
}

namespace offringastman {

/**
 * Calculates RMS values per baseline from a measurement set.
 * @author André Offringa
 */
class RMSCollector
{
public:
	/** 
	 * Construct a new RMS collector for given baseline specs.
	 * @param antennaCount Number of antennas in observation
	 * @param fieldCount Number of fields in observation
	 */
	RMSCollector(size_t antennaCount, size_t fieldCount);
	
	/** Destructor */
	~RMSCollector();
	
	/**
	 * Read the measurement set and collect information required to make an RMS table.
	 * The MS will be read twice, because certain baselines (notably autocorrelations)
	 * might be completely flagged. These baselines are found during the first read,
	 * and will be treated differently during collecting.
	 * @param ms The measurement set to read
	 * @param dataColumn Name of the column to use (e.g. DATA or CORRECTED_DATA)
	 */
	void CollectRMS(casa::MeasurementSet &ms, const std::string &dataColumn);
	
	/**
	 * As CollectRMS(), but calculated RMS of difference between two MSes.
	 */
	void CollectDifferenceRMS(casa::MeasurementSet &lhs, casa::MeasurementSet &rhs, const std::string &dataColumn);
	
	/**
	 * As CollectRMS(), but calculates Mean of difference between two MSes.
	 */
	void CollectDifferenceMean(casa::MeasurementSet &lhs, casa::MeasurementSet &rhs, const std::string &dataColumn);
	
	/**
	 * Get the table of RMS values per baseline.
	 * @returns The RMS values.
	 */
	RMSTable GetRMSTable();
	
	/**
	 * Get a table of Mean values per baseline. Only useful for CollectDifferenceMean()
	 * @returns The Mean values, stored as an RMSTable.
	 */
	RMSTable GetMeanTable();
	
	/**
	 * Summarize the RMS or Mean values for the first few antennas.
	 * @param rms Whether to show RMS values or Mean values.
	 */
	void Summarize(bool rms=true);
private:
	template<bool forRMS>
	void collectDifference(casa::MeasurementSet &lhs, casa::MeasurementSet &rhs, const std::string &dataColumn);
	
	void countUnflaggedSamples(casa::MeasurementSet& ms, std::vector<size_t> &unflaggedDataCounts);
	
	size_t matrixSize() const { return _antennaCount*_antennaCount*_fieldCount; }
	size_t index(size_t antenna1, size_t antenna2, size_t fieldId) const
	{
		return antenna1 + _antennaCount*(antenna2 + fieldId*_fieldCount);
	}
	
	size_t _antennaCount, _fieldCount;
	
	size_t *_counts;
	long double *_squaredSum;
};

} // end of namespace

#endif
