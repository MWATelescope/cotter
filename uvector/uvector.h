#ifndef STDEXT_UVECTOR_H
#define STDEXT_UVECTOR_H

#include <cstring>
#include <iterator>
#include <memory>
#include <utility>
#include <stdexcept>

/**
 * Tp must satisfy:
 * - Its behaviour must be defined when it is not initialized
 * - It must be copyable with memcpy
 */
template<typename Tp, typename Alloc = std::allocator<Tp> >
class uvector : private Alloc
{
public:
	typedef Tp value_type;
	typedef Alloc allocator_type;
	typedef Tp& reference;
	typedef const Tp& const_reference;
	typedef Tp* pointer;
	typedef const Tp* const_pointer;
	typedef std::size_t size_t;
	typedef std::size_t size_type;
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
	
	iterator begin() { return _begin; }
	
	const_iterator begin() const { return _begin; }
	
	iterator end() { return _end; }
	
	const_iterator end() const { return _end; }
	
	reverse_iterator rbegin() { return reverse_iterator(end()); }
	
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	
	reverse_iterator rend() { return reverse_iterator(begin()); }
	
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
	
	const_iterator cbegin() const { return _begin; }
	
	const_iterator cend() const { return _end; }
	
	const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
	
	const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }
	
	size_t size() const noexcept { return _end - _begin; }
	
	size_t max_size() const noexcept { return Alloc::max_size(); }
	
	void resize(size_t n)
	{
		if(capacity() < n)
		{
			pointer newStorage = allocate(n);
			memcpy(newStorage, _begin, size() * sizeof(Tp));
			deallocate();
			_begin = newStorage;
			_endOfStorage = _begin + n;
		}
		_end = _begin + n;
	}
	
	void resize(size_t n, const Tp& val)
	{
		size_t oldSize = size();
		if(capacity() < n)
		{
			pointer newStorage = allocate(n);
			memcpy(newStorage, _begin, size() * sizeof(Tp));
			deallocate();
			_begin = newStorage;
			_endOfStorage = _begin + n;
		}
		_end = _begin + n;
		if(oldSize < n)
			fill(_begin + oldSize, _end, val);
	}
	
	size_t capacity() const noexcept { return _endOfStorage - _begin; }
	
	bool empty() const noexcept { return _begin == _end; }
	
	void reserve(size_t n)
	{
		if(capacity() < n)
		{
			const size_t curSize = size();
			pointer newStorage = allocate(n);
			memcpy(newStorage, _begin, curSize * sizeof(Tp));
			deallocate();
			_begin = newStorage;
			_end = newStorage + curSize;
			_endOfStorage = _begin + n;
		}
	}
	
	void shrink_to_fit()
	{
		const size_t curSize = size();
		if(curSize == 0)
		{
			deallocate();
			_begin = nullptr;
			_end = nullptr;
			_endOfStorage = nullptr;
		}
		else {
			pointer newStorage = allocate(curSize);
			memcpy(newStorage, _begin, curSize * sizeof(Tp));
			deallocate();
			_begin = newStorage;
			_end = newStorage + curSize;
			_endOfStorage = _begin + curSize;
		}
	}
	
	Tp& operator[](size_t index) { return _begin[index]; }
	
	const Tp& operator[](size_t index) const { return _begin[index]; }
	
	Tp& at(size_t index)
	{
		check_bounds(index);
		return _begin[index];
	}
	
	const Tp& at(size_t index) const
	{
		check_bounds(index);
		return _begin[index];
	}
	
	Tp& front() { return *_begin; }
	
	const Tp& front() const { return *_begin; }
	
	Tp& back() { return *(_end - 1); }
	
	const Tp& back() const { return *(_end - 1); }
	
	Tp* data() { return _begin; }
	
	const Tp* data() const { return _begin; }
	
	template<class InputIterator>
  void assign(InputIterator first, InputIterator last)
	{
		assign_from_range<InputIterator>(first, last, std::is_integral<InputIterator>());
	}
	
	void assign(size_t n, const Tp& val)
	{
		if(n > capacity())
		{
			deallocate();
			_begin = allocate(n);
			_endOfStorage = _begin + n;
		}
		_end = _begin + n;
		fill(_begin, _end, val);
	}
	
	void assign(std::initializer_list<Tp> initlist)
	{
		if(initlist.size() > capacity())
		{
			deallocate();
			_begin = allocate(initlist.size());
			_endOfStorage = _begin + initlist.size();
		}
		_end = _begin + initlist.size();
		iterator destIter = _begin;
		for(typename std::initializer_list<Tp>::const_iterator i=initlist.begin(); i!=initlist.end(); ++i)
		{
			*destIter = *i;
			++destIter;
		}
	}
	
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
	
	iterator insert(const_iterator position, const Tp& item)
	{
		if(_end == _endOfStorage)
		{
			size_t index = position - _begin;
			enlarge_for_insert(enlarge_size(), index, 1);
			position = _begin + index;
		}
		else {
			memmove(const_cast<iterator>(position)+1, position, (_end - position) * sizeof(Tp));
			++_end;
		}
		*const_cast<iterator>(position) = item;
		return const_cast<iterator>(position);
	}
	
	iterator insert(const_iterator position, size_t n, const Tp& val)
	{
		if(capacity() < size() + n)
		{
			size_t index = position - _begin;
			enlarge_for_insert(std::max(size() + n, enlarge_size()), index, n);
			position = _begin + index;
		}
		else {
			memmove(const_cast<iterator>(position)+n, position, (_end - position) * sizeof(Tp));
			_end += n;
		}
		fill(const_cast<iterator>(position), const_cast<iterator>(position)+n, val);
		return const_cast<iterator>(position);
	}
	
	template <class InputIterator>
	iterator insert(const_iterator position, InputIterator first, InputIterator last)
	{
		return insert_from_range<InputIterator>(position, first, last, std::is_integral<InputIterator>());
	}
	
	iterator insert(const_iterator position, Tp&& item)
	{
		if(_end == _endOfStorage)
		{
			size_t index = position - _begin;
			enlarge_for_insert(enlarge_size(), index, 1);
			position = _begin + index;
		}
		else {
			memmove(const_cast<iterator>(position)+1, position, (_end - position) * sizeof(Tp));
			++_end;
		}
		*const_cast<iterator>(position) = std::move(item);
		return const_cast<iterator>(position);
	}
	
	iterator insert(const_iterator position, std::initializer_list<Tp> initlist)
	{
		if(capacity() < size() + initlist.size())
		{
			size_t index = position - _begin;
			enlarge_for_insert(std::max(size() + initlist.size(), enlarge_size()), index, initlist.size());
			position = _begin + index;
		}
		else {
			memmove(const_cast<iterator>(position)+initlist.size(), position, (_end - position) * sizeof(Tp));
			_end += initlist.size();
		}
		iterator destIter = const_cast<iterator>(position);
		for(typename std::initializer_list<Tp>::const_iterator i=initlist.begin(); i!=initlist.end(); ++i)
		{
			*destIter = *i;
			++destIter;
		}
		return const_cast<iterator>(position);
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
	
	template<typename InputIterator>
	void assign_from_range(InputIterator first, InputIterator last, std::false_type)
	{
		assign_from_range<InputIterator>(first, last, typename std::iterator_traits<InputIterator>::iterator_category());
	}
	
	template<typename Integral>
	void assign_from_range(Integral n, Integral val, std::true_type)
	{
		if(size_t(n) > capacity())
		{
			deallocate();
			_begin = allocate(n);
			_endOfStorage = _begin + n;
		}
		_end = _begin + n;
		fill(_begin, _end, val);
	}
	
	template<typename InputIterator>
	void assign_from_range(InputIterator first, InputIterator last, std::forward_iterator_tag)
	{
		size_t n = std::distance(first, last);
		if(n > capacity())
		{
			deallocate();
			_begin = allocate(n);
			_endOfStorage = _begin + n;
		}
		_end = _begin + n;
		Tp* destIter = _begin;
		while(first != last)
		{
			*destIter = *first;
			++destIter; ++first;
		}
	}
	
	template<typename InputIterator>
	iterator insert_from_range(const_iterator position, InputIterator first, InputIterator last, std::false_type)
	{
		return insert_from_range<InputIterator>(position, first, last,
			typename std::iterator_traits<InputIterator>::iterator_category());
	}
	
	template<typename Integral>
	iterator insert_from_range(const_iterator position, Integral n, Integral val, std::true_type)
	{
		if(capacity() < size() + n)
		{
			size_t index = position - _begin;
			enlarge_for_insert(std::max(size() + n, enlarge_size()), index, n);
			position = _begin + index;
		}
		else {
			memmove(const_cast<iterator>(position)+n, position, (_end - position) * sizeof(Tp));
			_end += n;
		}
		fill(const_cast<iterator>(position), const_cast<iterator>(position)+n, val);
		return const_cast<iterator>(position);
	}
	
	template<typename InputIterator>
	iterator insert_from_range(const_iterator position, InputIterator first, InputIterator last, std::forward_iterator_tag)
	{
		size_t n = std::distance(first, last);
		if(capacity() < size() + n)
		{
			size_t index = position - _begin;
			enlarge_for_insert(std::max(size() + n, enlarge_size()), index, n);
			position = _begin + index;
		}
		else {
			memmove(const_cast<iterator>(position)+n, position, (_end - position) * sizeof(Tp));
			_end += n;
		}
		Tp* destIter = const_cast<iterator>(position);
		while(first != last)
		{
			*destIter = *first;
			++destIter; ++first;
		}
		return const_cast<iterator>(position);
	}
	
	void check_bounds(size_t index) const
	{
		if(index >= size())
			throw std::out_of_range("Access to element in uvector >= size()");
	}
	
	size_t enlarge_size() const noexcept
	{
		return size() * 2 + 16;
	}
	
	void enlarge()
	{
		enlarge(enlarge_size());
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
	
	void enlarge_for_insert(size_t newSize, size_t insert_position, size_t insert_count)
	{
		pointer newStorage = allocate(newSize);
		memcpy(newStorage, _begin, insert_position * sizeof(Tp));
		memcpy(newStorage + insert_position + insert_count, _begin + insert_position, (size() - insert_position) * sizeof(Tp));
		deallocate();
		_end = newStorage + size() + insert_count;
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
