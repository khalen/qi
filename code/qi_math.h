#ifndef __QI_MATH_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Basic math constants, inlines, and types
//

#include "basictypes.h"
#include <math.h>

#define Q_PI 3.1415927f
#define Q_2PI 2.0f * Q_PI

#define Q_PIG 3.141592653589793
#define Q_2PIG 2.0 * Q_PIG

#define Q_RAD_DEG_FACTOR (180.0f / Q_PI)
#define Q_DEG_RAD_FACTOR (1.0f / Q_RAD_DEG_FACTOR)

#define Q_R32_EPSILON 1.0e-5f
#define Q_R32_LARGE_NUMBER 1.0e6f

struct v2
{
	union {
		r32 f[2];
		struct
		{
			r32 x;
			r32 y;
		};
	};

	r32 dot(const v2& b) const { return x * b.x + y * b.y; }

	v2  perp() const
	{
		v2 result;
		result.x = y;
		result.y = -x;
		return result;
	}

	r32 lenSq() const { return x * x + y * y; }
	r32 len() const { return sqrtf(lenSq()); }

	v2  normal() const
	{
		v2        result;
		const r32 lenSq = x * x + y * y;
		if (lenSq < Q_R32_EPSILON)
		{
			result.x = 0.0f;
			result.y = 0.0f;
		}
		else
		{
			const r32 invLength = 1.0f / sqrtf(lenSq);
			result.x            = x * invLength;
			result.y            = y * invLength;
		}
		return result;
	}
};

inline v2
V2(const r32 x, const r32 y)
{
	v2 result;
	result.x = x;
	result.y = y;
	return result;
}

inline v2
operator-(const v2& a)
{
	return V2(-a.x, -a.y);
}

inline v2
operator+(const v2& a, const v2& b)
{
	return V2(a.x + b.x, a.y + b.y);
}

inline v2
operator-(const v2& a, const v2& b)
{
	return a + -b;
}

inline v2&
operator+=(v2& a, const v2& b)
{
	a.x += b.x;
	a.y += b.y;
	return a;
}

inline v2 operator*(const v2& a, const r32 s)
{
	return V2(a.x * s, a.y * s);
}

inline v2 operator*(const r32 s, const v2& a)
{
	return a * s;
}

inline v2&
operator*=(v2& a, const r32 s)
{
	a.x *= s;
	a.y *= s;
	return a;
}

struct v3
{
	union {
		r32 f[3];
		struct
		{
			r32 x;
			r32 y;
			r32 z;
		};
	};
};

struct v4
{
	union {
		r32 f[4];
		struct
		{
			r32 x;
			r32 y;
			r32 z;
			r32 w;
		};
	};
	v4(r32 xi, r32 yi, r32 zi, r32 wi)
	    : x(xi)
	    , y(yi)
	    , z(zi)
	    , w(wi)
	{
	}
};

struct Color_s
{
	union {
		r32 f[4];
		struct
		{
			r32 r;
			r32 g;
			r32 b;
			r32 a;
		};
	};
	Color_s(r32 xi, r32 yi, r32 zi, r32 wi)
	    : r(xi)
	    , g(yi)
	    , b(zi)
	    , a(wi)
	{
	}
};

static inline void
VAdd(v2* out, const v2* ina, const v2* inb)
{
	out->x = ina->x + inb->x;
	out->y = ina->y + inb->y;
}

static inline void
VSub(v2* out, const v2* ina, const v2* inb)
{

	out->x = ina->x - inb->x;
	out->y = ina->y - inb->y;
}

static inline r32
VDot(const v2& ina, const v2& inb)
{
	return ina.x * inb.x + ina.y * inb.y;
}

static inline r32
VDot(const v2* ina, const v2* inb)
{
	return ina->x * inb->x + ina->y * inb->y;
}

static inline void
VMulScalar(v2* out, const v2* input, const r32 scalar)
{
	out->x = input->x * scalar;
	out->y = input->y * scalar;
}

static inline r32
VLength(const v2* input)
{
	return sqrtf(VDot(input, input));
}

static inline void
VNormalizeInto(v2* out, v2* input)
{
	const r32 lengthSq = VDot(input, input);
	if (lengthSq > 0.000001)
	{
		const r32 invLength = 1.0f / VLength(input);
		VMulScalar(out, input, invLength);
	}
	else
	{
		out->x = out->y = 0.0f;
	}
}

static inline void
VNormalize(v2* inOut)
{
	VNormalizeInto(inOut, inOut);
}

// This gives a vector perpendicular to ac that points in the direction of b
static inline v2
VTriple(const v2& a, const v2& b, const v2& c)
{
	const r32 ac = a.dot(c);
	const r32 bc = b.dot(c);

	v2 result;
	result.x = b.x * ac - a.x * bc;
	result.y = b.y * ac - a.y * bc;

	return result;
}

static inline void
VAdd(v3* out, const v3* ina, const v3* inb)
{
	out->x = ina->x + inb->x;
	out->y = ina->y + inb->y;
	out->z = ina->z + inb->z;
}

static inline void
Vec3Sub(v3* out, const v3* ina, const v3* inb)
{
	out->x = ina->x - inb->x;
	out->y = ina->y - inb->y;
	out->z = ina->z - inb->z;
}

static inline r32
Vec3Dot(const v3* ina, const v3* inb)
{
	return ina->x * inb->x + ina->y * inb->y + ina->z * inb->z;
}

static inline void
Vec3MulScalar(v3* out, const v3* input, const r32 scalar)
{
	out->x = input->x * scalar;
	out->y = input->y * scalar;
	out->z = input->z * scalar;
}

static inline r32
Vec3Length(const v3* input)
{
	return sqrtf(Vec3Dot(input, input));
}

static inline void
Vec3NormalizeInto(v3* out, v3* input)
{
	const r32 invLength = 1.0f / Vec3Length(input);
	Vec3MulScalar(out, input, invLength);
}

static inline void
Vec3Normalize(v3* inOut)
{
	Vec3NormalizeInto(inOut, inOut);
}

static inline void
Vec4Add(v4* out, const v4* ina, const v4* inb)
{
	out->x = ina->x + inb->x;
	out->y = ina->y + inb->y;
	out->z = ina->z + inb->z;
	out->w = ina->w + inb->w;
}

static inline void
Vec4Sub(v4* out, const v4* ina, const v4* inb)
{
	out->x = ina->x - inb->x;
	out->y = ina->y - inb->y;
	out->z = ina->z - inb->z;
	out->w = ina->w - inb->w;
}

static inline r32
Vec4Dot(const v4* ina, const v4* inb)
{
	return ina->x * inb->x + ina->y * inb->y + ina->z * inb->z + ina->w * inb->w;
}

static inline void
Vec4AddScalar(v4* out, const v4* input, const r32 scalar)
{
	out->x = input->x + scalar;
	out->y = input->y + scalar;
	out->z = input->z + scalar;
	out->w = input->w + scalar;
}

static inline void
Vec4MulScalar(v4* out, const v4* input, const r32 scalar)
{
	out->x = input->x * scalar;
	out->y = input->y * scalar;
	out->z = input->z * scalar;
	out->w = input->w * scalar;
}

static inline r32
Vec4Length(const v4* input)
{
	return sqrtf(Vec4Dot(input, input));
}

static inline void
Vec4NormalizeInto(v4* out, v4* input)
{
	const r32 invLength = 1.0f / Vec4Length(input);
	Vec4MulScalar(out, input, invLength);
}

static inline void
Vec4Normalize(v4* inOut)
{
	Vec4NormalizeInto(inOut, inOut);
}

template<typename T>
T
Qi_Clamp(const T val, const T minT, const T maxT)
{
	if (val < minT)
		return minT;
	if (val > maxT)
		return maxT;
	return val;
}

static inline u32
Qi_PackColor(const Color_s* c)
{
	v4 adj = {0.0f, 0.0f, 0.0f, 0.0f};
	v4 col = {c->r, c->g, c->b, c->a};
	Vec4AddScalar(&adj, &col, 0.5f);
	Vec4MulScalar(&adj, &adj, 255.0f);
	return ((u32)adj.x << 24) | ((u32)adj.y << 16) | ((u32)adj.z << 8) | ((u32)adj.w);
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

template<typename T>
static inline void
Swap(T& a, T& b)
{
	T tmp = a;
	a     = b;
	b     = tmp;
}

#define __QI_MATH_H
#endif
