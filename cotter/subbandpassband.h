#ifndef SUBBAND_PASSBAND_H
#define SUBBAND_PASSBAND_H

#include <vector>

class SubbandPassband
{
	public:
		static void GetSubbandPassband(std::vector<double> &passband, std::size_t channelsPerSubband);
		
	private:
		const static double _sb128ChannelSubbandValue[128];
		
		static double getSubbandValue(std::size_t channel, std::size_t channelCount);
};

#endif
