// basictypes.h
// Copyright 2017 Jon Davis, Quantum Immortality Software (QIS)

#ifndef BASICTYPES_H
#define BASICTYPES_H

#include "has.h"
#include <stdlib.h>
#include <stdint.h>

typedef unsigned char      u8;
typedef signed char        i8;
typedef unsigned short     u16;
typedef signed short       i16;
typedef unsigned int       u32;
typedef signed int         i32;
typedef unsigned long long u64;
typedef long long          i64;

typedef float  r32;
typedef double r64;

typedef r32 Time_t;
typedef r64 PreciseTime_t;

struct Color_u
{
	union {
		u32 value;
		struct
		{
			u8 a, b, g, r;
		};
	};

	Color_u(const u32 color)
		: value(color)
	{
	}

	Color_u(const u8 ia, const u8 ib, const u8 ig, const u8 ir)
		: a(ia)
		, b(ib)
		, g(ig)
		, r(ir)
	{
	}
};

static_assert(sizeof(u8) == 1, "Bad size u8");
static_assert(sizeof(i8) == 1, "Bad size i8");
static_assert(sizeof(u16) == 2, "Bad size u16");
static_assert(sizeof(i16) == 2, "Bad size i16");
static_assert(sizeof(u32) == 4, "Bad size u32");
static_assert(sizeof(i32) == 4, "Bad size i32");
static_assert(sizeof(u64) == 8, "Bad size u64");
static_assert(sizeof(i64) == 8, "Bad size i64");

static_assert(sizeof(r32) == 4, "Bad size u64");
static_assert(sizeof(r64) == 8, "Bad size i64");

#define internal static

#define KB(amt) ((amt) * (size_t)1024)
#define MB(amt) (KB(amt) * 1024)
#define GB(amt) (MB(amt) * 1024)
#define TB(amt) (GB(amt) * 1024)

#if defined(_MSC_VER)
#define BEGIN_PACKED_DEFS __pragma(pack(push)) __pragma(pack(1))

#define END_PACKED_DEFS __pragma(pack(pop))

#define PACKED
#else
#define BEGIN_PACKED_DEFS
#define END_PACKED_DEFS
#define PACKED __attribute__((packed))
#endif

#endif // #ifndef BASICTYPES_H
