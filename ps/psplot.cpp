#include "psdatafile.h"
#include "gnuplot.h"
#include "channeltransform.h"
#include "perptransform.h"
#include "fitswriter.h"

#include <iostream>
#include <vector>

void psplot(const std::string& filename)
{
	const size_t perpGridSize = 2048, parGridSize = 1024;
	
	PSDataFile file(filename);
	ChannelTransform transform(perpGridSize);
	ChannelTransform::TransformedChannel transformedData;
	GNUPlot
		plot("perp-power", "Distance from centre (deg)", "Brightness (mK)"),
		weightPlot("perp-weight", "Distance from centre (deg)", "Weight"),
		tPlot("kperp-power", "k-perp (1/deg)", "Brightness (mK)", true);
	
	std::vector<PSDataFile::ChannelData> allInputData(file.ChannelCount());
	std::vector<PerpTransform::PerpData> allTransformedData;
	
	double xPerpStart = 0.0, xPerpStep = 0.0;
	
	for(size_t ch=0; ch!=file.ChannelCount(); ++ch)
	{
		PSDataFile::ChannelData& data = allInputData[ch];
		data = file.Channel(ch);
		double scaleSize = data.values.size() * data.xPerpStep + data.xPerpStart;
		std::cout << "Channel " << (ch+1) << " (" << data.frequency*1e-6 << " MHz, " << round(scaleSize*(1800.0/M_PI))*0.1 << " deg) "<< std::flush;
		
		if(ch == 0)
		{
			allTransformedData.resize(data.values.size());
			for(size_t ui=0; ui!=allTransformedData.size(); ++ui)
			{
				allTransformedData[ui].values.resize(file.ChannelCount());
				allTransformedData[ui].weights.resize(file.ChannelCount());
			}
			xPerpStart = data.xPerpStart;
			xPerpStep = data.xPerpStep;
		}
		else {
			if(data.xPerpStart != xPerpStart || data.xPerpStep != xPerpStep || data.values.size() > allTransformedData.size())
				throw std::runtime_error("Channels are not gridded on same grid");
		}
		
		std::ostringstream lineName, lineCaption;
		lineName << "perp-power-ch" << ch << ".txt";
		lineCaption << (data.frequency*1e-6) << " MHz";
		GNUPlot::Line* line = plot.AddLine(lineName.str(), lineCaption.str());
		GNUPlot::Line* weightLine = weightPlot.AddLine(std::string("w") + lineName.str(), lineCaption.str());
		for(size_t i=0; i!=data.values.size(); ++i)
		{
			double dist = data.xPerpStart + data.xPerpStep * i;
			if(data.weights[i] != 0.0)
			{
				double power = data.values[i] / data.weights[i];
				line->AddPoint(dist, power);
			}
			weightLine->AddPoint(dist, data.weights[i]);
		}
		
		std::cout << "Transforming... " << std::flush;
		
		transform.Transform(transformedData, data);
		std::ostringstream tLineName, tLineCaption;
		tLineName << "kperp-power-ch" << ch << ".txt";
		tLineCaption << (data.frequency*1e-6) << " MHz";
		GNUPlot::Line* tLine = tPlot.AddLine(tLineName.str(), tLineCaption.str());
		for(size_t ui=0; ui!=transformedData.values.size(); ++ui)
		{
			double u = transformedData.uStep * ui;
			if(transformedData.weights[ui] != 0.0)
			{
				double power = transformedData.values[ui];
				tLine->AddPoint(u, power);
			}
			allTransformedData[ui].values[ch] = transformedData.values[ui];
			if(std::isfinite(allTransformedData[ui].values[ch]))
				allTransformedData[ui].weights[ch] = transformedData.weights[ui];
			else
				allTransformedData[ui].weights[ch] = 0.0;
		}
			
		for(size_t ui=transformedData.values.size(); ui!=allTransformedData.size(); ++ui)
		{
			allTransformedData[ui].values[ch] = 0.0;
			allTransformedData[ui].weights[ch] = 0.0;
		}
		
		std::cout << '\n';
	}
	
	ao::uvector<double>
		spectrumImage(perpGridSize * parGridSize),
		channelImage(perpGridSize * file.ChannelCount()),
		channelWeightImage(perpGridSize * file.ChannelCount());
	
	std::cout << "Transforming to " << perpGridSize << " x " << parGridSize << " spectra...\n";
	PerpTransform perpTransform(parGridSize);
	for(size_t ui=0; ui!=perpGridSize; ++ui)
	{
		const PerpTransform::PerpData& data = allTransformedData[ui];
		
		ao::uvector<double>::iterator
			imagePtr = channelImage.begin() + ui,
			weightPtr = channelWeightImage.begin() + ui;
		for(size_t y=0; y!=file.ChannelCount(); ++y)
		{
			*imagePtr = data.values[y];
			imagePtr += perpGridSize;
			*weightPtr = data.weights[y];
			weightPtr += perpGridSize;
		}
		
		PerpTransform::PerpData pTransformedData;
		perpTransform.Transform(pTransformedData, data);
		
		imagePtr = spectrumImage.begin() + ui;
		for(size_t y=0; y!=parGridSize; ++y)
		{
			*imagePtr = pTransformedData.values[y];
			imagePtr += perpGridSize;
		}
	}
	
	FitsWriter writer;
	writer.SetImageDimensions(perpGridSize, file.ChannelCount());
	writer.Write("channels.fits", channelImage.data());
	for(ao::uvector<double>::iterator i=channelImage.begin(); i!=channelImage.end(); ++i)
		*i = log(fabs(*i));
	writer.Write("channels-log.fits", channelImage.data());
	
	writer.SetImageDimensions(perpGridSize, file.ChannelCount());
	writer.Write("channels-weights.fits", channelWeightImage.data());
	for(ao::uvector<double>::iterator i=channelWeightImage.begin(); i!=channelWeightImage.end(); ++i)
		*i = log(fabs(*i));
	writer.Write("channels-weights-log.fits", channelWeightImage.data());
	
	writer.SetImageDimensions(perpGridSize, parGridSize);
	writer.Write("spectrum.fits", spectrumImage.data());
	for(ao::uvector<double>::iterator i=spectrumImage.begin(); i!=spectrumImage.end(); ++i)
		*i = log(fabs(*i));
	writer.Write("spectrum-log.fits", spectrumImage.data());
}

int main(int argc, char* argv[])
{
	if(argc!=2)
		std::cout << "Syntax: psplot [name]\n";
	else {
		psplot(argv[1]);
	}
}
