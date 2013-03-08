#ifndef BANDDATA_H
#define BANDDATA_H

#include <stdexcept>

#include <ms/MeasurementSets/MeasurementSet.h>

#include <tables/Tables/ArrayColumn.h>
#include <tables/Tables/ScalarColumn.h>

class BandData
{
	public:
		BandData(casa::MSSpectralWindow &spwTable)
		{
			size_t spwCount = spwTable.nrow();
			if(spwCount != 1) throw std::runtime_error("Set should have exactly one spectral window");
			
			casa::ROScalarColumn<int> numChanCol(spwTable, casa::MSSpectralWindow::columnName(casa::MSSpectralWindowEnums::NUM_CHAN));
			_channelCount = numChanCol.get(0);
			if(_channelCount == 0) throw std::runtime_error("No channels in set");
			
			casa::ROArrayColumn<double> chanFreqCol(spwTable, casa::MSSpectralWindow::columnName(casa::MSSpectralWindowEnums::CHAN_FREQ));
			casa::Array<double> channelFrequencies = chanFreqCol.get(0);
			
			_channelFrequencies = new double[_channelCount];
			size_t index = 0;
			for(casa::Array<double>::const_iterator i=channelFrequencies.begin();
					i != channelFrequencies.end(); ++i)
			{
				_channelFrequencies[index] = *i;
				++index;
			}
		}
		
		
		size_t ChannelCount() const { return _channelCount; }
		
		double ChannelFrequency(size_t channelIndex) const
		{
			return _channelFrequencies[channelIndex];
		}
		double ChannelWavelength(size_t channelIndex) const
		{
			return 299792458.0L / _channelFrequencies[channelIndex];
		}
		double HighestFrequency() const
		{
			return _channelFrequencies[_channelCount-1];
		}
		double LowestFrequency() const
		{
			return _channelFrequencies[0];
		}
		double FrequencyStep() const
		{
			return _channelFrequencies[1] - _channelFrequencies[0];
		}
		double BandStart() const
		{
			return LowestFrequency() - FrequencyStep()*0.5;
		}
		double BandEnd() const
		{
			return HighestFrequency() + FrequencyStep()*0.5;
		}
		
	private:
		size_t _channelCount;
		double *_channelFrequencies;
};

#endif
