#ifndef FITSWRITER_H
#define FITSWRITER_H

#include <string>
#include <vector>
#include <map>
#include <cmath>

#include "polarizationenum.h"

class FitsWriter
{
	public:
		FitsWriter() :
			_width(0), _height(0),
			_phaseCentreRA(0.0), _phaseCentreDec(0.0), _pixelSizeX(0.0), _pixelSizeY(0.0),
			_phaseCentreDL(0.0), _phaseCentreDM(0.0),
			_frequency(0.0), _bandwidth(0.0),
			_dateObs(0.0),
			_hasBeam(false),
			_beamMajorAxisRad(0.0), _beamMinorAxisRad(0.0), _beamPositionAngle(0.0),
			_polarization(Polarization::StokesI),
			_origin("AO/WSImager"), _originComment("Imager written by Andre Offringa")
		{
		}
		
		FitsWriter(const class FitsReader& reader) :
			_width(0), _height(0),
			_phaseCentreRA(0.0), _phaseCentreDec(0.0), _pixelSizeX(0.0), _pixelSizeY(0.0),
			_phaseCentreDL(0.0), _phaseCentreDM(0.0),
			_frequency(0.0), _bandwidth(0.0),
			_dateObs(0.0),
			_hasBeam(false),
			_beamMajorAxisRad(0.0), _beamMinorAxisRad(0.0), _beamPositionAngle(0.0),
			_polarization(Polarization::StokesI),
			_origin("AO/WSImager"), _originComment("Imager written by Andre Offringa")
		{
			SetMetadata(reader);
		}
		
		template<typename NumType> void Write(const std::string& filename, const NumType* image) const;
		
		void SetBeamInfo(double widthRad)
		{
			SetBeamInfo(widthRad, widthRad, 0.0);
		}
		void SetBeamInfo(double majorAxisRad, double minorAxisRad, double positionAngleRad)
		{
			_hasBeam = true;
			_beamMajorAxisRad = majorAxisRad;
			_beamMinorAxisRad = minorAxisRad;
			_beamPositionAngle = positionAngleRad;
		}
		void SetImageDimensions(size_t width, size_t height)
		{
			_width = width;
			_height = height;
		}
		void SetImageDimensions(size_t width, size_t height, double phaseCentreRA, double phaseCentreDec, double pixelSizeX, double pixelSizeY)
		{
			_width = width;
			_height = height;
			_phaseCentreRA = phaseCentreRA;
			_phaseCentreDec = phaseCentreDec;
			_pixelSizeX = pixelSizeX;
			_pixelSizeY = pixelSizeY;
		}
		void SetFrequency(double frequency, double bandwidth)
		{
			_frequency = frequency;
			_bandwidth = bandwidth;
		}
		void SetDate(double dateObs)
		{
			_dateObs = dateObs;
		}
		void SetPolarization(PolarizationEnum polarization)
		{
			_polarization = polarization;
		}
		void SetOrigin(const std::string& origin, const std::string& comment)
		{
			_origin = origin;
			_originComment = comment;
		}
		void SetHistory(const std::vector<std::string>& history)
		{
			_history = history;
		}
		void AddHistory(std::string& historyLine)
		{
			_history.push_back(historyLine);
		}
		void SetMetadata(const class FitsReader& reader);
		
		double RA() const { return _phaseCentreRA; }
		double Dec() const { return _phaseCentreDec; }
		double Frequency() const { return _frequency; }
		double Bandwidth() const { return _bandwidth; }
		double BeamSizeMajorAxis() const { return _beamMajorAxisRad; }
		
		void SetExtraKeyword(const std::string& name, const std::string& value)
		{
			if(_extraStringKeywords.count(name) != 0)
				_extraStringKeywords.erase(name);
			_extraStringKeywords.insert(std::make_pair(name, value));
		}
		void SetExtraKeyword(const std::string& name, double value)
		{
			if(_extraNumKeywords.count(name) != 0)
				_extraNumKeywords.erase(name);
			_extraNumKeywords.insert(std::make_pair(name, value));
		}
		void SetExtraStringKeywords(const std::map<std::string, std::string>& keywords)
		{
			_extraStringKeywords = keywords;
		}
		void SetExtraNumKeywords(const std::map<std::string, double>& keywords)
		{
			_extraNumKeywords = keywords;
		}
		void SetPhaseCentreShift(double dl, double dm)
		{
			_phaseCentreDL = dl;
			_phaseCentreDM = dm;
		}
		size_t Width() const { return _width; }
		size_t Height() const { return _height; }
	private:
		template<typename T>
		static T setNotFiniteToZero(T num)
		{
			return std::isfinite(num) ? num : 0.0;
		}
		std::size_t _width, _height;
		double _phaseCentreRA, _phaseCentreDec, _pixelSizeX, _pixelSizeY;
		double _phaseCentreDL, _phaseCentreDM;
		double _frequency, _bandwidth;
		double _dateObs;
		bool _hasBeam;
		double _beamMajorAxisRad, _beamMinorAxisRad, _beamPositionAngle;
		PolarizationEnum _polarization;
		std::string _origin, _originComment;
		std::vector<std::string> _history;
		std::map<std::string, std::string> _extraStringKeywords;
		std::map<std::string, double> _extraNumKeywords;
		
		void checkStatus(int status, const std::string& filename) const;
		void julianDateToYMD(double jd, int &year, int &month, int &day) const;
		void mjdToHMS(double mjd, int& hour, int& minutes, int& seconds, int& deciSec) const;
};

#endif
