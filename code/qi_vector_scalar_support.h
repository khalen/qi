#ifndef __QI_VECTOR_SCALAR_SUPPORT_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Vector traits for scalar types
//

#include "basictypes.h"

template<typename ScalarType, size_t Size>
class Vector;

template<typename T>
struct GetVectorTypeImplForScalar
{
    using Type = Vector<T, 1>;
};

template <>
struct GetVectorTypeImpl<bool> : GetVectorTypeImplForScalar<bool>
{};

template<>
struct GetVectorTypeImpl<r32> : GetVectorTypeImplForScalar<r32>
{};

template<>
struct GetVectorTypeImpl<r64> : GetVectorTypeImplForScalar<r64>
{};

template<>
struct GetVectorTypeImpl<u64> : GetVectorTypeImplForScalar<u64>
{};

template<>
struct GetVectorTypeImpl<i64> : GetVectorTypeImplForScalar<i64>
{};

template<>
struct GetVectorTypeImpl<u32> : GetVectorTypeImplForScalar<u32>
{};

template<>
struct GetVectorTypeImpl<i32> : GetVectorTypeImplForScalar<i32>
{};

template<>
struct GetVectorTypeImpl<u16> : GetVectorTypeImplForScalar<u16>
{};

template<>
struct GetVectorTypeImpl<i16> : GetVectorTypeImplForScalar<i16>
{};

template<>
struct GetVectorTypeImpl<u8> : GetVectorTypeImplForScalar<u8>
{};

template<>
struct GetVectorTypeImpl<i8> : GetVectorTypeImplForScalar<i8>
{};
#define __QI_VECTOR_SCALAR_SUPPORT_H
#endif // #ifndef __QI_VECTOR_SCALAR_SUPPORT_H
