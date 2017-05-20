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
		float f[2];
		struct
		{
			float x;
			float y;
		};
	};
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

static inline void Vec2MulScalar(Vec2_s* out, const Vec2_s* input, const r32 scalar)
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

template<typename T>
T Qi_Clamp(const T val, const T minT, const T maxT)
{
    if ( val < minT )
        return minT;
    if ( val > maxT )
        return maxT;
    return val;
}

#define __QI_MATH_H
#endif
