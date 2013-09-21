#ifndef STDEXT_UVECTOR_H
#define STDEXT_UVECTOR_H

#include <cstring>
#include <iterator>
#include <memory>
#include <utility>
#include <stdexcept>

/**
 * @file uvector.h
 * Header file for uvector and its relational and swap function.
 * @author André Offringa
 * @copyright André Offringa, 2013, distributed under the GPL license version 3.
 */

/**
 * @brief A container similar to std::vector that can be constructed without initializing its elements.
 * @details This container is similar to a std::vector, except that it can be constructor without
 * initializing its elements. This saves the overhead of initialization, hence the
 * constructor @ref uvector(size_t) is significantly faster than the corresponding std::vector
 * constructor, and has no overhead to a manually allocated array.
 * 
 * Probably its greatest strength lies in the construction of containers with a number of elements
 * that is runtime defined, but that will be initialized later. For example:
 * 
 * @code
 * // Open a file
 * std::ifstream file("myfile.bin");
 * 
 * // Construct a buffer for this file
 * uvector<unsigned char> buffer(buffer_size);
 * 
 * // Read some data into the buffer
 * file.read(&buffer[0], buffer_size);
 * 
 * @endcode
 * 
 * However, it has a few more use-cases with improved performance over std::vector. This is
 * true because of more strengent requirements on the element's type.
 * 
 * The element type must be able to skip its constructor. This is the case for all integral
 * types, such as @c char, @c int, pointers or simple structs or classes, but not for complex
 * types that e.g. require their constructor to perform allocation, such as std::string or
 * std::unique_ptr. A uvector can therefore also not hold itself as element type.
 * 
 * When an element is copied or assigned, there is no guarantee that the copy or move constructor
 * or assignment operator are called. Instead, a byte-wise copy or displacement might be performed
 * with @c memcpy or @c memmove. Finally, if a non-default copy or move
 * constructor or assignment operator is defined, they are not allowed to throw. It also does not
 * make much sense to define these methods, as there is no guarantee that they are called.
 * 
 * Because of the use of @c memcpy and @c memmove, the @ref push_back() and @ref insert()
 * methods are a bit faster than the std::vector counterparts, at least on gcc 4.8. 
 * 
 * The methods with different semantics compared to std::vector are:
 * * @ref uvector(size_t)
 * * @ref resize(size_t)
 * 
 * Also one new member is introduced:
 * * @ref insert_uninitialized(const_iterator, size_t)
 * 
 * All other members work exactly like std::vector's members, although some are slightly faster because of
 * the stricter requirements on the element type.
 * 
 * @tparam Tp Container's element type
 * @tparam Alloc Allocator type. Default is to use the std::allocator.
 * 
 * @author André Offringa
 * @copyright André Offringa, 2013, distributed under the GPL license version 3.
 */
template<typename Tp, typename Alloc = std::allocator<Tp> >
class uvector : private Alloc
{
public:
	/// Element type
	typedef Tp value_type;
	/// Type of allocator used to allocate and deallocate space
	typedef Alloc allocator_type;
	/// Reference to element type
	typedef Tp& reference;
	/// Constant reference to element type
	typedef const Tp& const_reference;
	/// Pointer to element type
	typedef Tp* pointer;
	/// Pointer to constant element type
	typedef const Tp* const_pointer;
	/// Iterator type
	typedef Tp* iterator;
	/// Iterator type of constant elements
	typedef const Tp* const_iterator;
	/// Reverse iterator type
	typedef std::reverse_iterator<iterator> reverse_iterator;
	/// Reverse iterator of constant elements
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	/// Difference between to iterators
	typedef std::ptrdiff_t difference_type;
	/// Type used for indexing elements
	typedef std::size_t size_t;
	/// Type used for indexing elements
	typedef std::size_t size_type;
	
private:
	pointer _begin, _end, _endOfStorage;
	
public:
	/** @brief Construct an empty uvector.
	 * @param allocator Allocator used for allocating and deallocating memory.
	 */
	uvector(const allocator_type& allocator = Alloc()) noexcept
	: Alloc(allocator), _begin(nullptr), _end(nullptr), _endOfStorage(nullptr)
	{
	}
	
	/** @brief Construct a vector with given amount of elements, without initializing these.
	 * @details This constructor deviates from std::vector's behaviour, because it will not
	 * value construct its elements. It is therefore faster than the corresponding constructor
	 * of std::vector.
	 * @param n Number of elements that the uvector will be initialized with.
	 */
	explicit uvector(size_t n) :
		_begin(allocate(n)),
		_end(_begin + n),
		_endOfStorage(_end)
	{
	}
	
	/** @brief Construct a vector with given amount of elements and set these to a specific value.
	 * @details This constructor will initialize its members with the given value. 
	 * @param n Number of elements that the uvector will be initialized with.
	 * @param val Value to initialize all elements with
	 * @param allocator Allocator used for allocating and deallocating memory.
	 */
	uvector(size_t n, const value_type& val, const allocator_type& allocator = Alloc()) :
		Alloc(allocator),
		_begin(allocate(n)),
		_end(_begin + n),
		_endOfStorage(_end)
	{
		fill(_begin, _end, val);
	}
	
	/** @brief Construct a vector by copying elements from a range.
	 * @param first Iterator to range start
	 * @param last Iterator to range end
	 * @param allocator Allocator used for allocating and deallocating memory.
	 */
	template<class InputIterator>
	uvector(InputIterator first, InputIterator last, const allocator_type& allocator = Alloc()) :
		Alloc(allocator)
	{
		construct_from_range<InputIterator>(first, last, std::is_integral<InputIterator>());
	}
	
	/** @brief Copy construct a uvector.
	 * @param other Source uvector to be copied from.
	 */
	uvector(const uvector<Tp,Alloc>& other) :
		Alloc(static_cast<allocator_type>(other)),
		_begin(allocate(other.size())),
		_end(_begin + other.size()),
		_endOfStorage(_end)
	{
		memcpy(_begin, other._begin, other.size() * sizeof(Tp));
	}
	
	/** @brief Copy construct a uvector with custom allocator.
	 * @param other Source uvector to be copied from.
	 * @param allocator Allocator used for allocating and deallocating memory.
	 */
	uvector(const uvector<Tp,Alloc>& other, const allocator_type& allocator) :
		Alloc(allocator),
		_begin(allocate(other.size())),
		_end(_begin + other.size()),
		_endOfStorage(_end)
	{
		memcpy(_begin, other._begin, other.size() * sizeof(Tp));
	}
	
	/** @brief Move construct a uvector.
	 * @param other Source uvector to be moved from.
	 */
	uvector(uvector<Tp,Alloc>&& other) :
		Alloc(std::move(other)),
		_begin(other._begin),
		_end(other._end),
		_endOfStorage(other._endOfStorage)
	{
		other._begin = nullptr;
		other._end = nullptr;
		other._endOfStorage = nullptr;
	}
	
	/** @brief Move construct a uvector with custom allocator.
	 * @param other Source uvector to be moved from.
	 * @param allocator Allocator used for allocating and deallocating memory.
	 */
	uvector(uvector<Tp,Alloc>&& other, const Alloc& allocator) :
		Alloc(allocator),
		_begin(other._begin),
		_end(other._end),
		_endOfStorage(other._endOfStorage)
	{
		other._begin = nullptr;
		other._end = nullptr;
		other._endOfStorage = nullptr;
	}
	
	/** @brief Construct a uvector from a initializer list.
	 * @param initlist Initializer list used for initializing the new uvector.
	 * @param allocator Allocator used for allocating and deallocating memory.
	 */
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
	
	/** @brief Destructor. */
	~uvector()
	{
		deallocate();
	}
	
	uvector& operator=(const uvector<Tp,Alloc>& other)
	{
		const size_t n = other.size();
		if(n > capacity()) {
			iterator newStorage = allocate(n);
			deallocate();
			_begin = newStorage;
			_end = _begin + n;
			_endOfStorage = _end;
		}
		memcpy(_begin, other._begin, n * sizeof(Tp));
		return *this;
	}
	
	uvector& operator=(uvector<Tp,Alloc>&& other)
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
			iterator newStorage = allocate(n);
			deallocate();
			_begin = newStorage;
			_endOfStorage = _begin + n;
		}
		_end = _begin + n;
		fill(_begin, _end, val);
	}
	
	void assign(std::initializer_list<Tp> initlist)
	{
		if(initlist.size() > capacity())
		{
			iterator newStorage = allocate(initlist.size());
			deallocate();
			_begin = newStorage;
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
	
	iterator erase(const_iterator position)
	{
		--_end;
		memmove(const_cast<iterator>(position), position+1, (_end-position)*sizeof(Tp));
		return const_cast<iterator>(position);
	}
	
	iterator erase(const_iterator first, const_iterator last)
	{
		size_t n = last - first;
		_end -= n;
		memmove(const_cast<iterator>(first), first+n, (_end-first)*sizeof(Tp));
		return const_cast<iterator>(first);
	}
	
	void swap(uvector<Tp, Alloc>& other)
	{
		std::swap(_begin, other._begin);
		std::swap(_end, other._end);
		std::swap(_endOfStorage, other._endOfStorage);
		//swap_allocator(other, Alloc::propagate_on_container_move_assignment());
		swap_allocator(other, std::true_type());
	}
	
	void clear()
	{
		_end = _begin;
	}
	
	template<typename... Args>
	iterator emplace(const_iterator position, Args&&... args)
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
		*const_cast<iterator>(position) = Tp(std::forward<Args...>(args...));
		return const_cast<iterator>(position);
	}
	
	template<typename... Args>
	void emplace_back(Args&&... args)
	{
		if(_end == _endOfStorage)
			enlarge();
		*_end = Tp(std::forward<Args...>(args...));
		++_end;
	}
	
	allocator_type get_allocator() const noexcept
	{
		return *this;
	}
	
	iterator insert_uninitialized(const_iterator position, size_t n)
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
			iterator newStorage = allocate(n);
			deallocate();
			_begin = newStorage;
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
			iterator newStorage = allocate(n);
			deallocate();
			_begin = newStorage;
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
		//return size() * 2 + 16;
		return size() + std::max(size(), size_t(1));
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
	
	void swap_allocator(uvector<Tp,Alloc>& other, std::true_type /* propagate_on_container_move_assignment */)
	{
		std::swap(static_cast<Alloc&>(other), static_cast<Alloc&>(*this));
	}
	
	void swap_allocator(uvector<Tp,Alloc>& other, std::false_type /* propagate_on_container_move_assignment */)
	{
		// no propagation: don't do anything
	}
};

template<class Tp, class Alloc>
inline bool operator==(const uvector<Tp,Alloc>& lhs, const uvector<Tp,Alloc>& rhs)
{
	return lhs.size()==rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template<class Tp, class Alloc>
inline bool operator!=(const uvector<Tp,Alloc>& lhs, const uvector<Tp,Alloc>& rhs)
{
	return !(lhs == rhs);
}

template <class Tp, class Alloc>
inline bool operator<(const uvector<Tp,Alloc>& lhs, const uvector<Tp,Alloc>& rhs)
{
	const size_t minSize = std::min(lhs.size(), rhs.size());
	for(size_t i=0; i!=minSize; ++i)
	{
		if(lhs[i] < rhs[i])
			return true;
		else if(lhs[i] > rhs[i])
			return false;
	}
	return lhs.size() < rhs.size();
}

template <class Tp, class Alloc>
inline bool operator<=(const uvector<Tp,Alloc>& lhs, const uvector<Tp,Alloc>& rhs)
{
	const size_t minSize = std::min(lhs.size(), rhs.size());
	for(size_t i=0; i!=minSize; ++i)
	{
		if(lhs[i] < rhs[i])
			return true;
		else if(lhs[i] > rhs[i])
			return false;
	}
	return lhs.size() <= rhs.size();
}

template <class Tp, class Alloc>
inline bool operator>(const uvector<Tp,Alloc>& lhs, const uvector<Tp,Alloc>& rhs)
{
	return rhs < lhs;
}

template <class Tp, class Alloc>
inline bool operator>=(const uvector<Tp,Alloc>& lhs, const uvector<Tp,Alloc>& rhs)
{
	return rhs <= lhs;
}

template <class Tp, class Alloc>
inline void swap(uvector<Tp,Alloc>& x, uvector<Tp,Alloc>& y)
{
	x.swap(y);
}

#endif // STDEXT_UVECTOR_H
