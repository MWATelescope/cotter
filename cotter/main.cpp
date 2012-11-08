#include <iostream>
#include <vector>

#include "cotter.h"

int main(int argc, char **argv)
{
	std::cout << "Running Cotter MWA preprocessing pipeline, converting to table.\n";
	
	std::vector<std::vector<std::string> > fileSets;
	int argi = 1;
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
		}
		else {
			++argi;
		}
	}
	
	if(argc == 1 || fileSets.empty())
	{
		std::cout << "usage: cotter -i <gpufile1> [<gpufile2> ..] [-i <gpufile1> <gpufile2> ..\n";
		return -1;
	}
	
	Cotter cotter;
	cotter.SetFileSets(fileSets);
	cotter.Run();
	
	return 0;
}
