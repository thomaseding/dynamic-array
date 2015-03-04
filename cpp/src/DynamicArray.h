// Copyright (c) 2015, Thomas Eding
// All rights reserved.
// 
// Homepage: https://github.com/thomaseding/dynamic-array
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// The views and conclusions contained in the software and documentation are those
// of the authors and should not be interpreted as representing official policies, 
// either expressed or implied, of the FreeBSD Project.

#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <utility>
#include <stdexcept>


//////////////////////////////////////////////////////////////////////


template <typename T, typename Alloc = std::allocator<T>>
class DynamicArray {
public:
	typedef T value_type;
	typedef Alloc allocator_type;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef value_type & reference;
	typedef value_type const & const_reference;
	typedef std::allocator_traits<Alloc>::pointer pointer;
	typedef std::allocator_traits<Alloc>::const_pointer const_pointer;
	typedef T * iterator;
	typedef T const * const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::const_reverse_iterator<iterator> const_reverse_iterator;

public:
	~DynamicArray ()
	{
		Clear();
	}

	DynamicArray ()
		: head(nullptr)
		, tail(nullptr)
		, tailCap(nullptr)
	{}

	DynamicArray (size_type n, T const & value)
		: head(Init(n, value))
		, tail(head + n)
		, tailCap(tail)
	{}

	DynamicArray (DynamicArray const & other)
		: head(Init(other))
		, tail(head + other.size())
		, tailCap(head + other.capacity())
	{}

	DynamicArray (DynamicArray && other)
		: head(std::move(other.head))
		, tail(std::move(other.tail))
		, tailCap(std::move(other.tailCap))
	{
		other.head = nullptr;
		other.tail = nullptr;
		other.tailCap = nullptr;
	}

	DynamicArray & operator= (DynamicArray const & other)
	{
		head = Init(other);
		tail = head + other.size();
		tailCap = tail;
		return *this;
	}

	DynamicArray & operator= (DynamicArray && other)
	{
		head = std::move(other.head);
		tail = std::move(other.tail);
		tailCap = std::move(other.tailCap);
		other.head = nullptr;
		other.tail = nullptr;
		other.tailCap = nullptr;
		return *this;
	}

	allocator_type get_allocator () const
	{
		return Alloc();
	}

	bool empty () const
	{
		return size() == 0;
	}

	size_type size () const
	{
		return static_cast<size_type>(tail - head);
	}

	size_type capacity () const
	{
		return static_cast<size_type>(tailCap - head);
	}

	size_type max_size () const
	{
		return std::numeric_limits<size_type>::max()
	}

	void clear ()
	{
		Clear();
		head = nullptr;
		tail = nullptr;
		tailCap = nullptr;
	}

	T & operator[] (size_type const index)
	{
		return head[index];
	}

	T const & operator[] (size_type const index) const
	{
		return head[index];
	}

	T & at (size_type index)
	{
		RangeGuard(index);
		return operator[](index);
	}

	T const & at (size_type index) const
	{
		RangeGuard(index);
		return operator[](index);
	}

	T & front ()
	{
		return *head;
	}

	T const & front () const
	{
		return *head;
	}

	T & back ()
	{
		return *(tail - 1);
	}

	T const & back () const
	{
		return *(tail - 1);
	}

	T * release ()
	{
		T * p = head;
		head = nullptr;
		tail = nullptr;
		tailCap = nullptr;
		return p;
	}

	T * data ()
	{
		return head;
	}

	T const * data () const
	{
		return head;
	}

	iterator begin ()
	{
		return head;
	}

	const_iterator begin () const
	{
		return head;
	}

	const_iterator cbegin () const
	{
		return begin();
	}

	iterator end ()
	{
		return tail;
	}

	const_iterator end () const
	{
		return tail;
	}

	const_iterator cend () const
	{
		return end();
	}

	reverse_iterator rbegin ()
	{
		return reverse_iterator(end());
	}

	const_reverse_iterator rbegin () const
	{
		return const_reverse_iterator(end());
	}

	const_reverse_iterator crbegin () const
	{
		return rbegin();
	}

	reverse_iterator rend ()
	{
		return reverse_iterator(begin());
	}

	const_reverse_iterator rend () const
	{
		return const_reverse_iterator(begin());
	}

	const_reverse_iterator crend () const
	{
		return rend();
	}

	void reserve (size_type const n)
	{
		if (n > size()) {
			Reallocate(n);
		}
	}

	void resize (size_type const newSize, T const & value = T())
	{
		size_type const oldSize = size();
		if (newSize > oldSize) {
			Reallocate(newSize);
			Alloc alloc(get_allocator());
			for (size_type i = oldSize; i < newSize; ++i) {
				alloc.construct(head + i, value);
			}
			tail = head + newSize;
		}
		else if (newSize < oldSize) {
			Alloc alloc(get_allocator());
			for (size_t i = size(); i-- > newSize; ) {
				alloc.destroy(head + i);
			}
			tail = head + low;
		}
	}

	void push_back (T && value)
	{
		PushBack(value);
	}

	void push_back (T const & value)
	{
		PushBack(value);
	}

	void pop_back ()
	{
		get_allocator().destroy(tail - 1);
		--tail;
	}

private:
	template <typename T2>
	void PushBack (T2 && value)
	{
		if (size() == capacity()) {
			AmortizedGrow();
		}
		get_allocator().construct(tail, std::forward<T2>(value));
		++tail;
	}

	size_type AmortizedGrowSize () const
	{
		size_type const n = size();
		return (n / 2) + n + 1;
	}

	void AmortizedGrow ()
	{
		Realloc(AmortizedGrowSize());
	}

	void RangeGuard (size_type const index) const
	{
		if (index > size()) {
			throw std::out_of_range("Index out of range");
		}
	}

	static T * Init (size_type const n, T const & value)
	{
		Alloc alloc(get_allocator);
		T * head = alloc.allocate(n);
		for (size_t i = 0; i < n; ++i) {
			alloc.construct(head + i, value);
		}
		return head;
	}

	static T * Init (DynamicArray const & arr)
	{
		size_type const n = arr.size();
		Alloc alloc(get_allocator);
		T * head = alloc.allocate(n);
		for (size_t i = 0; i < n; ++i) {
			alloc.construct(head + i, arr[i]);
		}
		return head;
	}

	void Realloc (size_t const newCapacity)
	{
		Alloc alloc;
		T * newHead = alloc.allocate(newCapacity);
		size_t const n = size();
		for (size_t i = 0; i < n; ++i) {
			alloc.construct(newHead + i, head[i]);
		}
		head = newHead;
		tail = newHead + n;
		tailCap = newHead + newCapacity;
	}

	void Clear ()
	{
		Alloc alloc(get_allocator());
		for (size_t i = size(); i-- > 0; ) {
			alloc.destroy(head + i);
		}
		alloc.deallocate(head, capacity());
	}

private:
	T * head;
	T * tail;
	T * tailCap;
};








