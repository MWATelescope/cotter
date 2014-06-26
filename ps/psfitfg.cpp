#include "psdatafile.h"
#include "gnuplot.h"
#include "channeltransform.h"
#include "perptransform.h"
#include "fitswriter.h"
#include "nlplfitter.h"

#include <iostream>
#include <vector>

void psfitfg(const std::string& inputFilename, const std::string& signalOut, const std::string& foregroundOut)
{
	PSDataFile file(inputFilename), model(file);
	std::vector<PSDataFile::ChannelData> allInputData(file.ChannelCount());
	
	double xPerpStart = 0.0, xPerpStep = 0.0;
	size_t maxValues = 0;
	
	for(size_t ch=0; ch!=file.ChannelCount(); ++ch)
	{
		PSDataFile::ChannelData& data = allInputData[ch];
		data = file.Channel(ch);
		if(ch == 0)
		{
			xPerpStart = data.xPerpStart;
			xPerpStep = data.xPerpStep;
		}
		else {
			if(data.xPerpStart != xPerpStart || data.xPerpStep != xPerpStep)
				throw std::runtime_error("Channels are not gridded on same grid");
		}
		if(data.values.size() > maxValues) maxValues = data.values.size();
	}
	
	std::cout << "Subtracting foregrounds... " << std::flush;
	double expSum = 0.0, facSum = 0.0;
	size_t sumCount = 0;
	for(size_t ui=0; ui!=maxValues; ++ui)
	{
		size_t sampleCount = 0;
		NonLinearPowerLawFitter nlplFitter;
		for(size_t ch=0; ch!=file.ChannelCount(); ++ch)
		{
			if(ui < allInputData[ch].values.size() && std::isfinite(allInputData[ch].values[ui]))
			{
				double
					v = allInputData[ch].values[ui],
					w = allInputData[ch].weights[ui];
				if(w != 0.0)
				{
					nlplFitter.AddDataPoint(allInputData[ch].frequency, v/w, w);
					++sampleCount;
				}
			}
		}
		if(sampleCount > 2)
		{
			double fitExp = -1.0, fitFac = 1.0;
			nlplFitter.Fit(fitExp, fitFac);
			for(size_t ch=0; ch!=file.ChannelCount(); ++ch)
			{
				if(ui < allInputData[ch].values.size() && std::isfinite(allInputData[ch].values[ui]) && allInputData[ch].weights[ui] != 0.0) {
					double foreground = fitFac * pow(allInputData[ch].frequency, fitExp);
					if(std::isfinite(foreground) && file.Channel(ch).weights[ui] != 0.0)
					{
						file.Channel(ch).values[ui] -= foreground * allInputData[ch].weights[ui];
						model.Channel(ch).values[ui] = foreground * allInputData[ch].weights[ui];
					}
				}
			}
			std::cout << "Fit: " << fitFac << " nu^x " << fitExp << "\n";
			++sumCount;
			expSum += fitExp;
			facSum += fitFac;
		}
	}
	std::cout << "average fit: " << facSum/sumCount << " nu^x " << expSum/sumCount << "\n";
	file.Save(signalOut);
	model.Save(foregroundOut);
}

int main(int argc, char* argv[])
{
	if(argc!=4)
		std::cout << "Syntax: psfitfg <input> <signal output file> <foreground output file> \n";
	else {
		psfitfg(argv[1], argv[2], argv[3]);
	}
}
