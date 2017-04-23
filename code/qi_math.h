#ifndef __QI_MATH_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Basic math constants, inlines, and types
//

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

#define __QI_MATH_H
#endif
