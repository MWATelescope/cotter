#ifndef FITSREADER_H
#define FITSREADER_H

#include <string>
#include <vector>

#include <fitsio.h>

#include "polarizationenum.h"

class FitsReader
{
	public:
		FitsReader(const std::string &filename) 
		: _filename(filename), _hasBeam(false)
		{
			initialize(); 
		}
		~FitsReader();
		
		template<typename NumType> void Read(NumType *image);
		
		size_t ImageWidth() const { return _imgWidth; }
		size_t ImageHeight() const { return _imgHeight; }
		
		double PhaseCentreRA() const { return _phaseCentreRA; }
		double PhaseCentreDec() const { return _phaseCentreDec; }
		
		double PixelSizeX() const { return _pixelSizeX; }
		double PixelSizeY() const { return _pixelSizeY; }
		
		double PhaseCentreDL() const { return _phaseCentreDL; }
		double PhaseCentreDM() const { return _phaseCentreDM; }
		
		double Frequency() const { return _frequency; }
		double Bandwidth() const { return _bandwidth; }
		
		double DateObs() const { return _dateObs; }
		PolarizationEnum Polarization() const { return _polarization; }
		
		bool HasBeam() const { return _hasBeam; }
		double BeamMajorAxisRad() const { return _beamMajorAxisRad; }
		double BeamMinorAxisRad() const { return _beamMinorAxisRad; }
		double BeamPositionAngle() const { return _beamPositionAngle; }
		
		const std::string& Origin() const { return _origin; }
		const std::string& OriginComment() const { return _originComment; }
		
		const std::vector<std::string>& History() const { return _history; }
		
		bool ReadDoubleKeyIfExists(const char* key, double& dest) { return readDoubleKeyIfExists(key, dest); }
	private:
		float readFloatKey(const char* key);
		double readDoubleKey(const char* key);
		bool readFloatKeyIfExists(const char* key, float& dest);
		bool readDoubleKeyIfExists(const char* key, double& dest);
		std::string readStringKey(const char* key);
		bool readStringKeyIfExists(const char* key, std::string& value, std::string& comment);
		void readHistory();
		double parseFitsDateToMJD(const char* valueStr);
		bool readDateKeyIfExists(const char *key, double &dest);
		
		std::string _filename;
		fitsfile *_fitsPtr;
		
		void initialize();
		void checkStatus(int status, const std::string &operation=std::string());
		
		size_t _imgWidth, _imgHeight;
		double _phaseCentreRA, _phaseCentreDec;
		double _pixelSizeX, _pixelSizeY;
		double _phaseCentreDL, _phaseCentreDM;
		double _frequency, _bandwidth, _dateObs;
		bool _hasBeam;
		double _beamMajorAxisRad, _beamMinorAxisRad, _beamPositionAngle;
		
		PolarizationEnum _polarization;
		std::string _origin, _originComment;
		std::vector<std::string> _history;
};

#endif
