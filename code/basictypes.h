// basictypes.h
// Copyright 2017 Jon Davis, Quantum Immortality Software (QIS)

#ifndef BASICTYPES_H
#define BASICTYPES_H

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

static_assert( sizeof( u8 ) == 1, "Bad size u8" );
static_assert( sizeof( i8 ) == 1, "Bad size i8" );
static_assert( sizeof( u16 ) == 2, "Bad size u16" );
static_assert( sizeof( i16 ) == 2, "Bad size i16" );
static_assert( sizeof( u32 ) == 4, "Bad size u32" );
static_assert( sizeof( i32 ) == 4, "Bad size i32" );
static_assert( sizeof( u64 ) == 8, "Bad size u64" );
static_assert( sizeof( i64 ) == 8, "Bad size i64" );

static_assert( sizeof( r32 ) == 4, "Bad size u64" );
static_assert( sizeof( r64 ) == 8, "Bad size i64" );

#endif // #ifndef BASICTYPES_H
