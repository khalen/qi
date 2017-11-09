//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Simple fixed size array implementation
//

#include "basictypes.h"

#include "qi.h"
#include "qi_array.h"

template< typename T, int SIZE >
Array<T, SIZE>* Array<T, SIZE>::Make(void* mem)
{
    Array<T, SIZE>* rVal = reinterpret_cast< Array< T, SIZE >* >(mem);
    rVal->Init();
    return rVal;
}
