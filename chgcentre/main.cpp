#include <iostream>
#include <memory>

#include <ms/MeasurementSets/MeasurementSet.h>

#include <measures/TableMeasures/ArrayMeasColumn.h>
#include <measures/TableMeasures/ScalarMeasColumn.h>

#include <measures/Measures/MBaseline.h>
#include <measures/Measures/MCBaseline.h>
#include <measures/Measures/MCPosition.h>
#include <measures/Measures/MCDirection.h>
#include <measures/Measures/MCuvw.h>
#include <measures/Measures/MDirection.h>
#include <measures/Measures/MEpoch.h>
#include <measures/Measures/MPosition.h>
#include <measures/Measures/Muvw.h>
#include <measures/Measures/MeasConvert.h>

#include "radeccoord.h"
#include "banddata.h"

using namespace casa;

std::vector<MPosition> antennas;

//typedef long int integer;
//typedef double doublereal;
typedef int integer;
typedef double doublereal;

extern "C" {
  extern void dgesvd_(const char* jobu, const char* jobvt, const integer* M, const integer* N,
        doublereal* A, const integer* lda, doublereal* S, doublereal* U, const integer* ldu,
        doublereal* VT, const integer* ldvt, doublereal* work,const integer* lwork, const
        integer* info);
}

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
	MBaseline baseline(MVBaseline(relativePos), MBaseline::Ref(MBaseline::ITRF, frame));
	MBaseline j2000Baseline = MBaseline::Convert(baseline, MBaseline::J2000)();
	MVuvw uvw(j2000Baseline.getValue(), direction.getValue());
	return Muvw(uvw, Muvw::J2000);
}

void rotateVisibilities(const BandData &bandData, double wShiftFactor, unsigned polarizationCount, Array<Complex>::contiter dataIter)
{
	for(unsigned ch=0; ch!=bandData.ChannelCount(); ++ch)
	{
		const double wShiftRad = wShiftFactor / bandData.ChannelWavelength(ch);
		double rotSin, rotCos;
		sincos(wShiftRad, &rotSin, &rotCos);
		for(unsigned p=0; p!=polarizationCount; ++p)
		{
			Complex v = *dataIter;
			*dataIter = Complex(
				v.real() * rotCos  -  v.imag() * rotSin,
				v.real() * rotSin  +  v.imag() * rotCos);
			++dataIter;
		}
	}
}

void processField(MeasurementSet &set, int fieldIndex, MSField &fieldTable, const MDirection &newDirection)
{
	BandData bandData(set.spectralWindow());
	ROScalarColumn<casa::String> nameCol(fieldTable, fieldTable.columnName(MSFieldEnums::NAME));
	MDirection::ArrayColumn phaseDirCol(fieldTable, fieldTable.columnName(MSFieldEnums::PHASE_DIR));
	MEpoch::ROScalarColumn timeCol(set, set.columnName(MSMainEnums::TIME));
	MDirection::ArrayColumn delayDirCol(fieldTable, fieldTable.columnName(MSFieldEnums::DELAY_DIR));
	MDirection::ArrayColumn refDirCol(fieldTable, fieldTable.columnName(MSFieldEnums::REFERENCE_DIR));
	
	ROScalarColumn<int>
		antenna1Col(set, set.columnName(MSMainEnums::ANTENNA1)),
		antenna2Col(set, set.columnName(MSMainEnums::ANTENNA2)),
		fieldIdCol(set, set.columnName(MSMainEnums::FIELD_ID));
	ArrayColumn<Complex>
		dataCol(set, set.columnName(MSMainEnums::DATA));
	Muvw::ROScalarColumn
		uvwCol(set, set.columnName(MSMainEnums::UVW));
	ArrayColumn<double>
		uvwOutCol(set, set.columnName(MSMainEnums::UVW));
	
	const bool
		hasCorrData = set.isColumn(casa::MSMainEnums::CORRECTED_DATA),
		hasModelData = set.isColumn(casa::MSMainEnums::MODEL_DATA);
	std::auto_ptr<ArrayColumn<Complex> > correctedDataCol, modelDataCol;
	if(hasCorrData)
	{
		correctedDataCol.reset(new ArrayColumn<Complex>(set,
			set.columnName(MSMainEnums::CORRECTED_DATA)));
	}
	if(hasModelData)
	{
		modelDataCol.reset(new ArrayColumn<Complex>(set,
			set.columnName(MSMainEnums::MODEL_DATA)));
	}
	
	Vector<MDirection> phaseDirVector = phaseDirCol(fieldIndex);
	MDirection phaseDirection = phaseDirVector[0];
	std::cout << "Processing field \"" << nameCol(fieldIndex) << "\": "
		<< dirToString(phaseDirection) << " -> "
		<< dirToString(newDirection) << "\n";
	if(dirToString(phaseDirection) == dirToString(newDirection))
	{
		std::cout << "Phase centre did not change: skipping field.\n";
	}
	else {
		MDirection refDirection =
			MDirection::Convert(newDirection,
													MDirection::Ref(MDirection::J2000))();
		IPosition dataShape = dataCol.shape(0);
		unsigned polarizationCount = dataShape[0];
		Array<Complex> dataArray(dataShape);
		
		for(unsigned row=0; row!=set.nrow(); ++row)
		{
			if(fieldIdCol(row) == fieldIndex)
			{
				// Read values from set
				const int
					antenna1 = antenna1Col(row),
					antenna2 = antenna2Col(row);
				const Muvw oldUVW = uvwCol(row);
				MEpoch time = timeCol(row);

				// Calculate the new UVW
				Muvw uvw1 = calculateUVW(antennas[antenna1], antennas[0], time, refDirection);
				Muvw uvw2 = calculateUVW(antennas[antenna2], antennas[0], time, refDirection);
				MVuvw newUVW = uvw1.getValue()-uvw2.getValue();
				
				// If one of the first results, output values for analyzing them.
				if(row < 5)
				{
					std::cout << "Old " << oldUVW << " (" << length(oldUVW) << ")\n";
					std::cout << "New " << newUVW << " (" << length(newUVW) << ")\n\n";
				}
				
				// Read the visibilities and phase-rotate them
				double wShiftFactor =
					-2.0*M_PI* (newUVW.getVector()[2] - oldUVW.getValue().getVector()[2]);

				dataCol.get(row, dataArray);
				rotateVisibilities(bandData, wShiftFactor, polarizationCount, dataArray.cbegin());
				dataCol.put(row, dataArray);
					
				if(hasCorrData)
				{
					correctedDataCol->get(row, dataArray);
					rotateVisibilities(bandData, wShiftFactor, polarizationCount, dataArray.cbegin());
					correctedDataCol->put(row, dataArray);
				}
				
				if(hasModelData)
				{
					modelDataCol->get(row, dataArray);
					rotateVisibilities(bandData, wShiftFactor, polarizationCount, dataArray.cbegin());
					modelDataCol->put(row, dataArray);
				}
				
				// Store uvws
				uvwOutCol.put(row, newUVW.getVector());
			}
		}
		phaseDirVector[0] = newDirection;
		phaseDirCol.put(fieldIndex, phaseDirVector);
		delayDirCol.put(fieldIndex, phaseDirVector);
		refDirCol.put(fieldIndex, phaseDirVector);
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
		//Vector<double> diff = antennas[i].getValue().getVector()-antennas[0].getValue().getVector();
		//std::cout << diff[0] << "\t" << diff[1] << "\t" << diff[2] << '\n';
	}
	//Vector<double> diff = antennas[0].getValue().getVector();
	//std::cout << diff[0]/5000.0 << "\t" << diff[1]/5000.0 << "\t" << diff[2]/5000.0 << '\n';
}

casa::MPosition ArrayCentroid(MeasurementSet& set)
{
	casa::MSAntenna aTable = set.antenna();
	if(aTable.nrow() == 0) throw std::runtime_error("No antennae in set");
	casa::MPosition::ROScalarColumn antPosColumn(aTable, aTable.columnName(casa::MSAntennaEnums::POSITION));
	double x = 0.0, y = 0.0, z = 0.0;
	for(size_t row=0; row!=aTable.nrow(); ++row)
	{
		casa::MPosition antPos = antPosColumn(row);
		casa::Vector<casa::Double> vec = antPos.getValue().getVector();
		x += vec[0]; y += vec[1]; z += vec[2];
	}
	casa::MPosition arrayPos = antPosColumn(0);
	double count = aTable.nrow();
	arrayPos.set(casa::MVPosition(x/count, y/count, z/count), arrayPos.getRef());
	return arrayPos;
}

MDirection ZenithDirection(MeasurementSet& set)
{
	casa::MPosition arrayPos = ArrayCentroid(set);
	casa::MEpoch::ROScalarColumn timeColumn(set, set.columnName(casa::MSMainEnums::TIME));
	casa::MEpoch time = timeColumn(set.nrow()/2);
	casa::MeasFrame frame(arrayPos, time);
	const casa::MDirection::Ref azelgeoRef(casa::MDirection::AZELGEO, frame);
	const casa::MDirection::Ref j2000Ref(casa::MDirection::J2000, frame);
	casa::MDirection zenithAzEl(casa::MVDirection(0.0, 0.0, 1.0), azelgeoRef);
	return casa::MDirection::Convert(zenithAzEl, j2000Ref)();
}

MDirection MinWDirection(MeasurementSet& set)
{
	MPosition centroid = ArrayCentroid(set);
	casa::Vector<casa::Double> cvec = centroid.getValue().getVector();
	double cx = cvec[0], cy = cvec[1], cz = cvec[2];
	
	casa::MSAntenna aTable = set.antenna();
	casa::MPosition::ROScalarColumn antPosColumn(aTable, aTable.columnName(casa::MSAntennaEnums::POSITION));
	integer n = 3, m = aTable.nrow(), lda = m, ldu = m, ldvt = n;
	std::vector<double> a(m*n);
	
	for(size_t row=0; row!=aTable.nrow(); ++row)
	{
		MPosition pos = antPosColumn(row);
		casa::Vector<casa::Double> vec = pos.getValue().getVector();
		a[row] = vec[0]-cx, a[row+m] = vec[1]-cy, a[row+2*m] = vec[2]-cz;
	}
	
	double wkopt;
	std::vector<double> s(n), u(ldu*m), vt(ldvt*n);
	integer lwork = -1, info;
	dgesvd_( "All", "All", &m, &n, &a[0], &lda, &s[0], &u[0], &ldu, &vt[0], &ldvt, &wkopt, &lwork, &info);
	lwork = (int) wkopt;
	double* work = (double*) malloc( lwork*sizeof(double) );
	/* Compute SVD */
	dgesvd_( "All", "All", &m, &n, &a[0], &lda, &s[0], &u[0], &ldu, &vt[0], &ldvt, work, &lwork, &info );
	free((void*) work);
	
	if(info > 0)
		throw std::runtime_error("The algorithm computing SVD failed to converge");
	else {
		// Get the right singular vector belonging to the smallest SV
		double x = vt[n*0 + n-1], y = vt[n*1 + n-1], z = vt[n*2 + n-1];
		// Get the hemisphere right
		if((z < 0.0 && cz > 0.0) || (z > 0.0 && cz < 0.0))
		{
			x = -x; y =-y; z = -z;
		}
		
		casa::MEpoch::ROScalarColumn timeColumn(set, set.columnName(casa::MSMainEnums::TIME));
		casa::MEpoch time = timeColumn(set.nrow()/2);
		casa::MeasFrame frame(centroid, time);
		MDirection::Ref ref(casa::MDirection::ITRF, frame);
		casa::MDirection direction(casa::MVDirection(x, y, z), ref);
		const casa::MDirection::Ref j2000Ref(casa::MDirection::J2000, frame);
		return casa::MDirection::Convert(direction, j2000Ref)();
	}
}

void printPhaseDir(const std::string &filename)
{
	MeasurementSet set(filename);
	MSField fieldTable = set.field();
	MDirection::ROArrayColumn phaseDirCol(fieldTable, fieldTable.columnName(MSFieldEnums::PHASE_DIR));
	MDirection zenith = ZenithDirection(set);
	
	std::cout << "Current phase direction:\n  ";
	for(size_t i=0; i!=fieldTable.nrow(); ++i)
	{
		Vector<MDirection> phaseDirVector = phaseDirCol(i);
		MDirection phaseDirection = phaseDirVector[0];
		std::cout << dirToString(phaseDirection) << '\n';
	}
	
	std::cout << "Zenith is at:\n  " << dirToString(zenith) << '\n';
	std::cout << "Min-w direction is at:\n  " << dirToString(MinWDirection(set)) << '\n';
}

int main(int argc, char **argv)
{
	std::cout <<
		"A program to change the phase centre of a measurement set.\n"
		"Written by André Offringa (offringa@gmail.com).\n\n";
	if(argc < 2)
	{
		std::cout <<
			"Syntax: chgcentre [options] <ms> <new ra> <new dec>\n\n"
			"The format of RA can either be 00h00m00.0s or 00:00:00.0\n"
			"The format of Dec can either be 00d00m00.0s or 00.00.00.0\n\n"
			"Example to rotate to HydA:\n"
			"\tchgcentre myset.ms 09h18m05.8s -12d05m44s\n\n";
	} else {
		int argi=1;
		bool toZenith = false, toMinW = false;
		while(argv[argi][0] == '-')
		{
			std::string param(&argv[argi][1]);
			if(param == "zenith")
			{
				toZenith = true;
			}
			else if(param == "minw")
			{
				toMinW = true;
			}
			else throw std::runtime_error("Invalid parameter");
			++argi;
		}
		if(argi == argc)
			std::cout << "Missing parameter.\n";
		else if(argi+1 == argc && !toZenith && !toMinW)
		{
			printPhaseDir(argv[argi]);
		}
		else {
			MeasurementSet set(argv[argi], Table::Update);
			MDirection newDirection;
			if(toZenith)
			{
				newDirection = ZenithDirection(set);
			}
			else if(toMinW)
			{
				newDirection = MinWDirection(set);
			}
			else {
				double newRA = RaDecCoord::ParseRA(argv[argi+1]);
				double newDec = RaDecCoord::ParseDec(argv[argi+2]);
				newDirection = MDirection(MVDirection(newRA, newDec), MDirection::Ref(MDirection::J2000));
			}
			
			readAntennas(set, antennas);
			
			MSField fieldTable = set.field();
			for(unsigned fieldIndex=0; fieldIndex!=fieldTable.nrow(); ++fieldIndex)
			{
				processField(set, fieldIndex, fieldTable, newDirection);
			}
		}
	}
	
	return 0;
}
