#include <iostream>

#include "uvector.h"

#include <vector>

void assert(bool test, const char* descr)
{
	std::cout << "Assert \"" << descr << "\" : " << std::flush;
	if(test)
		std::cout << "success\n";
	else
		std::cout << "FAILURE!\n";
}

template<typename Tp>
typename uvector<Tp>::iterator insert_uninitialized(uvector<Tp>& vec, typename uvector<Tp>::const_iterator i, size_t count)
{
	return vec.insert_uninitialized(i, count);
}

template<typename Tp>
typename std::vector<Tp>::iterator insert_uninitialized(std::vector<Tp>& vec, typename std::vector<Tp>::iterator i, size_t count)
{
	size_t index = i - vec.begin();
	vec.insert(i, count, Tp());
	return vec.begin() + index;
}

template<typename Vec>
void test()
{
	assert(Vec().empty(), "uvector<int>().empty()");
	assert(!Vec(1).empty(), "!uvector<int>(1).empty()");
	assert(Vec().capacity() == 0, "uvector<int>().capacity() == 0");
	Vec vec(1000);
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
	
	vec = Vec(100, 1);
	assert(vec.size() == 100, "vec.size() == 100");
	assert(vec[0] == 1, "vec[0] == 1");
	assert(vec[99] == 1, "vec[99] == 1");
	
	// construct with range
	vec[0] = 2; vec[99] = 3;
	vec = Vec(vec.rbegin(), vec.rend());
	assert(vec.size() == 100, "vec.size() == 100");
	assert(vec[0] == 3, "vec[0] == 3");
	assert(vec[99] == 2, "vec[99] == 2");
	assert(vec[1] == 1, "vec[1] == 1");
	assert(vec[50] == 1, "vec[50] == 1");
	
	// construct with init list
	vec = Vec({7, 8, 9});
	assert(vec.size() == 3, "vec.size() == 3");
	assert(vec[0] == 7, "vec[0] == 7");
	assert(vec[1] == 8, "vec[1] == 8");
	assert(vec[2] == 9, "vec[2] == 9");

	assert(Vec{7, 8}.size() == 2, "vec.size() == 2");
	assert(Vec({7, 8}).size() == 2, "vec.size() == 2");
	assert(Vec(7, 8).size() == 7, "vec.size() == 7");
	
	// resize
	vec.resize(5);
	assert(vec.size() == 5, "vec.size() == 5");
	assert(vec[0] == 7, "vec[0] == 7");
	assert(vec[1] == 8, "vec[1] == 8");
	assert(vec[2] == 9, "vec[2] == 9");
	vec[4] = 4;
	
	vec.resize(1000);
	assert(vec.size() == 1000, "vec.size() == 1000");
	assert(vec[0] == 7, "vec[0] == 7");
	assert(vec[1] == 8, "vec[1] == 8");
	assert(vec[2] == 9, "vec[2] == 9");
	assert(vec[4] == 4, "vec[4] == 4");

	vec.resize(5, 10);
	assert(vec.size() == 5, "vec.size() == 5");
	vec.resize(1000, 10);
	assert(vec.size() == 1000, "vec.size() == 1000");
	assert(vec[0] == 7, "vec[0] == 7");
	assert(vec[1] == 8, "vec[1] == 8");
	assert(vec[2] == 9, "vec[2] == 9");
	assert(vec[4] == 4, "vec[4] == 4");
	assert(vec[5] == 10, "vec[5] == 10");
	assert(vec[999] == 10, "vec[999] == 10");
	assert(vec.capacity() >= 1000, "vec.capacity() >= 1000");
	
	// reserve
	vec.reserve(0);
	assert(vec.capacity() >= 1000, "vec.capacity() >= 1000");
	vec = Vec();
	vec.reserve(1000);
	assert(vec.size() == 0, "vec.size() == 0");
	assert(vec.capacity() >= 1000, "vec.capacity() >= 1000");
	
	// shrink_to_fit
	vec.shrink_to_fit();
	assert(vec.size() == 0, "vec.size() == 0");
	assert(vec.capacity() == 0, "vec.capacity() == 0");
	
	vec.push_back(3);
	vec.push_back(7);
	vec.shrink_to_fit();
	assert(vec.size() == 2, "vec.size() == 2");
	assert(vec.capacity() == 2, "vec.capacity() == 2");
	assert(vec[0] == 3, "vec[0] == 3");
	assert(vec[1] == 7, "vec[1] == 7");
	
	// at
	assert(vec.at(0) == 3, "vec.at(0) == 3");
	assert(vec.at(1) == 7, "vec.at(1) == 7");
	try {
		vec.at(2);
		assert(false, "vec.at(2) throws");
	} catch(...) {
		assert(true, "vec.at(2) throws");
	}
	try {
		vec.at(-1);
		assert(false, "vec.at(-1) throws");
	} catch(...) {
		assert(true, "vec.at(-1) throws");
	}
	
	// assign
	vec.push_back(0);
	Vec temp = vec;
	vec.assign(temp.rbegin()+1, temp.rend());
	assert(vec.size() == 2, "vec.size() == 2");
	assert(vec[0] == 7, "vec[0] == 7");
	assert(vec[1] == 3, "vec[1] == 3");
	
	vec.assign(10, 1337);
	assert(vec.size() == 10, "vec.size() == 10");
	assert(vec[0] == 1337, "vec[0] == 1337");
	assert(vec[9] == 1337, "vec[9] == 1337");
	
	vec = Vec();
	vec.assign({1, 2, 3, 5, 7, 9});
	assert(vec.size() == 6, "vec.size() == 6");
	assert(vec[0] == 1, "vec[0] == 1");
	assert(vec[1] == 2, "vec[1] == 2");
	assert(vec[5] == 9, "vec[5] == 9");
	
	// insert
	vec = Vec(1000);
	vec.push_back(16);
	assert(vec.insert(vec.begin()+1001, 42) - vec.begin() == 1001, "insert()");
	assert(vec.insert(vec.begin()+1001, 37) - vec.begin() == 1001, "insert()");
	assert(vec.size() == 1003, "vec.size() == 1003");
	assert(vec[1000] == 16, "vec[1000] == 16");
	assert(vec[1001] == 37, "vec[1001] == 37");
	assert(vec[1002] == 42, "vec[1002] == 42");
	
	// GNU C++ doesn't return something yet, so skip for now
	//assert(vec.insert(vec.begin()+1001, 3, 3) == vec.begin() + 1001, "insert()");
	vec.insert(vec.begin()+1001, 3, 3);
	assert(vec[1000] == 16, "vec[1000] == 16");
	assert(vec[1001] == 3, "vec[1001] == 3");
	assert(vec[1002] == 3, "vec[1002] == 3");
	assert(vec[1003] == 3, "vec[1003] == 3");
	assert(vec[1004] == 37, "vec[1004] == 37");
	
	vec = Vec();
	assert(vec.capacity() == 0, "vec.capacity() == 0"); // to make sure inserts are going to increment size
	for(size_t i=0; i != 100; ++i)
	{
		vec.insert(vec.begin(), 1);
		vec.insert(vec.begin() + (vec.size()-1)/3+1, 2);
		vec.insert(vec.end(), 3);
	}
	bool allCorrect[3] = { true, true, true};
	for(size_t i=0; i != 100; ++i)
	{
		allCorrect[0] = allCorrect[0] && (vec[i] == 1);
		allCorrect[1] = allCorrect[1] && (vec[i+vec.size()/3] == 2);
		allCorrect[2] = allCorrect[2] && (vec[i+vec.size()*2/3] == 3);
	}
	assert(allCorrect[0], "insert()'s 1s were all correct");
	assert(allCorrect[1], "insert()'s 2s were all correct");
	assert(allCorrect[2], "insert()'s 3s were all correct");
	
	temp = Vec{ 1, 2, 3, 4, 5 };
	vec = Vec();
	for(size_t i=0; i != 100; ++i)
	{
		vec.insert(vec.begin(), temp.begin(), temp.end());
		vec.insert(vec.begin() + (vec.size()-5)/3+5, temp.rbegin(), temp.rend());
		vec.insert(vec.end(), temp.begin(), temp.end());
	}
	assert(vec.size() == 100*15, "vec.size() == 1000*15");
	for(size_t i=0; i != 100; ++i)
	{
		for(int j=0; j!=5; ++j)
		{
			allCorrect[0] = allCorrect[0] && vec[i*5+j]==j+1;
			allCorrect[1] = allCorrect[1] && vec[i*5+vec.size()/3+j]==5-j;
			allCorrect[2] = allCorrect[2] && vec[i*5+vec.size()*2/3+j]==j+1;
		}
	}
	assert(allCorrect[0], "insert()'s range A's were all correct");
	assert(allCorrect[1], "insert()'s range B's were all correct");
	assert(allCorrect[2], "insert()'s range C's were all correct");
	
	vec = Vec{1, 4};
	vec.insert(vec.begin() + 1, {2, 3});
	assert(vec[0] == 1, "vec[0] == 1");
	assert(vec[1] == 2, "vec[1] == 2");
	assert(vec[2] == 3, "vec[2] == 3");
	assert(vec[3] == 4, "vec[3] == 4");
	
	// insert_uninitialized
	vec = Vec(1000);
	vec.push_back(16);
	assert(insert_uninitialized(vec, vec.begin()+1001, 1) - vec.begin() == 1001, "insert_uninitialized()");
	assert(insert_uninitialized(vec, vec.begin()+1001, 1) - vec.begin() == 1001, "insert_uninitialized()");
	assert(vec.size() == 1003, "vec.size() == 1003");
	assert(vec[1000] == 16, "vec[1000] == 16");
	
	vec[1001] = 37;
	// GNU C++ doesn't return something yet, so skip for now
	//assert(insert_uninitialized(vec, vec.begin()+1001, 3, 3) == vec.begin() + 1001, "insert()");
	insert_uninitialized(vec, vec.begin()+1001, 3);
	assert(vec[1000] == 16, "vec[1000] == 16");
	assert(vec[1004] == 37, "vec[1004] == 37");
	
	// Erase
	vec = Vec{1, 2, 3, 4};
	typename Vec::iterator iter = vec.erase(vec.begin() + 1);
	assert(iter == vec.begin() + 1, "vec.erase(vec.begin() + 1) == vec.begin() + 1");
	assert(vec.size() == 3, "vec.size() == 3");
	assert(vec[0] == 1, "vec[0] == 1");
	assert(vec[1] == 3, "vec[1] == 3");
	assert(vec[2] == 4, "vec[2] == 4");
	iter = vec.erase(vec.begin() + 2);
	assert(iter == vec.end(), "vec.erase(vec.begin() + 2) == vec.end()");
	assert(vec.size() == 2, "vec.size() == 2");
	assert(vec[0] == 1, "vec[0] == 1");
	assert(vec[1] == 3, "vec[1] == 3");
	
	iter = vec.erase(vec.begin(), vec.end());
	assert(iter == vec.end(), "vec.erase(vec.begin() + 2) == vec.end()");
	assert(vec.empty(), "vec.empty()");
	vec = Vec{1, 2, 3, 4};
	iter = vec.erase(vec.begin()+1, vec.end()-1);
	assert(iter == vec.begin()+1, "vec.erase(vec.begin()+1, vec.end()-1) == vec.begin()+1");
	assert(vec.size() == 2, "vec.size() == 2");
	
	// Swap
	temp = Vec{5, 6, 7};
	iter = temp.begin();
	vec = Vec{1, 2, 3, 4};
	vec.swap(temp);
	assert(vec.size() == 3, "vec.size()");
	assert(temp.size() == 4, "temp.size()");
	assert(*iter == 5, "*iter == 5");
	assert(vec[0] == 5, "vec[0] == 5");
	assert(vec.back() == 7, "vec.back() == 7");
	
	// Clear
	vec = Vec();
	vec.clear();
	assert(vec.empty(), "vec.empty()");
	vec = Vec{1, 2, 3, 4};
	vec.clear();
	assert(vec.empty(), "vec.empty()");
	vec.push_back(5);
	assert(vec.size()==1, "vec.size()==1");
	assert(vec.front()==5, "vec.front()==5");
	
	// Emplace
	vec = Vec{1, 3};
	iter = vec.emplace(vec.begin()+1, 2);
	assert(iter == vec.begin()+1, "vec.emplace(vec.begin()+1, 2) == vec.begin()+1");
	assert(vec.size() == 3, "vec.size() == 3");
	assert(vec[0] == 1, "vec[0] == 1");
	assert(vec[1] == 2, "vec[1] == 2");
	assert(vec[2] == 3, "vec[2] == 3");
	
	// operator==
	assert(Vec() == Vec(), "Vec() == Vec()");
	assert(!(Vec(1) == Vec()), "!(Vec(1) == Vec())");
	vec = Vec();
	vec.push_back(7);
	vec.pop_back();
	assert(vec == Vec(), "vec == Vec()");
	assert(Vec{1, 2, 3, 4, 5} == Vec{1, 2, 3, 4, 5}, "Vec{1, 2, 3, 4, 5} == Vec{1, 2, 3, 4, 5}");
	assert(!(Vec{1, 2, 3, 4, 5} == Vec{1, 2, 3, 4, 6}), "!(Vec{1, 2, 3, 4, 5} == Vec{1, 2, 3, 4, 5})");
	assert(!(Vec{1, 2, 3, 4} == Vec{1, 2, 3, 4, 5}), "!(Vec{1, 2, 3, 4} == Vec{1, 2, 3, 4, 5})");
	
	// operator!=
	assert(!(Vec() != Vec()), "!(Vec() != Vec())");
	assert(Vec(1) != Vec(), "Vec(1) != Vec()");
	vec = Vec();
	vec.push_back(7);
	vec.pop_back();
	assert(!(vec != Vec()), "!(vec != Vec())");
	assert(!(Vec{1, 2, 3, 4, 5} != Vec{1, 2, 3, 4, 5}), "V!(Vec{1, 2, 3, 4, 5} != Vec{1, 2, 3, 4, 5})");
	assert(Vec{1, 2, 3, 4, 5} != Vec{1, 2, 3, 4, 6}, "Vec{1, 2, 3, 4, 5} != Vec{1, 2, 3, 4, 6}");
	assert(Vec{1, 2, 3, 4} != Vec{1, 2, 3, 4, 5}, "Vec{1, 2, 3, 4} != Vec{1, 2, 3, 4, 5}");
	
	// operator<
	assert(!(Vec() < Vec()), "!(Vec() < Vec())");
	assert(!(Vec{1} < Vec{1}), "!(Vec{1} < Vec{1})");
	assert(!(Vec{2} < Vec{1}), "!(Vec{2} < Vec{1})");
	assert(Vec() < Vec{1}, "Vec() < Vec{1}");
	assert(!(Vec{1} < Vec()), "!(Vec{1} < Vec()");
	assert(Vec{1} < Vec{2}, "Vec{1} < Vec{2}");
	assert(Vec{1,2,3} < Vec{1,2,4}, "Vec{1,2,3} < Vec{1,2,4}");

	// operator>
	assert(!(Vec() > Vec()), "!(Vec() > Vec())");
	assert(!(Vec{1} > Vec{1}), "!(Vec{1} > Vec{1})");
	assert(!(Vec{1} > Vec{2}), "!(Vec{1} > Vec{2})");
	assert(Vec{1,2,4} > Vec{1,2,3}, "Vec{1,2,4} > Vec{1,2,3}");
	
	// operator<=
	assert(Vec() <= Vec(), "Vec() <= Vec()");
	assert(Vec{1} <= Vec{1}, "Vec{1} <= Vec{1})");
	assert(!(Vec{2} <= Vec{1}), "!(Vec{2} <= Vec{1})");
	assert(Vec() <= Vec{1}, "Vec() <= Vec{1}");
	assert(!(Vec{1} <= Vec()), "!(Vec{1} <= Vec())");
	assert(Vec{1} <= Vec{2}, "Vec{1} <= Vec{2}");
	assert(Vec{1,2,3} <= Vec{1,2,4}, "Vec{1,2,3} <= Vec{1,2,4}");

	// operator>=
	assert(Vec() >= Vec(), "Vec() >= Vec()");
	assert(Vec{1} >= Vec{1}, "Vec{1} >= Vec{1}");
	assert(!(Vec{1} >= Vec{2}), "(Vec{1} >= Vec{2})");
	assert(Vec{1} >= Vec(), "Vec{1} >= Vec()");
	assert(!(Vec() >= Vec{1}), "!(Vec() >= Vec{1})");
	assert(Vec{2} >= Vec{1}, "Vec{2} >= Vec{1}");
	assert(Vec{1,2,4} >= Vec{1,2,3}, "Vec{1,2,4} >= Vec{1,2,3}");
	
	// ::swap
	temp = Vec{100, 101, 102};
	swap(temp, vec);
	assert(vec.size() == 3, "vec.size() == 3");
	assert(vec[0] == 100, "vec[0] == 100");
	assert(vec[1] == 101, "vec[1] == 101");
	assert(vec[2] == 102, "vec[2] == 102");
}

template<typename Tp>
class FailingAllocator
{
public:
	typedef std::size_t size_type;
	typedef Tp* pointer;
	FailingAllocator() { }
	FailingAllocator(bool fail) { _failAllocation = fail; }
	void SetFailAllocation(bool fail) { _failAllocation = fail; }
protected:
	pointer allocate(size_type n, std::allocator<void>::const_pointer hint=0)
	{
		if(_failAllocation)
			throw std::bad_alloc();
		return static_cast<pointer>(malloc(n*sizeof(Tp)));
	}
	void deallocate(pointer ptr, size_type n)
	{
		free(ptr);
	}
private:
	static bool _failAllocation;
};

template<typename Tp>
bool FailingAllocator<Tp>::_failAllocation = false;

void testBadAllocs()
{
	typedef uvector<int,FailingAllocator<int>> Vec;
	Vec(5, 0, FailingAllocator<int>(false));
	try {
		Vec(5, 0, FailingAllocator<int>(true));
		assert(false, "constructor throws");
	} catch(...) { 
		assert(true, "constructor throws");
	}
	
	uvector<int> goodVec;
	
	Vec vec(FailingAllocator<int>(false));
	vec.assign({6, 7, 8, 9});
	goodVec.assign({6, 7, 8, 9});
	vec.get_allocator().SetFailAllocation(true);
	try {
		for(size_t i=0; i!=10000; ++i)
		{
			vec.push_back(i);
			goodVec.push_back(i);
		}
		assert(false, "push_back throws");
	} catch(...) { 
		assert(true, "push_back throws");
	}
	assert(vec.size() == goodVec.size(), "vector size intact");
	assert(vec.front() == goodVec.front(), "vector front intact");
	assert(vec.back() == goodVec.back(), "vector back intact");
	
	vec.get_allocator().SetFailAllocation(false);
	vec = Vec();
	vec.reserve(1000);
	vec.push_back(1);
	vec.get_allocator().SetFailAllocation(true);
	try {
		vec.shrink_to_fit();
		assert(false, "shrink_to_fit throws");
	} catch(...) { 
		assert(true, "shrink_to_fit throws");
	}
	assert(vec.size() == 1, "vector size intact");
	assert(vec[0] == 1, "vector content intact");
	vec.get_allocator().SetFailAllocation(false);
	vec.pop_back();
	vec.shrink_to_fit();
	vec.push_back(1);
	
	vec.get_allocator().SetFailAllocation(true);
	try {
		vec.insert(0, 1000, 0);
		assert(false, "insert throws");
	} catch(...) { 
		assert(true, "insert throws");
	}
	assert(vec.size() == 1, "vector size intact");
	assert(vec[0] == 1, "vector content intact");
}

template<typename Tp>
class IdAllocater
{
public:
	typedef std::size_t size_type;
	typedef Tp* pointer;
	IdAllocater(size_t id) : _id(id) { }
	size_t Id() const noexcept { return _id; }
protected:
	pointer allocate(size_type n, std::allocator<void>::const_pointer hint=0)
	{
		return static_cast<pointer>(malloc(n*sizeof(Tp)));
	}
	void deallocate(pointer ptr, size_type n)
	{
		free(ptr);
	}
private:
	size_t _id;
};

void testAllocater()
{
	uvector<int, IdAllocater<int>>
		vecA({1, 2, 3}, IdAllocater<int>(4)),
		vecB({1, 2, 3}, IdAllocater<int>(5)),
		vecC(vecA);
	assert(vecA.get_allocator().Id() == 4, "alloc id == 4");
	assert(vecB.get_allocator().Id() == 5, "alloc id == 5");
	assert(vecC.get_allocator().Id() == 4, "alloc id == 4");
	swap(vecA, vecB);
	assert(vecA.get_allocator().Id() == 5, "alloc id == 5");
	assert(vecB.get_allocator().Id() == 4, "alloc id == 4");
}

int main(int argc, char **argv) {
	std::cout << "== std::vector<int> ==\n";
	test<std::vector<int>>();
	std::cout << "\n== uvector<int> ==\n";
	test<uvector<int>>();
	std::cout << "\n== uvector<long int> ==\n";
	test<uvector<long int>>();
	std::cout << "\n== uvector<int> allocator ==\n";
	testBadAllocs();
	testAllocater();
	
	return 0;
}
