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

struct Vec2_s
{
	union {
		r32 f[2];
		struct
		{
			r32 x;
			r32 y;
		};
	};
};

struct Vec3_s
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

struct Vec4_s
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
	Vec4_s(r32 xi, r32 yi, r32 zi, r32 wi)
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
Vec2Add(Vec2_s* out, const Vec2_s* ina, const Vec2_s* inb)
{
	out->x = ina->x + inb->x;
	out->y = ina->y + inb->y;
}

static inline void
Vec2Sub(Vec2_s* out, const Vec2_s* ina, const Vec2_s* inb)
{

	out->x = ina->x - inb->x;
	out->y = ina->y - inb->y;
}

static inline r32
Vec2Dot(const Vec2_s* ina, const Vec2_s* inb)
{
	return ina->x * inb->x + ina->y * inb->y;
}

static inline void
Vec2MulScalar(Vec2_s* out, const Vec2_s* input, const r32 scalar)
{
	out->x = input->x * scalar;
	out->y = input->y * scalar;
}

static inline r32
Vec2Length(const Vec2_s* input)
{
	return sqrtf(Vec2Dot(input, input));
}

static inline void
Vec2NormalizeInto(Vec2_s* out, Vec2_s* input)
{
	const r32 invLength = 1.0f / Vec2Length(input);
	Vec2MulScalar(out, input, invLength);
}

static inline void
Vec2Normalize(Vec2_s* inOut)
{
	Vec2NormalizeInto(inOut, inOut);
}

static inline void
Vec3Add(Vec3_s* out, const Vec3_s* ina, const Vec3_s* inb)
{
	out->x = ina->x + inb->x;
	out->y = ina->y + inb->y;
	out->z = ina->z + inb->z;
}

static inline void
Vec3Sub(Vec3_s* out, const Vec3_s* ina, const Vec3_s* inb)
{
	out->x = ina->x - inb->x;
	out->y = ina->y - inb->y;
	out->z = ina->z - inb->z;
}

static inline r32
Vec3Dot(const Vec3_s* ina, const Vec3_s* inb)
{
	return ina->x * inb->x + ina->y * inb->y + ina->z * inb->z;
}

static inline void
Vec3MulScalar(Vec3_s* out, const Vec3_s* input, const r32 scalar)
{
	out->x = input->x * scalar;
	out->y = input->y * scalar;
	out->z = input->z * scalar;
}

static inline r32
Vec3Length(const Vec3_s* input)
{
	return sqrtf(Vec3Dot(input, input));
}

static inline void
Vec3NormalizeInto(Vec3_s* out, Vec3_s* input)
{
	const r32 invLength = 1.0f / Vec3Length(input);
	Vec3MulScalar(out, input, invLength);
}

static inline void
Vec3Normalize(Vec3_s* inOut)
{
	Vec3NormalizeInto(inOut, inOut);
}

static inline void
Vec4Add(Vec4_s* out, const Vec4_s* ina, const Vec4_s* inb)
{
	out->x = ina->x + inb->x;
	out->y = ina->y + inb->y;
	out->z = ina->z + inb->z;
	out->w = ina->w + inb->w;
}

static inline void
Vec4Sub(Vec4_s* out, const Vec4_s* ina, const Vec4_s* inb)
{
	out->x = ina->x - inb->x;
	out->y = ina->y - inb->y;
	out->z = ina->z - inb->z;
	out->w = ina->w - inb->w;
}

static inline r32
Vec4Dot(const Vec4_s* ina, const Vec4_s* inb)
{
	return ina->x * inb->x + ina->y * inb->y + ina->z * inb->z + ina->w * inb->w;
}

static inline void
Vec4AddScalar(Vec4_s* out, const Vec4_s* input, const r32 scalar)
{
	out->x = input->x + scalar;
	out->y = input->y + scalar;
	out->z = input->z + scalar;
	out->w = input->w + scalar;
}

static inline void
Vec4MulScalar(Vec4_s* out, const Vec4_s* input, const r32 scalar)
{
	out->x = input->x * scalar;
	out->y = input->y * scalar;
	out->z = input->z * scalar;
	out->w = input->w * scalar;
}

static inline r32
Vec4Length(const Vec4_s* input)
{
	return sqrtf(Vec4Dot(input, input));
}

static inline void
Vec4NormalizeInto(Vec4_s* out, Vec4_s* input)
{
	const r32 invLength = 1.0f / Vec4Length(input);
	Vec4MulScalar(out, input, invLength);
}

static inline void
Vec4Normalize(Vec4_s* inOut)
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
    Vec4_s adj = {0.0f, 0.0f, 0.0f, 0.0f};
    Vec4_s col = {c->r, c->g, c->b, c->a};
	Vec4AddScalar(&adj, &col, 0.5f);
	Vec4MulScalar(&adj, &adj, 255.0f);
	return ((u32)adj.x << 24) | ((u32)adj.y << 16) | ((u32)adj.z << 8) | ((u32)adj.w);
}

#define __QI_MATH_H
#endif
