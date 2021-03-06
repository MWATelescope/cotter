#ifndef ALIGNED_PTR_H
#define ALIGNED_PTR_H

#include <cstdlib>
#include <memory>
#include <stdexcept>

template<typename T> 
using aligned_ptr = std::unique_ptr<T[], decltype(&std::free)>;

template<typename T>
static aligned_ptr<T> empty_aligned()
{
	return aligned_ptr<T>(nullptr, &std::free);
}

template<typename T>
static aligned_ptr<T> make_aligned(size_t size, size_t align)
{
	T* data = nullptr;
	if(0 != posix_memalign((void**) &data, align, size*sizeof(T)))
		throw std::runtime_error("Failed to allocate aligned memory");
	return aligned_ptr<T>(data, &std::free);
}

#endif
