#include <iostream>
#include <vector>

#include "cotter.h"

void usage()
{
	std::cout << "usage: cotter [options] -i <gpufile1> [<gpufile2> ..] [-i <gpufile1> <gpufile2> ..\n"
	"Options:\n"
	"  -norfi  disable RFI detection\n";
}

int main(int argc, char **argv)
{
	std::cout << "Running Cotter MWA preprocessing pipeline, converting to table.\n";
	
	std::vector<std::vector<std::string> > fileSets;
	int argi = 1;
	Cotter cotter;
	while(argi!=argc)
	{
		if(strcmp(argv[argi], "-i") == 0)
		{
			++argi;
			fileSets.push_back(std::vector<std::string>());
			std::vector<std::string> &newSet = *fileSets.rbegin();
			while(argi!=argc && argv[argi][0]!='-')
			{
				newSet.push_back(argv[argi]);
				++argi;
			}
			--argi;
		}
		else if(strcmp(argv[argi], "-norfi") == 0)
		{
			cotter.SetRFIDetection(false);
		}
		else if(strcmp(argv[argi], "-nostats") == 0)
		{
			cotter.SetCollectStatistics(false);
		}
		else {
			usage();
			return -1;
		}
		
		++argi;
	}
	
	if(argc == 1 || fileSets.empty())
	{
		usage();
		return -1;
	}
	
	cotter.SetFileSets(fileSets);
	cotter.SetThreadCount(sysconf(_SC_NPROCESSORS_ONLN));
	cotter.Run("flagged.ms", 4, 4);
	
	return 0;
}
