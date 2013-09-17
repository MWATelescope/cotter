#ifndef STDEXT_UVECTOR_H
#define STDEXT_UVECTOR_H

#include <cstring>
#include <iterator>
#include <memory>
#include <utility>

/**
 * Tp must satisfy:
 * - Its behaviour must be defined when it is not initialized
 * - It must be copyable with memcpy
 */
template<typename Tp, typename Alloc = std::allocator<Tp> >
class uvector : private Alloc
{
public:
	typedef Tp* pointer;
	typedef std::size_t size_t;
	typedef Tp* iterator;
	typedef const Tp* const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	
private:
	pointer _begin, _end, _endOfStorage;
	
public:
	uvector(const Alloc& allocator = Alloc()) noexcept
	: Alloc(allocator), _begin(nullptr), _end(nullptr), _endOfStorage(nullptr)
	{
	}
	
	explicit uvector(size_t n) :
		_begin(allocate(n)),
		_end(_begin + n),
		_endOfStorage(_end)
	{
	}
	
	uvector(size_t n, const Tp& val, const Alloc& allocator = Alloc()) :
		Alloc(allocator),
		_begin(allocate(n)),
		_end(_begin + n),
		_endOfStorage(_end)
	{
		fill(_begin, _end, val);
	}
	
	template<class InputIterator>
	uvector(InputIterator first, InputIterator last, const Alloc& allocator = Alloc()) :
		Alloc(allocator)
	{
		construct_from_range<InputIterator>(first, last, std::is_integral<InputIterator>());
	}
	
	uvector(const uvector& other) :
		Alloc(other),
		_begin(allocate(other.size())),
		_end(_begin + other.size()),
		_endOfStorage(_end)
	{
		memcpy(_begin, other._begin, other.size() * sizeof(Tp));
	}
	
	uvector(const uvector& other, const Alloc& allocator) :
		Alloc(allocator),
		_begin(allocate(other.size())),
		_end(_begin + other.size()),
		_endOfStorage(_end)
	{
		memcpy(_begin, other._begin, other.size() * sizeof(Tp));
	}
	
	uvector(uvector&& other) :
		Alloc(std::move(other)),
		_begin(other._begin),
		_end(other._end),
		_endOfStorage(other._endOfStorage)
	{
		other._begin = nullptr;
		other._end = nullptr;
		other._endOfStorage = nullptr;
	}
	
	uvector(uvector&& other, const Alloc& allocator) :
		Alloc(allocator),
		_begin(other._begin),
		_end(other._end),
		_endOfStorage(other._endOfStorage)
	{
		other._begin = nullptr;
		other._end = nullptr;
		other._endOfStorage = nullptr;
	}
	
	uvector(std::initializer_list<Tp> initlist, const Alloc& allocator = Alloc()) :
		Alloc(allocator),
		_begin(allocate(initlist.size())),
		_end(_begin + initlist.size()),
		_endOfStorage(_end)
	{
		iterator destIter = _begin;
		for(typename std::initializer_list<Tp>::const_iterator i=initlist.begin(); i!=initlist.end(); ++i)
		{
			*destIter = *i;
			++destIter;
		}
	}
	
	~uvector()
	{
		deallocate();
	}
	
	uvector& operator=(const uvector& other)
	{
		const size_t n = other.size();
		if(n > capacity()) {
			deallocate();
			_begin = allocate(n);
			_end = _begin + n;
			_endOfStorage = _end;
		}
		memcpy(_begin, other._begin, n * sizeof(Tp));
		return *this;
	}
	
	uvector& operator=(uvector&& other)
	{
		Alloc::operator=(other);
		std::swap(_begin, other._begin);
		std::swap(_end, other._end);
		std::swap(_endOfStorage, other._endOfStorage);
		return *this;
	}
	
	bool empty() const noexcept { return _begin == _end; }
	
	size_t size() const noexcept { return _end - _begin; }
	
	size_t capacity() const noexcept { return _endOfStorage - _begin; }
	
	iterator begin() { return _begin; }
	
	const_iterator begin() const { return _begin; }
	
	iterator end() { return _end; }
	
	const_iterator end() const { return _end; }
	
	reverse_iterator rbegin() { return reverse_iterator(end()); }
	
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	
	reverse_iterator rend() { return reverse_iterator(begin()); }
	
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
	
	Tp& operator[](size_t index) { return _begin[index]; }
	
	const Tp& operator[](size_t index) const { return _begin[index]; }
	
	void resize(size_t n)
	{
		pointer newStorage = allocate(n);
		memcpy(newStorage, _begin, std::min(n, size()) * sizeof(Tp));
		deallocate();
		_begin = newStorage;
		_end = _begin + n;
		_endOfStorage = _begin + n;
	}
	
	void reserve(size_t n)
	{
		size_t newSize = std::min(n, size());
		pointer newStorage = allocate(n);
		memcpy(newStorage, _begin, newSize * sizeof(Tp));
		deallocate();
		_begin = newStorage;
		_end = newStorage + newSize;
		_endOfStorage = _begin + n;
	}
	
	Tp& front() { return *_begin; }
	
	const Tp& front() const { return *_begin; }
	
	Tp& back() { return *(_end - 1); }
	
	const Tp& back() const { return *(_end - 1); }
	
	Tp* data() { return _begin; }
	
	const Tp* data() const { return _begin; }
	
	void push_back(const Tp& item)
	{
		if(_end == _endOfStorage)
			enlarge();
		*_end = item;
		++_end;
	}
	
	void push_back(Tp&& item)
	{
		if(_end == _endOfStorage)
			enlarge();
		*_end = std::move(item);
		++_end;
	}
	
	void pop_back()
	{
		--_end;
	}
	
	template<typename... Args>
	void emplace_back(Args&&... args)
	{
		if(_end == _endOfStorage)
			enlarge();
		*_end = Tp(std::forward<Args...>(args...));
		++_end;
	}
	
	iterator insert(iterator position, const Tp& item)
	{
		if(_end == _endOfStorage)
		{
			size_t index = position - _begin;
			enlarge();
			position = _begin + index;
		}
		memmove(position+1, position, (_end - position) * sizeof(Tp));
		*position = item;
		++_end;
		return position;
	}
	
private:
	pointer allocate(size_t n)
	{
		return Alloc::allocate(n);
	}
	
	void deallocate()
	{
		deallocate(_begin, capacity());
	}
	
	void deallocate(pointer begin, size_t n)
	{
		if(begin != nullptr)
			Alloc::deallocate(begin, n);
	}
	
	template<typename InputIterator>
	void construct_from_range(InputIterator first, InputIterator last, std::false_type)
	{
		construct_from_range<InputIterator>(first, last, typename std::iterator_traits<InputIterator>::iterator_category());
	}
	
	template<typename Integral>
	void construct_from_range(Integral n, Integral val, std::true_type)
	{
		_begin = allocate(n);
		_end = _begin + n;
		_endOfStorage = _end;
		fill(_begin, _end, val);
	}
	
	template<typename InputIterator>
	void construct_from_range(InputIterator first, InputIterator last, std::forward_iterator_tag)
	{
		size_t n = std::distance(first, last);
		_begin = allocate(n);
		_end = _begin + n;
		_endOfStorage = _begin + n;
		Tp* destIter = _begin;
		while(first != last)
		{
			*destIter = *first;
			++destIter; ++first;
		}
	}
		
	void enlarge()
	{
		enlarge(size() * 2 + 16);
	}
	
	void enlarge(size_t newSize)
	{
		pointer newStorage = allocate(newSize);
		memcpy(newStorage, _begin, size() * sizeof(Tp));
		deallocate();
		_end = newStorage + size();
		_begin = newStorage;
		_endOfStorage = _begin + newSize;
	}
	
	void fill(iterator begin, iterator end, const Tp& val)
	{
		if(sizeof(Tp) == 1)
			memset(begin, (end-begin)*sizeof(Tp), val);
		else {
			while(begin != end) {
				*begin = val;
				++begin;
			}
		}
	}
};

#endif // STDEXT_UVECTOR_H
