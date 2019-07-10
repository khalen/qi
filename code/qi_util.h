#ifndef __QI_UTIL_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// General purpose utility functions
//

#include "basictypes.h"

template<typename T, size_t N>
constexpr int countof(T (&)[N])
{
    return N;
}

template<typename T>
static inline void
Swap(T& a, T& b)
{
    T tmp = a;
    a     = b;
    b     = tmp;
}

template<typename T>
static inline T
Min(const T& a, const T& b)
{
    return a < b ? a : b;
}

template<typename T>
static inline T
Max(const T& a, const T& b)
{
    return a > b ? a : b;
}

template <typename T>
static constexpr inline const T
NextHigherPow2(const T vi)
{
    T v = vi;
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    if (sizeof(T) > 4)
        v |= v >> 32;
    v++;
    return v;
}

template <typename T>
static constexpr inline const T
NextLowerPow2(const T vi)
{
    return NextHigherPow2(vi) >> 1;
}

template <typename T>
inline i32
BitScanRight(const T v)
{
    return ffs(v);
}

template <>
inline i32
BitScanRight<u64>(const u64 v)
{
    return ffsll(v);
}

#define __QI_UTIL_H
#endif // #ifndef __QI_UTIL_H
