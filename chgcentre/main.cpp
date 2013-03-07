#include <iostream>

#include <ms/MeasurementSets/MeasurementSet.h>

#include <measures/TableMeasures/ScalarMeasColumn.h>
#include <measures/Measures/MBaseline.h>
#include <measures/Measures/MCBaseline.h>
#include <measures/Measures/MCPosition.h>
#include <measures/Measures/MCDirection.h>
#include <measures/Measures/MDirection.h>
#include <measures/Measures/MEpoch.h>
#include <measures/Measures/MPosition.h>
#include <measures/Measures/Muvw.h>
#include <measures/Measures/MeasConvert.h>

#include "radeccoord.h"

using namespace casa;

std::vector<MPosition> antennas;

std::string dirToString(const MDirection &direction)
{
	double ra = direction.getAngle().getValue()[0];
	double dec = direction.getAngle().getValue()[1];
	return RaDecCoord::RAToString(ra) + " " + RaDecCoord::DecToString(dec);
}

double length(const Muvw &uvw)
{
	const Vector<double> v = uvw.getValue().getVector();
	return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

double length(const Vector<double> &v)
{
	return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

Muvw calculateUVW(const MPosition &antennaPos,
									const MPosition &refPos, const MEpoch &time, const MDirection &direction)
{
	const Vector<double> posVec = antennaPos.getValue().getVector();
	const Vector<double> refVec = refPos.getValue().getVector();
	MVPosition relativePos(posVec[0]-refVec[0], posVec[1]-refVec[1], posVec[2]-refVec[2]);
	MeasFrame frame(time, refPos, direction);
	MBaseline baseline(MVBaseline(relativePos), MBaseline::ITRF);
	baseline.getRefPtr()->set(frame);
	MBaseline j2000Baseline = MBaseline::Convert(baseline, MBaseline::J2000)();
	MVuvw uvw(j2000Baseline.getValue(), direction.getValue());
	return Muvw(uvw, Muvw::J2000);
}

void processField(MeasurementSet &set, int fieldIndex, MSField &fieldTable, const MDirection &newDirection)
{
	ROScalarColumn<casa::String> nameCol(fieldTable, fieldTable.columnName(MSFieldEnums::NAME));
	MDirection::ROScalarColumn phaseCol(fieldTable, fieldTable.columnName(MSFieldEnums::PHASE_DIR));
	Muvw::ROScalarColumn uvwCol(set, set.columnName(MSMainEnums::UVW));
	MEpoch::ROScalarColumn timeCol(set, set.columnName(MSMainEnums::TIME));
	
	ROScalarColumn<int>
		antenna1Col(set, set.columnName(MSMainEnums::ANTENNA1)),
		antenna2Col(set, set.columnName(MSMainEnums::ANTENNA2)),
		fieldIdCol(set, set.columnName(MSMainEnums::FIELD_ID));
	
	MDirection phaseDirection = phaseCol(fieldIndex);
	std::cout << "Processing field \"" << nameCol(fieldIndex) << "\": "
		<< dirToString(phaseDirection) << " -> "
		<< dirToString(newDirection) << "\n";
	MDirection refDirection =
		MDirection::Convert(newDirection,
												MDirection::Ref(MDirection::J2000))();
	
	for(unsigned row=0; row!=set.nrow(); ++row)
	{
		if(fieldIdCol(row) == fieldIndex)
		{
			const int
				antenna1 = antenna1Col(row),
				antenna2 = antenna2Col(row);
			const Muvw uvw = uvwCol(row);
			MEpoch time = timeCol(row);
			//MVEpoch b = time2.getValue() + 1.0/24.0;//7.778/24.0;
			//MEpoch time(b, time2.getRef());
			//MeasFrame refFrame(time, phaseDirection);
			
			Muvw uvw1 = calculateUVW(antennas[antenna1], antennas[0], time, refDirection);
			Muvw uvw2 = calculateUVW(antennas[antenna2], antennas[0], time, refDirection);
			
			if(row < 4)
			{
				//std::cout << "uvw1: " << uvw1 << " (" << length(uvw1) << ")\n";
				//std::cout << "uvw2: " << uvw2 << " (" << length(uvw2) << ")\n";
				//std::cout << "Antenna1: " << a1Pos << '\n';
				//std::cout << "Antenna2: " << a2Pos << '\n';
				std::cout << "Old " << uvw << " (" << length(uvw) << ")\n";
				std::cout << "New " << (uvw1.getValue().getVector()-uvw2.getValue().getVector()) << " (" << length(uvw1.getValue().getVector()-uvw2.getValue().getVector()) << ")\n";
				std::cout << "Time " << time << "\n";
			}
		}
	}
}

void readAntennas(MeasurementSet &set, std::vector<MPosition> &antennas)
{
	MSAntenna antennaTable = set.antenna();
	MPosition::ROScalarColumn posCol(antennaTable, antennaTable.columnName(MSAntennaEnums::POSITION));
	
	antennas.resize(antennaTable.nrow());
	for(unsigned i=0; i!=antennaTable.nrow(); ++i)
	{
		antennas[i] = MPosition::Convert(posCol(i), MPosition::ITRF)();
		Vector<double> diff = antennas[i].getValue().getVector()-antennas[0].getValue().getVector();
		std::cout << diff[0] << "\t" << diff[1] << "\t" << diff[2] << '\n';
	}
	Vector<double> diff = antennas[0].getValue().getVector();
	std::cout << diff[0]/5000.0 << "\t" << diff[1]/5000.0 << "\t" << diff[2]/5000.0 << '\n';
}

int main(int argc, char **argv)
{
	if(argc != 4)
	{
		std::cerr << "Syntax: chgcentre <ms> <new ra> <new dec>\n";
	} else {
		MeasurementSet set(argv[1]);
		double newRA = RaDecCoord::ParseRA(argv[2]);
		double newDec = RaDecCoord::ParseDec(argv[3]);
		MDirection newDirection(MVDirection(newRA, newDec), MDirection::Ref(MDirection::J2000));
		
		readAntennas(set, antennas);
		
		MSField fieldTable = set.field();
		for(unsigned fieldIndex=0; fieldIndex!=fieldTable.nrow(); ++fieldIndex)
		{
			processField(set, fieldIndex, fieldTable, newDirection);
		}
	}
	
	return 0;
}
