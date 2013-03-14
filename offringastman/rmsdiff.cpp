#include "rmscollector.h"

#include <ms/MeasurementSets/MeasurementSet.h>

#include <iostream>

using namespace offringastman;

int main(int argc, char *argv[])
{
	if(argc != 4)
	{
		std::cout << "Syntax: rmsdiff <lhs ms> <rhs ms> <column>\nWill output the RMS of the difference between the two MS for a few baselines.\n";
		return 0;
	}
	casa::MeasurementSet
		lhsMS(argv[1]),
		rhsMS(argv[2]);
	
	std::cout << "== STATISTICS FOR LHS ==\n";
	RMSCollector rmsCollector(lhsMS.antenna().nrow(), lhsMS.field().nrow());
	rmsCollector.CollectRMS(lhsMS, argv[3]);
	rmsCollector.Summarize();
	
	std::cout << "== STATISTICS FOR LHS-RHS ==\n";
	RMSCollector rms2Collector(lhsMS.antenna().nrow(), lhsMS.field().nrow());
	rms2Collector.CollectDifferenceRMS(lhsMS, rhsMS, argv[3]);
	rms2Collector.Summarize();
	
	RMSCollector meanCollector(lhsMS.antenna().nrow(), lhsMS.field().nrow());
	meanCollector.CollectDifferenceMean(lhsMS, rhsMS, argv[3]);
	meanCollector.Summarize(false);
}
