#ifndef __QI_UTIL_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// General purpose utility functions
//

#include "basictypes.h"
#if !HAS(IS_CLANG)
#include <intrin.h>
#endif
#include <stddef.h>

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
#if HAS(IS_CLANG)
    return __builtin_ffs(v);
#else
    return _tzcnt_u32(v);
#endif
}

template <>
inline i32
BitScanRight<u64>(const u64 v)
{
#if HAS(IS_CLANG)
    return __builtin_ffsll(v);
#else
	return _tzcnt_u64(v);
#endif
}

extern const char* VS(const char* msg, ...)
#if HAS(IS_CLANG)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

// FIXME: This is slow. Use intrinsics.
constexpr i32 ShiftFromMask(const u32 mask)
{
	if (mask == 0)
		return -1;

	u32 r = 0;
	while ((mask & (1 << r)) == 0)
		r++;
	return r;
}

#define __QI_UTIL_H
#endif // #ifndef __QI_UTIL_H

