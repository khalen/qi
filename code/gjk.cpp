//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Implementation of GJK algorithm in 2D
//

#include "basictypes.h"

#include "game.h"
#include "gjk.h"

v2
CircleShape_s::FarthestPointInDir(const v2& dir) const
{
	return origin + dir.normal() * radius;
}

v2
EllipseShape_s::FarthestPointInDir(const v2& dir) const
{
	const v2  dn            = dir.normal();
	const r32 distToEllipse = 1.0f / sqrtf((dn.x * dn.x) / (radii.x * radii.x) + (dn.y * dn.y) / (radii.y * radii.y));
	return origin + dn * distToEllipse;
}

v2
PolyShape_s::FarthestPointInDir(const v2& dir) const
{
	r32 farthestDot = points[0].dot(dir);
	u32 farthestIdx = 0;
	for (u32 idx = 1; idx < numPoints; idx++)
	{
		const r32 curDot = points[idx].dot(dir);
		if (curDot > farthestDot)
		{
			farthestIdx = idx;
			farthestDot = curDot;
		}
	}
	Assert(farthestIdx < numPoints);
	return points[farthestIdx];
}

static inline void
Support(Simplex_s* simplex, const GjkShape_s* a, const GjkShape_s* b, const v2& d)
{
	const v2 as = a->FarthestPointInDir(d);
	const v2 bs = b->FarthestPointInDir(-d);

	simplex->pts[simplex->numPts].sa = as;
	simplex->pts[simplex->numPts].sb = bs;
	simplex->pts[simplex->numPts].pt = as - bs;
}

struct Bary3_s
{
	r32 u, v, w;
};

struct BarySeg_s
{
	r32 u, v;
};

static inline void
OriginBarySeg(BarySeg_s* out, const v2& a, const v2& b)
{
	v2        ab    = b - a;
	const r32 abLen = VLength(&ab);

	if (abLen < Q_R32_EPSILON)
	{
		out->u = 1.0f;
		out->v = 0.0f;
	}
	else
	{
		const float invLen = 1.0f / abLen;
		ab *= invLen;
		out->v = b.dot(ab) * invLen;
		out->u = 1.0f - out->v;
	}
}

static inline r32
OriginDistAndNormalToSegment(BarySeg_s* bary, const v2& a, const v2& b, v2* normal)
{
	r32 dist = Q_R32_LARGE_NUMBER;

	OriginBarySeg(bary, a, b);

	if (bary->u <= 0.0)
	{
		*normal = -a;
	}
	else if (bary->v <= 0.0f)
	{
		*normal = -b;
	}
	else
	{
		*normal = -(a * bary->u + b * bary->v);
	}

	dist = VLength(normal);
	(*normal) *= 1.0f / dist;
	return dist;
}

// Compute the barycentric coordinates of the origin in triangle a, b, c
static inline bool
OriginBaryTri(Bary3_s* out, const v2& a, const v2& b, const v2& c)
{
	const r32 det = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
	if (det < Q_R32_EPSILON)
	{
		// Triangle is degenerate
		return false;
	}

	out->u = ((b.y - c.y) * -c.x + (c.x - b.x) * -c.y) / det;
	out->v = ((c.y - a.y) * -c.x + (a.x - c.x) * -c.y) / det;
	out->w = 1.0f - out->u - out->v;
	return true;
}

enum TriVoronoiRegion_e
{
	TriReg_A = 0,
	TriReg_AC,
	TriReg_C,
	TriReg_CB,
	TriReg_B,
	TriReg_BA,
	TriReg_ABC,
	TriReg_Degenerate
};

// Find the closest feature of this triangle to the origin - point, segment, or inside
static inline TriVoronoiRegion_e
FindOriginVoronoiRegion(const v2& a, const v2& b, const v2& c)
{
	Bary3_s baryTri;

	if (!OriginBaryTri(&baryTri, a, b, c))
		return TriReg_Degenerate;

	if (baryTri.u >= 0.0f && baryTri.v >= 0.0f && baryTri.w >= 0.0f)
		return TriReg_ABC;

	BarySeg_s baryAb, baryCa, baryBc;
	OriginBarySeg(&baryAb, a, b);
	OriginBarySeg(&baryCa, c, a);
	OriginBarySeg(&baryBc, b, c);

	if (baryAb.u <= 0.0f && baryCa.v <= 0.0f)
		return TriReg_A;
	if (baryBc.v <= 0.0f && baryCa.u <= 0.0f)
		return TriReg_C;
	if (baryAb.v <= 0.0f && baryBc.u <= 0.0f)
		return TriReg_B;

	if (baryAb.v > 0.0f && baryAb.u > 0.0f && baryTri.w <= 0.0f)
		return TriReg_BA;
	if (baryCa.v > 0.0f && baryCa.u > 0.0f && baryTri.v <= 0.0f)
		return TriReg_AC;
	if (baryBc.v > 0.0f && baryBc.u > 0.0f && baryTri.u <= 0.0f)
		return TriReg_CB;

	Assert(0);
	return TriReg_ABC;
}

#if 0
static inline bool
DoSimplexSegment(Simplex_s* simplex, v2* dir)
{
	const v2 b = simplex->pts[1].pt;
	const v2 a = simplex->pts[0].pt;

	BarySeg_s barySeg;
	OriginDistAndNormalToSegment(&barySeg, a, b, dir);

	Assert(barySeg.v >= 0.0f);

	return false;
}

static inline bool
DoSimplexTriangle(Simplex_s* simplex, v2* dir)
{
	const SimplexPt_s c = simplex->pts[0];
	const SimplexPt_s b = simplex->pts[1];
	const SimplexPt_s a = simplex->pts[2];

	const TriVoronoiRegion_e region = FindOriginVoronoiRegion(a.pt, b.pt, c.pt);
	switch (region)
	{
	case TriReg_A:
		simplex->pts[0] = a;
		simplex->numPts = 1;
		// simplex[0] = a;
		*dir = -a.pt;
		break;
	case TriReg_AC:
		// 0 is already C
		simplex->pts[1] = a;
		simplex->numPts = 2;
		*dir            = (a.pt - c.pt).perp();
		break;
	case TriReg_C:
		// 0 is already C
		simplex->numPts = 1;
		*dir            = -c.pt;
		break;
	case TriReg_CB:
		simplex->pts[0] = b;
		simplex->pts[1] = c;
		simplex->numPts = 2;
		*dir            = (c.pt - b.pt).perp();
		break;
	case TriReg_B:
		simplex->pts[0] = b;
		simplex->numPts = 1;
		*dir            = -b.pt;
		break;
	case TriReg_BA:
		// 1 is already B
		simplex->pts[0] = a;
		simplex->numPts = 2;
		*dir            = (b.pt - a.pt).perp();
		break;
	case TriReg_Degenerate:
	{
		// Triangle is degenerate, use the longest edge
		r32 abLen = (b.pt - a.pt).lenSq();
		r32 bcLen = (c.pt - b.pt).lenSq();
		r32 caLen = (a.pt - c.pt).lenSq();
		v2  sa, sb;
		if (abLen > bcLen)
		{
			if (abLen > caLen)
			{
				sa = b.pt;
				sb = a.pt;
			}
			else
			{
				sa = c.pt;
				sb = a.pt;
			}
		}
		else
		{
			if (bcLen > caLen)
			{
				sa = c.pt;
				sb = b.pt;
			}
			else
			{
				sa = c.pt;
				sb = a.pt;
			}
		}
		simplex->pts[0] = b;
		simplex->pts[1] = a;
		simplex->numPts = 2;
		*dir            = (sb - sa).perp();
		if (dir->dot(sa) < 0.0f)
			*dir = -*dir;

        break;
	}

	case TriReg_ABC:
		return true;
	}

	return false;
}
#else
static inline bool
DoSimplexSegment(Simplex_s* simplex, v2* dir)
{
    const v2& b = simplex->pts[0].pt;
    const v2& a = simplex->pts[1].pt;
    const v2 a0 = -a;
    v2 ab = b - a;
    *dir = VTriple(ab, a0, ab);
    if (dir->lenSq() < Q_R32_EPSILON)
        *dir = ab.perp();

    return false;
}


static inline bool
DoSimplexTriangle(Simplex_s* simplex, v2* dir)
{
    const v2& c = simplex->pts[0].pt;
    const v2& b = simplex->pts[1].pt;
    const v2& a = simplex->pts[2].pt;
    const v2 a0 = -a;
    v2 ab = b - a;
    v2 ac = c - a;

    v2 acPerp = VTriple(ab, ac, ac);

    if (acPerp.dot(a0) >= 0.0f)
    {
        *dir = acPerp;
    }
    else
    {
        v2 abPerp = VTriple(ac, ab, ab);
        if (abPerp.dot(a0) < 0.0f)
            return true;

        simplex->pts[0] = simplex->pts[1];
        *dir = abPerp;
    }

    simplex->pts[1] = simplex->pts[2];
    simplex->numPts = 2;

    return false;
}
#endif

static bool
DoSimplex(Simplex_s* simplex, v2* dir)
{
	switch (simplex->numPts)
	{
	case 0:
	case 1:
		Assert(0);
		break;

	case 2:
		return DoSimplexSegment(simplex, dir);

	case 3:
		return DoSimplexTriangle(simplex, dir);

	default:
		Assert(0);
		return false;
	}

	return false;
}

static r32
OriginDistToClosestEdge(PolyShape_s* poly, v2* normal, u32* index)
{
	r32 nearestDist = Q_R32_LARGE_NUMBER;
	v2  bestNormal;
	u32 bestIdx;

	for (u32 pi = 0; pi < poly->numPoints; pi++)
	{
		const u32 pn = (pi + 1) % poly->numPoints;
		const v2& sa = poly->points[pi];
		const v2& sb = poly->points[pn];

		v2 seg  = sb - sa;
		v2 perp = seg.perp();
        v2 perpNormal;
        r32 curDist;

        if (perp.dot(perp) < Q_R32_EPSILON)
        {
            curDist = sa.len();
            perpNormal = -sa;
        }
        else
        {
            if (perp.dot(sa) < 0.0f)
                perp = -perp;

            perpNormal = perp.normal();
            curDist    = perpNormal.dot(sa);
        }

		if (curDist < nearestDist)
		{
			nearestDist = curDist;
			bestNormal  = perpNormal;
			bestIdx     = pi;
		}
	}

	*normal = bestNormal;
	*index  = bestIdx;
	return nearestDist;
}

static void
GetPenetrationDepthAndNormal(GjkResult_s* result, const GjkShape_s* shapeA, const GjkShape_s* shapeB)
{
	PolyShape_s spoly;
	spoly.numPoints = result->simplex.numPts;
	for (u32 i = 0; i < result->simplex.numPts; i++)
		spoly.points[i] = result->simplex.pts[i].pt;

	for (;;)
	{
		u32       index;
		v2        normal;
		const r32 distToOrigin = OriginDistToClosestEdge(&spoly, &normal, &index);

		v2 sa         = shapeA->FarthestPointInDir(normal);
		v2 sb         = shapeB->FarthestPointInDir(-normal);
		v2 newSupport = sa - sb;

		const r32 newSupDist = newSupport.dot(normal);
		if (newSupDist - distToOrigin < 0.02)
		{
			result->penetrationDepth  = distToOrigin;
			result->penetrationNormal = -normal;
			break;
		}

		Assert(spoly.numPoints < QI_GJK_MAX_POLY_POINTS - 1);
		for (u32 moveIdx = spoly.numPoints - 1; moveIdx > index; moveIdx--)
			spoly.points[moveIdx + 1] = spoly.points[moveIdx];

		spoly.points[index + 1] = newSupport;
		spoly.numPoints++;
	}
}

bool
Qi_Gjk(GjkResult_s* result, const GjkShape_s* shapeA, const GjkShape_s* shapeB)
{
	memset(result, 0, sizeof(*result));

	Simplex_s* simplex = &result->simplex;
    Simplex_s prevSimplex;

	v2 dir = shapeB->origin - shapeA->origin;
	Support(simplex, shapeA, shapeB, dir);
	dir = -simplex->pts[simplex->numPts].pt;
	simplex->numPts++;
    prevSimplex = *simplex;

	for (;;)
	{
		Support(simplex, shapeA, shapeB, dir);
		if (VDot(simplex->pts[simplex->numPts].pt, dir) < 0.0f)
		{
			result->intersected = false;
			return false;
		}

        if (dir.dot(dir) < Q_R32_EPSILON)
        {
			result->intersected = true;
			GetPenetrationDepthAndNormal(result, shapeA, shapeB);
			return true;
        }

		simplex->numPts++;
		if (DoSimplex(simplex, &dir))
		{
			result->intersected = true;
			GetPenetrationDepthAndNormal(result, shapeA, shapeB);
			return true;
		}
        prevSimplex = *simplex;
	}
}
