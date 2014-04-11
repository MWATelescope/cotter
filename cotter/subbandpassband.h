#ifndef SUBBAND_PASSBAND_H
#define SUBBAND_PASSBAND_H

#include <vector>

class SubbandPassband
{
	public:
		static void GetSubbandPassband(std::vector<double> &passband, std::size_t channelsPerSubband);
		
	private:
		/**
		 * See the cpp file for explanation of the difference between these bandpasses.
		 */
		const static double _sb128ChannelSubbandValue2013[128];
		const static double _sb128ChannelSubbandValue2014FromMemo[128];
		
		const static double *_sb128ChannelSubbandValue;
		
		static double getSubbandValue(std::size_t channel, std::size_t channelCount);
};

#endif
