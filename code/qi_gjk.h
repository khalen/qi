#ifndef __QI_GJK_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// 2D implementation of GJK
//

#include "qi_math.h"

struct SimplexPt_s
{
	v2 pt; // Support point
	v2 sa;
	v2 sb;
	v2 dir;
};

struct Simplex_s
{
	SimplexPt_s pts[3];
	u32         numPts;
};

struct GjkResult_s
{
	Simplex_s simplex;
	v2        closestPoint;
	v2        penetrationNormal;
	r32       penetrationDepth;
	bool      intersected;
};

struct GjkShape_s
{
	GjkShape_s() { origin = V2(0.0f, 0.0f); }
	GjkShape_s(const v2& iOrigin)
	    : origin(iOrigin)
	{
	}

	virtual v2 FarthestPointInDir(const v2& dir) const = 0;

	v2 origin;
};

struct CircleShape_s : public GjkShape_s
{
	CircleShape_s(const v2& iOrigin, const r32 iRadius)
	    : GjkShape_s(iOrigin)
	    , radius(iRadius)
	{
	}

	v2 FarthestPointInDir(const v2& dir) const;

	r32 radius;
};

struct EllipseShape_s : public GjkShape_s
{
	EllipseShape_s(const v2& iOrigin, const v2& iRadii)
	    : GjkShape_s(iOrigin)
	    , radii(iRadii)
	{
	}

	v2 FarthestPointInDir(const v2& dir) const;

	v2 radii;
};

#define QI_GJK_MAX_POLY_POINTS 32
struct PolyShape_s : public GjkShape_s
{
	PolyShape_s() {}

	PolyShape_s(v2* iPoints, const u32 iNumPoints)
	    : numPoints(iNumPoints)
	{
		Assert(numPoints > 0 && numPoints < QI_GJK_MAX_POLY_POINTS);

		r32 invCount = 1.0f / numPoints;
		v2  center   = {};
		for (u32 idx = 0; idx < numPoints; idx++)
		{
			center += iPoints[idx] * invCount;
			points[idx] = iPoints[idx];
		}

		origin = center;
	}

	v2 FarthestPointInDir(const v2& dir) const;

	v2  points[QI_GJK_MAX_POLY_POINTS];
	u32 numPoints;
};

struct BoxShape_s : public PolyShape_s
{
	BoxShape_s(const v2& iOrigin, const v2& iRadii)
	{
		origin    = iOrigin;
		points[0] = V2(iOrigin.x - iRadii.x, iOrigin.y - iRadii.y);
		points[1] = V2(iOrigin.x - iRadii.x, iOrigin.y + iRadii.y);
		points[2] = V2(iOrigin.x + iRadii.x, iOrigin.y + iRadii.y);
		points[3] = V2(iOrigin.x + iRadii.x, iOrigin.y - iRadii.y);
		numPoints = 4;
	}
};

extern bool Qi_Gjk(GjkResult_s* result, const GjkShape_s* shapeA, const GjkShape_s* shapeB);

#define __QI_GJK_H
#endif // #ifndef __QI_GJK_H
