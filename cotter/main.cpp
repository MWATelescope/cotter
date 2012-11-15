#include <iostream>
#include <vector>

#include "cotter.h"

void usage()
{
	std::cout << "usage: cotter [options] -i <gpufile1> [<gpufile2> ..] [-i <gpufile1> <gpufile2> ..\n"
	"Options:\n"
	"  -o <filename>      Save output to given filename. Default is 'flagged.ms'.\n"
	"  -timeavg <factor>  Average 'factor' timesteps together before writing to measurement set.\n"
	"  -freqavg <factor>  Average 'factor' channels together before writing to measurement set.\n"
	"                     When averaging: flagging, collecting statistics and cable length fixes will be done\n"
	"                     at highest resolution. UVW positions will be recalculated for new timesteps.\n"
	"  -norfi             Disable RFI detection\n"
	"  -nostats           Disable collecting statistics\n";
}

int main(int argc, char **argv)
{
	std::cout << "Running Cotter MWA preprocessing pipeline.\n";
	
	std::vector<std::vector<std::string> > fileSets;
	int argi = 1;
	size_t freqAvg = 1, timeAvg = 1;
	Cotter cotter;
	const char *outputFilename = "flagged.ms";
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
		else if(strcmp(argv[argi], "-o") == 0)
		{
			outputFilename = argv[argi];
		}
		else if(strcmp(argv[argi], "-norfi") == 0)
		{
			cotter.SetRFIDetection(false);
		}
		else if(strcmp(argv[argi], "-nostats") == 0)
		{
			cotter.SetCollectStatistics(false);
		}
		else if(strcmp(argv[argi], "-timeavg") == 0)
		{
			++argi;
			timeAvg = atoi(argv[argi]);
		}
		else if(strcmp(argv[argi], "-freqavg") == 0)
		{
			++argi;
			freqAvg = atoi(argv[argi]);
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
	cotter.Run(outputFilename, timeAvg, freqAvg);
	
	return 0;
}
