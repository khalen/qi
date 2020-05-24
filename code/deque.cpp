//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Deque implementation
//

#include "basictypes.h"

#include "game.h"
#include "deque.h"

template<typename T, int size>
Deque<T, size>* Deque<T,size>::Make(void* mem)
{
    Deque<T, size>* dq = reinterpret_cast<Deque<T, size>*>(mem);
    dq->Init();
    return dq;
}
