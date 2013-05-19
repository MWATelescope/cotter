#include <iostream>
#include <vector>

#include "cotter.h"
#include "radeccoord.h"

#include <boost/algorithm/string.hpp>

bool isDigit(char c)
{
	return c >= '0' && c <= '9';
}

void checkInputFilename(const std::string &filename)
{
	if(filename.size() < 5 || filename.substr(filename.size()-5)!=".fits")
		throw std::runtime_error("The raw GPU input files should have a .fits extension");
	if(filename.size() < 10 || filename[filename.size()-8] != '_')
		throw std::runtime_error("The raw GPU input filenames should end in '...nn_nn.fits'");
	if(!isDigit(filename[filename.size()-9]) || !isDigit(filename[filename.size()-10]))
		throw std::runtime_error("Could not parse gpu box number -- the raw GPU input filenames should end in '...nn_nn.fits'");
	if(!isDigit(filename[filename.size()-6]) || !isDigit(filename[filename.size()-7]))
		throw std::runtime_error("Could not parse time step number -- the raw GPU input filenames should end in '...nn_nn.fits'");
}

size_t timeStepNumberFromFilename(const std::string &filename)
{
	return (filename[filename.size()-7]-'0')*10 + (filename[filename.size()-6]-'0');
}

size_t gpuBoxNumberFromFilename(const std::string &filename)
{
	size_t num = (filename[filename.size()-10]-'0')*10 + (filename[filename.size()-9]-'0');
	if(num == 0)
		throw std::runtime_error("Could not parse gpu box number in filename (it was zero)");
	return num-1;
}

bool isFitsFile(const std::string &filename)
{
	if(filename.size() > 7)
	{
		return boost::to_upper_copy(filename.substr(filename.size()-7)) == ".UVFITS";
	}
	return false;
}

bool isMWAFlagFile(const std::string &filename)
{
	if(filename.size() > 5)
	{
		return boost::to_upper_copy(filename.substr(filename.size()-5)) == ".MWAF";
	}
	return false;
}

void usage()
{
	std::cout << "usage: cotter [options] <gpufiles> \n"
	"Options:\n"
	"  -o <filename>      Save output to given filename. Default is 'preprocessed.ms'.\n"
	"                     If the files' extension is .uvfits, it will be outputted in uvfits format.\n"
	"  -m <filename>      Read meta data from given fits filename (if not specified, cotter will\n"
	"                     look for the txt metafiles with default names)\n"
	"  -mem <percentage>  Use at most the given percentage of memory.\n"
	"  -timeavg <factor>  Average 'factor' timesteps together before writing to measurement set.\n"
	"  -freqavg <factor>  Average 'factor' channels together before writing to measurement set.\n"
	"                     When averaging: flagging, collecting statistics and cable length fixes will be done\n"
	"                     at highest resolution. UVW positions will be recalculated for new timesteps.\n"
	"  -norfi             Disable RFI detection\n"
	"  -nostats           Disable collecting statistics\n"
	"  -nogeom            Disable geometric corrections\n"
	"  -noantennapruning  Do not remove the flagged antennae.\n"
	"  -noautos           Do not output auto-correlations.\n"
	"  -centre <ra> <dec> Set alternative phase centre, e.g. -centre 00h00m00.0s 00d00m00.0s\n"
	"  -sbcount <count>   Read/processes the first given number of subbands\n"
	"  -sbpassband <file> Read the sub-band passband from given file instead of using default passband\n"
	"                     (default passband does a reasonably good job)\n"
	"\n"
	"The filenames of the input gpu files should end in '...nn_mm.fits', where nn >= 1 is the\n"
	"gpu box number and mm >= 0 is the time step number.\n";
}

int cotterMain(int argc, const char* const* argv);

int main(int argc, char **argv)
{
	std::cout << "Running Cotter MWA preprocessing pipeline.\n";
	
	try {
		cotterMain(argc, argv);
	} catch(std::exception &e)
	{
		std::cout << "\nAn unhandled exception occured while running Cotter:\n" << e.what() << '\n';
	}
}

int cotterMain(int argc, const char* const* argv)
{
	std::vector<std::string> unsortedFiles;
	int argi = 1;
	size_t freqAvg = 1, timeAvg = 1;
	double memPercentage = 90.0;
	Cotter cotter;
	const char *outputFilename = "preprocessed.ms";
	while(argi!=argc)
	{
		if(strcmp(argv[argi], "-o") == 0)
		{
			++argi;
			outputFilename = argv[argi];
			if(isFitsFile(outputFilename))
			{
				cotter.SetCollectStatistics(false);
				cotter.SetOutputFormat(Cotter::FitsOutputFormat);
			}
			else if(isMWAFlagFile(outputFilename))
			{
				cotter.SetCollectStatistics(false);
				cotter.SetOutputFormat(Cotter::FlagsOutputFormat);
			}
		}
		else if(strcmp(argv[argi], "-m") == 0)
		{
			++argi;
			cotter.SetMetaFilename(argv[argi]);
		}
		else if(strcmp(argv[argi], "-mem") == 0)
		{
			++argi;
			memPercentage = atof(argv[argi]);
		}
		else if(strcmp(argv[argi], "-norfi") == 0)
		{
			cotter.SetRFIDetection(false);
		}
		else if(strcmp(argv[argi], "-nostats") == 0)
		{
			cotter.SetCollectStatistics(false);
		}
		else if(strcmp(argv[argi], "-nogeom") == 0)
		{
			cotter.SetDisableGeometricCorrections(true);
		}
		else if(strcmp(argv[argi], "-noantennapruning") == 0)
		{
			cotter.SetRemoveFlaggedAntennae(false);
		}
		else if(strcmp(argv[argi], "-noautos") == 0)
		{
			cotter.SetRemoveAutoCorrelations(true);
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
		else if(strcmp(argv[argi], "-centre") == 0)
		{
			++argi;
			long double centreRA = RaDecCoord::ParseRA(argv[argi]);
			++argi;
			long double centreDec = RaDecCoord::ParseDec(argv[argi]);
			cotter.SetOverridePhaseCentre(centreRA, centreDec);
		}
		else if(strcmp(argv[argi], "-sbcount") == 0)
		{
			++argi;
			cotter.SetSubbandCount(atoi(argv[argi]));
		}
		else if(strcmp(argv[argi], "-sbpassband") == 0)
		{
			++argi;
			cotter.SetReadSubbandPassbandFile(argv[argi]);
		}
		else if(argv[argi][0] == '-')
		{
			std::cout << "Unknown command line option: " << argv[argi] << '\n';
			return -1;
		}
		else {
			unsortedFiles.push_back(argv[argi]);
		}
		
		++argi;
	}
	
	if(argc == 1 || unsortedFiles.empty())
	{
		usage();
		return -1;
	}
	
	size_t gpuBoxCount = 0;
	std::vector<std::vector<std::string> > fileSets;
	for(std::vector<std::string>::const_iterator i=unsortedFiles.begin(); i!=unsortedFiles.end(); ++i)
	{
		checkInputFilename(*i);
		size_t gpuNum = gpuBoxNumberFromFilename(*i);
		size_t timeNum = timeStepNumberFromFilename(*i);
		if(fileSets.size() <= timeNum)
			fileSets.resize(timeNum+1);
		std::vector<std::string> &timestepSets = fileSets[timeNum];
		if(timestepSets.size() <= gpuNum)
			timestepSets.resize(gpuNum+1);
		if(!timestepSets[gpuNum].empty())
		{
			std::ostringstream errmsg;
			errmsg << "Two files in the list describe the same raw gpu box file: did you specify the same file more than once?\nGPU box number: " << (gpuNum+1)
				<< " time range index: " << timeNum;
			throw std::runtime_error(errmsg.str());
		}
		timestepSets[gpuNum] = *i;
		if(gpuNum+1 > gpuBoxCount) gpuBoxCount = gpuNum+1;
	}
	
	for(size_t j=0; j!=fileSets.size(); ++j)
	{
		for(size_t i=0; i!=gpuBoxCount; ++i)
		{
			if(i >= fileSets[j].size() || fileSets[j][i].empty()) {
				std::ostringstream errstr;
				std::cout << "Missing information from GPU box " << (i+1) << ", timerange " << j << ". Maybe you are missing an input file?\n";
			}
		}
	}
	std::cout << "Input filenames succesfully parsed: using " << unsortedFiles.size() << " files covering " << fileSets.size() << " timeranges from " << gpuBoxCount << " GPU boxes.\n";
	
	std::ostringstream commandLineStr;
	commandLineStr << argv[0];
	for(int i=1; i!= argc; ++i)
		commandLineStr << ' ' << '\"' << argv[i] << '\"';
	cotter.SetHistoryInfo(commandLineStr.str());
	
	long int pageCount = sysconf(_SC_PHYS_PAGES), pageSize = sysconf(_SC_PAGE_SIZE);
	int64_t memSize = (int64_t) pageCount * (int64_t) pageSize;
	double memSizeInGB = (double) memSize / (1024.0*1024.0*1024.0);
	std::cout << "Detected " << round(memSizeInGB*10.0)/10.0 << " GB of system memory.\n";
	
	cotter.SetFileSets(fileSets);
	cotter.SetMaxBufferSize(memSize*memPercentage/(100*(sizeof(float)*2+1)));
	cotter.SetThreadCount(sysconf(_SC_NPROCESSORS_ONLN));
	cotter.Run(outputFilename, timeAvg, freqAvg);
	
	return 0;
}
