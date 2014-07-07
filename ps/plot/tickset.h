
#include <string>
#include <vector>
#include <cmath>
#include <sstream>

#include "date.h"

typedef std::pair<double, std::string> Tick;

class TickSet
{
	public:
		TickSet()
		{
		}
		virtual ~TickSet()
		{
		}
		
		virtual unsigned Size() const = 0;
		virtual Tick GetTick(unsigned i) const = 0;
		
		virtual void DecreaseTicks()
		{
			if(Size() > 1)
			{
				Set(Size() - 1);
			}
		}
		virtual void Set(unsigned maxSize) = 0;
		virtual void Reset() = 0;
	protected:
	private:
		
};

class NumericTickSet : public TickSet
{
	public:
		NumericTickSet(double min, double max, unsigned sizeRequest) : _min(min), _max(max), _sizeRequest(sizeRequest)
		{
			set(sizeRequest);
		}
		
		virtual unsigned Size() const
		{
			return _ticks.size();
		}
		
		virtual Tick GetTick(unsigned i) const
		{
			std::stringstream tickStr;
			tickStr << _ticks[i];
			return Tick((_ticks[i] - _min) / (_max - _min), tickStr.str());
		}
		
		virtual void Reset()
		{
			_ticks.clear();
			set(_sizeRequest);
		}
		
		virtual void Set(unsigned maxSize)
		{
			_ticks.clear();
			set(maxSize);
		}
	private:
		void set(unsigned sizeRequest)
		{
			if(std::isfinite(_min) && std::isfinite(_max))
			{
				if(_max == _min)
					_ticks.push_back(_min);
				else
				{
					if(sizeRequest == 0)
						return;
					double tickWidth = roundUpToNiceNumber(fabs(_max - _min) / (double) sizeRequest);
					if(tickWidth == 0.0)
						tickWidth = 1.0;
					if(_min < _max)
					{
						double pos = roundUpToNiceNumber(_min, tickWidth);
						while(pos <= _max)
						{
							if(fabs(pos) < tickWidth/100.0)
								_ticks.push_back(0.0);
							else
								_ticks.push_back(pos);
							pos += tickWidth;
						}
					} else {
						double pos = -roundUpToNiceNumber(-_min, tickWidth);
						while(pos >= _max)
						{
							if(fabs(pos) < tickWidth/100.0)
								_ticks.push_back(0.0);
							else
								_ticks.push_back(pos);
							pos -= tickWidth;
						}
					}
					while(_ticks.size() > sizeRequest)
						_ticks.pop_back();
				}
			}
		}
		
		double roundUpToNiceNumber(double number)
		{
			if(!std::isfinite(number))
				return number;
			double roundedNumber = 1.0;
			if(number <= 0.0)
			{
				if(number == 0.0)
					return 0.0;
				else
				{
					roundedNumber = -1.0;
					number *= -1.0;
				}
			}
			while(number > 10)
			{
				number /= 10;
				roundedNumber *= 10;
			}
			while(number <= 1)
			{
				number *= 10;
				roundedNumber /= 10;
			}
			if(number <= 2) return roundedNumber * 2;
			else if(number <= 5) return roundedNumber * 5;
			else return roundedNumber * 10;
		}
		double roundUpToNiceNumber(double number, double roundUnit)
		{
			return roundUnit * ceil(number / roundUnit);
		}
		
		double _min, _max;
		unsigned _sizeRequest;
		std::vector<double> _ticks;
};

class LogarithmicTickSet : public TickSet
{
	public:
		LogarithmicTickSet(double min, double max, unsigned sizeRequest) : _min(min), _minLog10(log10(min)), _max(max), _maxLog10(log10(max)), _sizeRequest(sizeRequest)
		{
			if(!std::isfinite(min) || !std::isfinite(max))
				throw std::runtime_error("Invalid (non-finite) range in LogarithmicTickSet");
			set(sizeRequest);
		}
		
		virtual unsigned Size() const
		{
			return _ticks.size();
		}
		
		virtual Tick GetTick(unsigned i) const
		{
			std::stringstream tickStr;
			tickStr << _ticks[i];
			return Tick((log10(_ticks[i]) - _minLog10) / (_maxLog10 - _minLog10), tickStr.str());
		}
		
		virtual void Reset()
		{
			_ticks.clear();
			set(_sizeRequest);
		}
		
		virtual void Set(unsigned maxSize)
		{
			_ticks.clear();
			set(maxSize);
		}
	private:
		void set(unsigned sizeRequest)
		{
			if(_max == _min)
				_ticks.push_back(_min);
			else
			{
				if(sizeRequest == 0)
					sizeRequest = 1;
				const double
					tickStart = roundUpToBase10Number(_min*0.999),
					tickEnd = roundDownToBase10Number(_max*1.001);
				_ticks.push_back(tickStart);
				if(sizeRequest == 1)
					return;
				if(tickEnd > tickStart)
				{
					const unsigned distance = (unsigned) log10(tickEnd / tickStart);
					const unsigned step = (distance + sizeRequest - 1) / sizeRequest;
					const double factor = exp10((double) step);
					double pos = tickStart * factor;
					while(pos <= tickEnd && _ticks.size() < sizeRequest)
					{
						_ticks.push_back(pos);
						pos *= factor;
					}
				}
				// can we add two to nine?
				if((_ticks.size()+1)*10 < sizeRequest)
				{
					double base = tickStart / 10.0;
					do {
						for(double i=2.0;i<9.5;++i)
						{
							double val = base * i;
							if(val >= _min && val <= _max)
								_ticks.push_back(val);
						}
						base *= 10.0;
					} while(base < _max);
				}
				// can we add two, four, ... eight?
				else if((_ticks.size()+1)*5 < sizeRequest)
				{
					double base = tickStart / 10.0;
					do {
						for(double i=2.0;i<9.0;i+=2.0)
						{
							double val = base * i;
							if(val >= _min && val <= _max)
								_ticks.push_back(val);
						}
						base *= 10.0;
					} while(base < _max);
				}
				// can we add two and five?
				else if((_ticks.size()+1)*3 < sizeRequest)
				{
					double base = tickStart / 10.0;
					do {
						for(double i=2.0;i<6.0;i+=3.0)
						{
							double val = base * i;
							if(val >= _min && val <= _max)
								_ticks.push_back(val);
						}
						base *= 10.0;
					} while(base < _max);
				}
				// can we add two and five?
				else if((_ticks.size()+1)*3 < sizeRequest)
				{
					double base = tickStart / 10.0;
					do {
						for(double i=2.0;i<6.0;i+=3.0)
						{
							double val = base * i;
							if(val >= _min && val <= _max)
								_ticks.push_back(val);
						}
						base *= 10.0;
					} while(base < _max);
				}
				// can we add five?
				else if((_ticks.size()+1)*2 < sizeRequest)
				{
					double base = tickStart / 10.0;
					do {
						double val = base * 5.0;
						if(val >= _min && val <= _max)
							_ticks.push_back(val);
						base *= 10.0;
					} while(base < _max);
				}
				std::sort(_ticks.begin(), _ticks.end());
			}
		}
		
		double roundUpToBase10Number(double number) const
		{
			if(!std::isfinite(number))
				return number;
			const double l = log10(number);
			return exp10(ceil(l));
		}
		
		double roundDownToBase10Number(double number) const
		{
			if(!std::isfinite(number))
				return number;
			const double l = log10(number);
			return exp10(floor(l));
		}
		
		double roundUpToNiceNumber(double number, double roundUnit) const
		{
			return roundUnit * ceil(number / roundUnit);
		}
		
		double _min, _minLog10, _max, _maxLog10;
		unsigned _sizeRequest;
		std::vector<double> _ticks;
};

class TimeTickSet : public TickSet
{
	public:
		TimeTickSet(double minTime, double maxTime, unsigned sizeRequest) : _min(minTime), _max(maxTime), _sizeRequest(sizeRequest)
		{
			if(!std::isfinite(minTime) || !std::isfinite(maxTime))
				throw std::runtime_error("Invalid (non-finite) range in TimeTickSet");
			set(sizeRequest);
		}
		
		virtual unsigned Size() const
		{
			return _ticks.size();
		}
		
		virtual Tick GetTick(unsigned i) const
		{
			double val = _ticks[i];
			return Tick((val - _min) / (_max - _min), Date::AipsMJDToTimeString(val));
		}
		
		virtual void Reset()
		{
			_ticks.clear();
			set(_sizeRequest);
		}
		
		virtual void Set(unsigned maxSize)
		{
			_ticks.clear();
			set(maxSize);
		}
	private:
		void set(unsigned sizeRequest)
		{
			if(_max == _min)
				_ticks.push_back(_min);
			else
			{
				if(sizeRequest == 0)
					return;
				double tickWidth = calculateTickWidth((_max - _min) / (double) sizeRequest);
				if(tickWidth == 0.0 || !std::isfinite(tickWidth))
					tickWidth = 1.0;
				double
					pos = roundUpToNiceNumber(_min, tickWidth);
				while(pos < _max)
				{
					_ticks.push_back(pos);
					pos += tickWidth;
				}
				while(_ticks.size() > sizeRequest)
					_ticks.pop_back();
			}
		}
		
		double calculateTickWidth(double lowerLimit) const
		{
			if(!std::isfinite(lowerLimit))
				return lowerLimit;
			
			// number is in units of seconds
			
			// In days?
			if(lowerLimit >= 60.0*60.0*24.0)
			{
				double width = 60.0*60.0*24.0;
				while(width < lowerLimit)
					width *= 2.0;
				return width;
			}
			// in hours?
			else if(lowerLimit > 60.0*30.0)
			{
				if(lowerLimit <= 60.0*60.0)
					return 60.0*60.0; // hours
				else if(lowerLimit <= 60.0*60.0*2.0)
					return 60.0*60.0*2.0; // two hours
				else if(lowerLimit <= 60.0*60.0*3.0)
					return 60.0*60.0*3.0; // three hours
				else if(lowerLimit <= 60.0*60.0*4.0)
					return 60.0*60.0*4.0; // four hours
				else if(lowerLimit <= 60.0*60.0*6.0)
					return 60.0*60.0*6.0; // six hours
				else
					return 60.0*60.0*12.0; // twelve hours
			}
			// in minutes?
			else if(lowerLimit > 30.0)
			{
				if(lowerLimit <= 60.0)
					return 60.0; // in minutes
				else if(lowerLimit <= 60.0*2.0)
					return 60.0*2.0; // two minutes
				else if(lowerLimit <= 60.0*5.0)
					return 60.0*5.0; // five minutes
				else if(lowerLimit <= 60.0*10.0)
					return 60.0*10.0; // ten minutes
				else if(lowerLimit <= 60.0*15.0)
					return 60.0*15.0; // quarter hours
				else
					return 60.0*30.0; // half hours
			}
			// in seconds?
			else if(lowerLimit > 0.5)
			{
				if(lowerLimit <= 1.0)
					return 1.0; // in seconds
				else if(lowerLimit <= 2.0)
					return 2.0; // two seconds
				else if(lowerLimit <= 5.0)
					return 5.0; // five seconds
				else if(lowerLimit <= 10.0)
					return 10.0; // ten seconds
				else if(lowerLimit <= 15.0)
					return 15.0; // quarter minute
				else
					return 30.0; // half a minute
			}
			else if(lowerLimit == 0.0)
				return 0.0;
			// in 10th of seconds or lower?
			else
			{
				double factor = 1.0;
				while(lowerLimit <= 0.1 && std::isfinite(lowerLimit))
				{
					factor *= 0.1;
					lowerLimit *= 10.0;
				}
				if(lowerLimit <= 0.2)
					return 0.2 * factor;
				else if(lowerLimit <= 0.5)
					return 0.5 * factor;
				else
					return factor;
			}
		}
		
		double roundUpToNiceNumber(double number, double roundUnit)
		{
			return roundUnit * ceil(number / roundUnit);
		}
		
		double _min, _max;
		unsigned _sizeRequest;
		std::vector<double> _ticks;
};

class TextTickSet : public TickSet
{
	public:
		TextTickSet(const std::vector<std::string> &labels, unsigned sizeRequest) : _sizeRequest(sizeRequest), _labels(labels)
		{
			set(sizeRequest);
		}
		
		virtual unsigned Size() const
		{
			return _ticks.size();
		}
		
		virtual Tick GetTick(unsigned i) const
		{
			const size_t labelIndex = _ticks[i];
			const double val = (_labels.size() == 1) ? 0.5 : (double) labelIndex / (double) (_labels.size() - 1);
			return Tick(val, _labels[labelIndex]);
		}
		
		virtual void Reset()
		{
			_ticks.clear();
			set(_sizeRequest);
		}
		
		virtual void Set(unsigned maxSize)
		{
			_ticks.clear();
			set(maxSize);
		}
	private:
		void set(unsigned sizeRequest)
		{
			if(sizeRequest > _labels.size())
				sizeRequest = _labels.size();
			const unsigned stepSize =
				(unsigned) ceil((double) _labels.size() / (double) sizeRequest);
			
			for(size_t tick=0;tick<_labels.size();tick += stepSize)
				_ticks.push_back(tick);
		}
		
		unsigned _sizeRequest;
		std::vector<std::string> _labels;
		std::vector<size_t> _ticks;
};
