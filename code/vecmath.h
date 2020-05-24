#ifndef __QI_VECMATH_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Vector math functions and overloaded operators
//

#include "qi_vecmath_internal.h"

template<typename TypeStruct>
inline auto operator+(const Vec2D<TypeStruct>& a, const Vec2D<TypeStruct>& b)
{
    return Vec2D<TypeStruct>(a.x + b.x, a.y + b.y, a.z + b.z);
}

template<typename TypeStruct>
inline auto operator-(const Vec2D<TypeStruct>& a, const Vec2D<TypeStruct>& b)
{
    return Vec2D<TypeStruct>(a.x - b.x, a.y - b.y, a.z - b.z);
}

template<typename TypeStruct>
inline auto operator*(const Vec2D<TypeStruct>& a, const Vec2D<TypeStruct>& b)
{
    return Vec2D<TypeStruct>(a.x * b.x, a.y * b.y, a.z * b.z);
}

template<typename TypeStruct>
inline auto operator*(const Vec2D<TypeStruct>& a, const typename TypeStruct::ComponentType& b)
{
    return Vec2D<TypeStruct>(a.x * b, a.y * b, a.z * b);
}

template<typename TypeStruct>
inline auto operator*(const typename TypeStruct::ComponentType& b, const Vec2D<TypeStruct>& a)
{
    return Vec2D<TypeStruct>(a.x * b, a.y * b, a.z * b);
}

#define __QI_VECMATH_H
#endif // #ifndef __QI_VECMATH_H
