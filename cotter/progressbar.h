#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include <string>

class ProgressBar
{
	public:
		ProgressBar(const std::string &taskDescription);
	
		void SetProgress(size_t taskIndex, size_t taskCount);
		
	private:
		const std::string _taskDescription;
		unsigned _displayedDots;
};

#endif
