#ifndef __QI_DEQUE_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Fixed size double ended queue implemented as a circular array

#define QI_DEFAULT_DEQUE_SIZE 4096
template<typename T, int size = QI_DEFAULT_DEQUE_SIZE>
struct Deque
{
	T   store[size];
	i32 front, back;

	static Deque<T, size>* Make(void* mem);

	void Init() { front = back = -1; }

	bool IsFull() { return ((back + 1) % size) == front; }
	bool IsEmpty() { return back == -1; }

	void PushR(const T& item)
	{
		Assert(!IsFull());
		if (IsEmpty())
		{
			front = back = 0;
			store[0]     = item;
		}
		else
		{
			back        = (back + 1) % size;
			store[back] = item;
		}
	}

	T PopR()
	{
		Assert(!IsEmpty());
		T* rValPtr = &store[back];
		if (front == back)
			Init();
		else
			back = (back - 1 + size) % size;
		return *rValPtr;
	}

	void PushF(const T& item)
	{
		Assert(!IsFull());
		if (IsEmpty())
		{
			front = back = 0;
			store[0]     = item;
		}
		else
		{
			front        = (front - 1 + size) % size;
			store[front] = item;
		}
	}

	T PopF()
	{
		Assert(!IsEmpty());
		T* rValPtr = &store[front];
		if (front == back)
			Init();
		else
			front = (front + 1) % size;
		return *rValPtr;
	}
};

#define __QI_DEQUE_H
#endif // #ifndef __QI_DEQUE_H
