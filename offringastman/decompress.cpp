#include "offringastman.h"
#include "stmanmodifier.h"

using namespace offringastman;

int main(int argc, char *argv[])
{
	register_offringastman();
	
	if(argc < 2)
	{
		std::cerr << "Usage: decompress <ms>\n";
		return 0;
	}
	std::auto_ptr<casa::MeasurementSet> ms(new casa::MeasurementSet(argv[1], casa::Table::Update));
	StManModifier modifier(*ms);
	
	bool isDataReplaced = modifier.InitColumnWithDefaultStMan("DATA", false);
	bool isWeightReplaced = modifier.InitColumnWithDefaultStMan("WEIGHT_SPECTRUM", true);
	
	if(isDataReplaced)
		modifier.MoveColumnData<casa::Complex>("DATA");
	if(isWeightReplaced)
		modifier.MoveColumnData<float>("WEIGHT_SPECTRUM");

	if(isDataReplaced || isWeightReplaced)
	{
		StManModifier::Reorder(ms, argv[1]);
	}
	
	std::cout << "Finished.\n";
}
