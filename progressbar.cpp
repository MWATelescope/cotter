#include "progressbar.h"

#include <iostream>

ProgressBar::ProgressBar(const std::string& taskDescription) :
	_taskDescription(taskDescription),
	_displayedDots(0)
{
	std::cout << taskDescription << ":";
	if(taskDescription.size() < 40)
		std::cout << " 0%" << std::flush;
	else
		std::cout << "\n 0%" << std::flush;
}

ProgressBar::~ProgressBar()
{
	SetProgress(1,1);
}

void ProgressBar::SetProgress(size_t taskIndex, size_t taskCount)
{
	unsigned progress = (taskIndex * 100 / taskCount);
	unsigned dots = progress / 2;
	
	if(dots > _displayedDots)
	{
		while(dots != _displayedDots)
		{
			++_displayedDots;
			if(_displayedDots % 5 == 0)
				std::cout << ((_displayedDots/5)*10) << '%';
			else
				std::cout << '.';
		}
		if(progress == 100)
			std::cout << '\n';
		std::cout << std::flush;
	}
}
