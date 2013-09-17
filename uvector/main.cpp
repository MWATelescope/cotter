#include <iostream>

#include "uvector.h"

void assert(bool test, const char* descr)
{
	std::cout << "Assert \"" << descr << "\" : " << std::flush;
	if(test)
		std::cout << "success\n";
	else
		std::cout << "FAILURE!\n";
}

void test()
{
	assert(uvector<int>().empty(), "uvector<int>().empty()");
	uvector<int> vec(1000);
	assert(!vec.empty(), "!vec.empty()");
	assert(vec.size() == 1000, "vec.size() == 1000");
	assert(vec.capacity() >= 1000, "vec.capacity() >= 1000");
	
	vec.push_back(16);
	assert(vec.size() == 1001, "vec.size() == 1001");
	assert(vec[1000] == 16, "vec[1000] == 16");
	
	vec.emplace_back(17);
	assert(vec.size() == 1002, "vec.size() == 1002");
	assert(vec[1001] == 17, "vec[1001] == 17");
	
	vec[0] = 1337;
	assert(vec[0] == 1337, "vec[0] == 1337");
	
	vec.pop_back();
	assert(vec.size() == 1001, "vec.size() == 1001");
	assert(*vec.begin() == 1337, "*vec.begin() == 1337");
	assert(*(vec.end()-1) == 16, "*(vec.end()-1) == 16");
	assert(*vec.rbegin() == 16, "*vec.rbegin() == 16");
	assert(*(vec.rend()-1) == 1337, "*(vec.rend()-1)) == 1337");
	
	assert(vec.insert(vec.begin()+1001, 42) - vec.begin() == 1001, "insert()");
	assert(vec.insert(vec.begin()+1001, 37) - vec.begin() == 1001, "insert()");
	assert(vec[1000] == 16, "vec[1000] == 16");
	assert(vec[1001] == 37, "vec[1001] == 37");
	assert(vec[1002] == 42, "vec[1002] == 42");
	
	vec = uvector<int>(100, 1);
	assert(vec.size() == 100, "vec.size() == 100");
	assert(vec[0] == 1, "vec[0] == 1");
	assert(vec[99] == 1, "vec[99] == 1");
	
	vec[0] = 2; vec[99] = 3;
	vec = uvector<int>(vec.rbegin(), vec.rend());
	assert(vec.size() == 100, "vec.size() == 100");
	assert(vec[0] == 3, "vec[0] == 3");
	assert(vec[99] == 2, "vec[99] == 2");
	assert(vec[1] == 1, "vec[1] == 1");
	assert(vec[50] == 1, "vec[50] == 1");
	
	vec = uvector<int>({7, 8, 9});
	assert(vec.size() == 3, "vec.size() == 3");
	assert(vec[0] == 7, "vec[0] == 7");
	assert(vec[1] == 8, "vec[1] == 8");
	assert(vec[2] == 9, "vec[2] == 9");

	assert(uvector<int>{7, 8}.size() == 2, "vec.size() == 2");
	assert(uvector<int>(7, 8).size() == 7, "vec.size() == 7");
}

int main(int argc, char **argv) {
	test();
	return 0;
}
