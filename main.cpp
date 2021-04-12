#include "cotter.h"
#include "numberlist.h"
#include "radeccoord.h"
#include "version.h"

#include <aoflagger.h>

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <vector>

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
	"                     If the files' extension is .uvfits, it will be outputted in uvfits format\n"
	"                     and extension .mwaf is the flag-only format for input into the RTS.\n"
	"  -m <filename>      Read meta data from given fits filename..\n"
	"  -a <filename>      Read antenna locations from given text file (overrides the metadata).\n"
	"  -h <filename>      Read header data from given text file (overrides the metadata.)\n"
	"  -i <filename>      Read meta data from given fits filename (overrides the metadata).\n"
	"  -mem <percentage>  Use at most the given percentage of memory.\n"
	"  -absmem <gb>       Use at most the given amount of memory, specified in gigabytes.\n"
	"  -j <ncpus>         Number of CPUs to use. Default is to use all.\n"
	"  -timeres <s>       Average nr of sec of timesteps together before writing to measurement set.\n"
	"  -freqres <kHz>     Average kHz bandwidth of channels together before writing to measurement set.\n"
	"                     When averaging: flagging, collecting statistics and cable length fixes are done\n"
	"                     at highest resolution. UVW positions are recalculated for new timesteps.\n"
	"  -norfi             Disable RFI detection.\n"
	"  -nostats           Disable collecting statistics (default for uvfits file output).\n"
	"  -nogeom            Disable geometric corrections.\n"
	"  -noalign           Do not align GPU boxes according to the time in their header.\n"
	"  -noantennapruning  Do not remove the flagged antennae.\n"
	"  -noautos           Do not output auto-correlations.\n"
	"  -noflagautos       Do not flag auto-correlations (default for uvfits file output).\n"
	"  -nosbgains         Do not correct for the digital gains.\n"
	"  -noflagmissings    Do not flag missing gpu box files (only makes sense with -allowmissing).\n"
	"  -allowmissing      Do not abort when not all GPU box files are available (default is to abort).\n"
	"  -flagdcchannels    Flag the centre channel of each sub-band (currently the default).\n"
	"  -noflagdcchannels  Do not flag the centre channel of each sub-band.\n"
    "  -nocablelength     Do not perform cable length corrections.\n"
	"  -centre <ra> <dec> Set alternative phase centre, e.g. -centre 00h00m00.0s 00d00m00.0s.\n"
	"  -usepcentre        Centre on pointing centre.\n"
	"  -sbcount <count>   Read/processes the first given number of sub-bands.\n"
	"  -sbstart <number>  Number of first GPU box. Default: 1.\n"
	"  -sbpassband <file> Read the sub-band passband from given file instead of using default passband.\n"
	"                     (default passband does a reasonably good job)\n"
	"  -flagantenna <lst> Mark the comma-separated list of zero-indexed antennae as flagged antennae.\n"
	"  -flagsubband <lst> Flag the comma-separated list of zero-indexed sub-bands.\n"
	"  -edgewidth <kHz>   Flag the given width of edge channels of each sub-band (default: 80 kHz).\n"
	"  -initflag <sec>    Specify number of seconds to flag at beginning of observation (default: 4s).\n"
	"  -endflag <sec>     Specify number of seconds to flag extra at end of observation (default: 0s).\n"
	"  -flagfiles <name>  Use previously writted MWAF files to skip RFI detection. Name should have\n"
	"                     two percentage symbols (%%), which will be replaced by GPU numbers.\n"
	"  -saveqs <file.qs>  Save the quality statistics to the specified file. Use extension of '.qs'.\n"
	"  -histograms        Also collect 'log N log S' histograms of the visibilities (slower).\n"
	"                     These will be stored in the quality statistics tables viewable with aoqplot.\n"
	"  -offline-gpubox-format Assume the GPU Box do not have an initial HDU for metadata. This is\n"
	"                     used for offline correlation of VCS observations.\n"
	"  -skipwrite         Skip the writing step completely: only collect statistics.\n"
	"  -apply <file>      Apply a solution file after averaging. The solution file should have as many\n"
	"                     channels as that the observation will have after the given averaging settings.\n"
	"  -full-apply <file> Apply a solution file before averaging. The solution file should have as many\n"
	"                     channels as the observation.\n"
	"  -flag-strategy <file> Use the specified aoflagger strategy.\n"
	"  -use-dysco         Compress the Measurement Set using Dysco.\n"
	"  -dysco-config <data bits> <weight bits> <distribution> <truncation> <normalization>\n"
	"                     Set advanced Dysco options.\n"
	"  -version           Output version and exit.\n"
	"\n"
	"The filenames of the input gpu files should end in '...nn_mm.fits', where nn >= 1 is the\n"
	"gpu box number and mm >= 0 is the time step number.\n";
}

int cotterMain(int argc, const char* const* argv);

int main(int argc, char **argv)
{
	std::cout << "Running Cotter MWA preprocessing pipeline, version " << COTTER_VERSION_STR <<
		" (" << COTTER_VERSION_DATE << ").\n"
		"Flagging is performed by AOFlagger " << aoflagger::AOFlagger::GetVersionString() << " (" << aoflagger::AOFlagger::GetVersionDate() << ").\n";
	
	int result = 0;
	try {
		result = cotterMain(argc, argv);
	} catch(std::exception &e)
	{
		std::cerr << "\nAn unhandled exception occured while running Cotter:\n" << e.what() << '\n';
		return -1;
	}
	return result;
}

int cotterMain(int argc, const char* const* argv)
{
	std::vector<std::string> unsortedFiles;
	int argi = 1;
	double freqRes = 0.0, timeRes = 0.0;
	double memPercentage = 90.0, memLimit = 0.0;
	Cotter cotter;
	const char *outputFilename = 0;
	bool saveQualityStatistics = false;
	bool allowMissingFiles = false;
	size_t nCPUs = 0, sbStart = 1;
	while(argi!=argc)
	{
		if(argv[argi][0] == '-')
		{
			const std::string param = &argv[argi][1];
			if(param == "version" || param == "-version")
			{
				// Version is already outputted, so just exit
				return 0;
			}
			if(param == "o")
			{
				++argi;
				outputFilename = argv[argi];
				if(isFitsFile(outputFilename))
				{
					cotter.SetCollectStatistics(saveQualityStatistics);
					cotter.SetFlagAutoCorrelations(false);
					cotter.SetOutputFormat(Cotter::FitsOutputFormat);
				}
				else if(isMWAFlagFile(outputFilename))
				{
					cotter.SetCollectStatistics(saveQualityStatistics);
					cotter.SetOutputFormat(Cotter::FlagsOutputFormat);
					cotter.SetRemoveFlaggedAntennae(false);
				}
			}
			else if(param == "m")
			{
				++argi;
				cotter.SetMetaFilename(argv[argi]);
			}
			else if(param == "a")
			{
				++argi;
				cotter.SetAntennaLocationsFilename(argv[argi]);
			}
			else if(param == "h")
			{
				++argi;
				cotter.SetHeaderFilename(argv[argi]);
			}
			else if(param == "i")
			{
				++argi;
				cotter.SetInstrConfigFilename(argv[argi]);
			}
			else if(param == "j")
			{
				++argi;
				nCPUs = atoi(argv[argi]);
			}
			else if(param == "mem")
			{
				++argi;
				memPercentage = atof(argv[argi]);
			}
			else if(param == "absmem")
			{
				++argi;
				memLimit = atof(argv[argi]);
			}
			else if(param == "noflagautos")
			{
				cotter.SetFlagAutoCorrelations(false);
			}
			else if(param == "norfi")
			{
				cotter.SetRFIDetection(false);
			}
			else if(param == "nostats")
			{
				cotter.SetCollectStatistics(false);
			}
			else if(param == "histograms")
			{
				cotter.SetCollectHistograms(true);
			}
			else if(param == "nohistograms")
			{
				cotter.SetCollectHistograms(false);
			}
			else if(param == "nogeom")
			{
				cotter.SetDisableGeometricCorrections(true);
			}
			else if(param == "noalign")
			{
				cotter.SetDoAlign(false);
			}
			else if(param == "noantennapruning")
			{
				cotter.SetRemoveFlaggedAntennae(false);
			}
			else if(param == "noautos")
			{
				cotter.SetRemoveAutoCorrelations(true);
			}
			else if(param == "nosbgains")
			{
				cotter.SetApplySBGains(false);
			}
			else if(param == "noflagmissings")
			{
				cotter.SetDoFlagMissingSubbands(false);
			}
			else if(param == "allowmissing")
			{
				allowMissingFiles = true;
			}
			else if(param == "flagdcchannels")
			{
				cotter.SetFlagDCChannels(true);
			}
			else if(param == "noflagdcchannels")
			{
				cotter.SetFlagDCChannels(false);
			}
			else if(param == "nocablelength")
			{
				cotter.SetDoCorrectCableLength(false);
			}
			else if(param == "timeres")
			{
				++argi;
				timeRes = atof(argv[argi]);
			}
			else if(param == "freqres")
			{
				++argi;
				freqRes = atof(argv[argi]);
			}
			else if(param == "centre")
			{
				++argi;
				long double centreRA = RaDecCoord::ParseRA(argv[argi]);
				++argi;
				long double centreDec = RaDecCoord::ParseDec(argv[argi]);
				cotter.SetOverridePhaseCentre(centreRA, centreDec);
			}
			else if(param == "usepcentre")
			{
				cotter.SetUsePointingCentre(true);
			}
			else if(param == "sbcount")
			{
				++argi;
				cotter.SetSubbandCount(atoi(argv[argi]));
			}
			else if(param == "sbstart")
			{
				++argi;
				sbStart = atoi(argv[argi]);
			}
			else if(param == "sbpassband")
			{
				++argi;
				cotter.SetReadSubbandPassbandFile(argv[argi]);
			}
			else if(param == "initflag")
			{
				++argi;
				cotter.SetInitDurationToFlag(atof(argv[argi]));
			}
			else if(param == "endflag")
			{
				++argi;
				cotter.SetEndDurationToFlag(atof(argv[argi]));
			}
			else if(param == "flagantenna")
			{
				++argi;
				std::vector<int> antennaList;
				NumberList::ParseIntList(argv[argi], antennaList);
				std::cout << "Flagging antennae: ";
				for(std::vector<int>::const_iterator i=antennaList.begin(); i!=antennaList.end(); ++i)
				{
					cotter.FlagAntenna(*i);
					std::cout << *i << ' ';
				}
				std::cout << "\n";
			}
			else if(param == "flagsubband")
			{
				++argi;
				std::vector<int> sbList;
				NumberList::ParseIntList(argv[argi], sbList);
					std::cout << "Flagging sub-bands: ";
				for(std::vector<int>::const_iterator i=sbList.begin(); i!=sbList.end(); ++i)
				{
					cotter.FlagSubband(*i);
					std::cout << *i << ' ';
				}
				std::cout << "\n";
			}
			else if(param == "edgewidth")
			{
				++argi;
				cotter.SetSubbandEdgeFlagWidth(atoi(argv[argi]));
			}
			else if(param == "flagfiles")
			{
				++argi;
				cotter.SetRFIDetection(false);
				cotter.SetFlagFileTemplate(argv[argi]);
				if(memLimit == 0.0)
					memLimit = 2.0;
			}
			else if(param == "saveqs")
			{
				++argi;
				cotter.SetSaveQualityStatistics(argv[argi]);
				cotter.SetCollectStatistics(true);
				saveQualityStatistics = true;
			}
			else if(param == "skipwrite")
			{
				cotter.SetSkipWriting(true);
			}
			else if(param == "offline-gpubox-format")
			{
				cotter.SetOfflineGPUBoxFormat(true);
			}
			else if(param == "apply")
			{
				++argi;
				cotter.SetSolutionFile(argv[argi]);
				cotter.SetApplyBeforeAveraging(false);
			}
			else if(param == "full-apply")
			{
				++argi;
				cotter.SetSolutionFile(argv[argi]);
				cotter.SetApplyBeforeAveraging(true);
			}
			else if(param == "flag-strategy")
			{
				++argi;
				cotter.SetStrategyFile(argv[argi]);
			}
			else if(param == "use-dysco")
			{
				cotter.SetUseDysco(true);
			}
			else if(param == "dysco-config")
			{
				cotter.SetAdvancedDyscoOptions(atoi(argv[argi+1]), atoi(argv[argi+2]), argv[argi+3], atof(argv[argi+4]), argv[argi+5]);
				argi += 5;
			}
			else
			{
				std::cout << "Unknown command line option: " << argv[argi] << '\n';
				return -1;
			}
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
		size_t gpuFilenameNumber = gpuBoxNumberFromFilename(*i);
		if(gpuFilenameNumber < sbStart-1)
		{
			std::ostringstream errmsg;
			errmsg << "A GPU box file was specified with number " << (gpuFilenameNumber+1) << ", which is lower than the expected start number of " << sbStart << ".";
			throw std::runtime_error(errmsg.str());
		}
		size_t gpuNum = gpuFilenameNumber - sbStart + 1;
		size_t timeNum = timeStepNumberFromFilename(*i);
		if(fileSets.size() <= timeNum)
			fileSets.resize(timeNum+1);
		std::vector<std::string> &timestepSets = fileSets[timeNum];
		if(timestepSets.size() <= gpuNum)
			timestepSets.resize(gpuNum+1);
		if(!timestepSets[gpuNum].empty())
		{
			std::ostringstream errmsg;
			errmsg << "Two files in the list describe the same raw gpu box file: did you specify the same file more than once?\nGPU box number: " << (gpuNum+sbStart)
				<< " time range index: " << timeNum;
			throw std::runtime_error(errmsg.str());
		}
		timestepSets[gpuNum] = *i;
		if(gpuNum+1 > gpuBoxCount) gpuBoxCount = gpuNum+1;
	}
	
	bool aFileIsMissing = false;
	for(size_t j=0; j!=fileSets.size(); ++j)
	{
		for(size_t i=0; i!=cotter.SubbandCount(); ++i)
		{
			if(i >= fileSets[j].size() || fileSets[j][i].empty()) {
				std::ostringstream errstr;
				std::cout << "Missing information from GPU box " << (i+sbStart) << ", timerange " << j << ". Maybe you are missing an input file?\n";
				aFileIsMissing = true;
			}
		}
	}
	if(aFileIsMissing && !allowMissingFiles)
	{
		throw std::runtime_error("Because at least one input file is missing, I will refuse to continue. This is to prevent download errors cause incomplete observations. If you are missing input files because some of the files are not available (e.g. because of a correlator GPU box failure), specify '-allowmissing' to continue with missing files.");
	}
	if(gpuBoxCount > cotter.SubbandCount())
	{
		std::ostringstream errstr;
		errstr << "The highest GPU box number (" << gpuBoxCount << ") is higher than the number of subbands (" << cotter.SubbandCount() << "). Either a wrong -sbcount was specified, or files are not correctly named.";
		throw std::runtime_error(errstr.str());
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
	if(memLimit == 0.0)
		std::cout << "Detected " << round(memSizeInGB*10.0)/10.0 << " GB of system memory.\n";
	else {
		std::cout << "Using " << round(memLimit*10.0)/10.0 << '/' << round(memSizeInGB*10.0)/10.0 << " GB of system memory.\n";
		memSize = int64_t(memLimit * (1024.0*1024.0*1024.0));
		memPercentage = 100.0;
	}
	
	cotter.SetFileSets(fileSets);
	cotter.SetMaxBufferSize(memSize*memPercentage/(100*(sizeof(float)*2+1)));
	if(nCPUs == 0)
		cotter.SetThreadCount(sysconf(_SC_NPROCESSORS_ONLN));
	else
		cotter.SetThreadCount(nCPUs);
	if(outputFilename != 0)
		cotter.SetOutputFilename(outputFilename);
	cotter.Run(timeRes, freqRes);
	
	return 0;
}
