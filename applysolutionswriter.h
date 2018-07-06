#ifndef APPLYCAL_WRITER_H
#define APPLYCAL_WRITER_H

#include "forwardingwriter.h"
#include "matrix2x2.h"

#include <memory>
#include <string>
#include <vector>

class ApplySolutionsWriter : public ForwardingWriter
{
	public:
		ApplySolutionsWriter(std::unique_ptr<Writer> parentWriter, const std::string& filename);
		
		virtual ~ApplySolutionsWriter() final override;
		
		virtual void WriteBandInfo(const std::string& name, const std::vector<Writer::ChannelInfo>& channels, double refFreq, double totalBandwidth, bool flagRow) final override;
		
		virtual void WriteRow(double time, double timeCentroid, size_t antenna1, size_t antenna2, double u, double v, double w, double interval, const std::complex<float>* data, const bool* flags, const float *weights) final override;
		
	private:
		size_t _nChannels, _nSolutionAntennas, _nSolutionChannels;
		std::vector<std::complex<float>> _correctedData;
		std::vector<MC2x2> _solutions;
};

#endif
