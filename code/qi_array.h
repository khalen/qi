#ifndef __QI_ARRAY_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Fixed size array / stack
//

#include "has.h"

#define ARRAY_BOUNDS_CHECK HAS__

#define QI_DEFAULT_ARRAY_SIZE 1024
template<typename T, int SIZE = QI_DEFAULT_ARRAY_SIZE>
struct Array
{
	u32 front;
    u32 __pad[3];
	T   store[SIZE];

	static Array<T, SIZE>* Make(void* mem);

	void Init() { front = 0; }

	bool IsFull() { return front == SIZE; }
	bool IsEmpty() { return front == 0; }
	T*   Base() { return store; }
	u32  Num() { return front; }
	u32  Capacity() { return SIZE; }

	void Push(const T& item)
	{
		Assert(!IsFull());

		store[front] = item;
		front++;
	}

	T Pop()
	{
		Assert(!IsEmpty());
		return store[--front];
	}

	T Get(const u32 pos)
	{
#if HAS(ARRAY_BOUNDS_CHECK)
		Assert(pos < front);
#endif
		return store[pos];
	}

	T& At(const u32 pos)
	{
#if HAS(ARRAY_BOUNDS_CHECK)
		Assert(pos < front);
#endif
		return store[pos];
	}

	const T& At(const u32 pos) const
	{
#if HAS(ARRAY_BOUNDS_CHECK)
		Assert(pos < front);
#endif
		return store[pos];
	}

	T&       operator[](const u32 pos) { return At(pos); }
	const T& operator[](const u32 pos) const { return At(pos); }
};

#define __QI_ARRAY_H
#endif // #ifndef __QI_ARRAY_H
