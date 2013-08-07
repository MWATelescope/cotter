#ifndef NUMBERLIST_H
#define NUMBERLIST_H

#include <cstdlib>
#include <string>
#include <vector>

class NumberList
{
public:
	static void ParseIntList(const std::string &str, std::vector<int> &list)
	{
		std::string temp = str;
		size_t pos = temp.find(",");
		while(pos != std::string::npos)
		{
			std::string idStr = temp.substr(0, pos);
			temp = temp.substr(pos+1);
			int num = atoi(idStr.c_str());
			list.push_back(num);
			pos = temp.find(",");
		}
		int id = atoi(temp.c_str());
		list.push_back(num);
	}
};

#endif
