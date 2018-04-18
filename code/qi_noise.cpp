//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Simple noise implementations
//

#include "basictypes.h"

#include "qi_noise.h"
#include <stdio.h>
#include <cmath>

Vector2 NoiseGenerator::GradientTable[NoiseGenerator::NumGradients];
static size_t GradCounts[NoiseGenerator::NumGradients];

r32
NoiseGenerator::Smoothed(r32 xCoord, r32 scale)
{
	const r32 scaledCoord = xCoord / scale;
	const r32 rCoord      = floor(scaledCoord);
	const r32 frac        = scaledCoord - rCoord;

	const r32 noiseFloor = GetRealFullRange((i32)rCoord);
	const r32 noiseCeil  = GetRealFullRange((i32)rCoord + 1);

	const r32 weight = frac * frac * (3.0f - 2.0f * frac);

	return noiseFloor + weight * (noiseCeil - noiseFloor);
}

r32
NoiseGenerator::Smoothed2D(const Vector2& unscaledCoords, r32 scale)
{
	const Vector2  coords  = unscaledCoords / scale;
	const Vector2  rCoords = floor(coords);
	const Vector2  UV    = coords - rCoords;
	const IVector2 iCoords(rCoords.x, rCoords.y);

	const r32 noiseLL = GetRealFullRange2D(iCoords.x, iCoords.y);
	const r32 noiseLR = GetRealFullRange2D(iCoords.x + 1, iCoords.y);
	const r32 noiseUR = GetRealFullRange2D(iCoords.x + 1, iCoords.y + 1);
	const r32 noiseUL = GetRealFullRange2D(iCoords.x, iCoords.y + 1);

	const r32 xweight = UV.x * UV.x * (3.0f - 2.0f * UV.x);
	const r32 yweight = UV.y * UV.y * (3.0f - 2.0f * UV.y);

	const r32 xValL = noiseLL + xweight * (noiseLR - noiseLL);
	const r32 xValU = noiseUL + xweight * (noiseUR - noiseUL);
	return xValL + yweight * (xValU - xValL);
}

void
NoiseGenerator::InitGradients()
{
	const r32 dTheta  = (2.0f * Q_PI) / (NumGradients + 0);
	const r32 gradMag = sqrtf(2.0f);
	r32       theta   = 0.0f;
	for (size_t i = 0; i < NumGradients; i++, theta += dTheta)
    {
		GradientTable[i] = Vector2(cos(theta) * gradMag, sin(theta) * gradMag);
        printf("%f %f\n", GradientTable[i].x, GradientTable[i].y);
    }
}

r32
NoiseGenerator::Perlin2D(const Vector2& unscaledCoords, r32 scale)
{
	const Vector2 coords  = unscaledCoords / scale;
	const Vector2 rCoords = floor(coords);
	const Vector2 UV      = coords - rCoords;
	const Vector2 antiUV  = UV - 1.0f;

	const IVector2 iCoords((i32)rCoords.x, (i32)rCoords.y);

    const u32 noiseLL = Get2D(iCoords.x, iCoords.y) & GradientMask;
    const u32 noiseLR = Get2D(iCoords.x + 1, iCoords.y) & GradientMask;
    const u32 noiseUR = Get2D(iCoords.x + 1, iCoords.y + 1) & GradientMask;
    const u32 noiseUL = Get2D(iCoords.x, iCoords.y + 1) & GradientMask;

#if 0 // HAS(DEV_BUILD)
    GradCounts[noiseLL]++;
    GradCounts[noiseLR]++;
    GradCounts[noiseUR]++;
    GradCounts[noiseUL]++;
#endif

	const Vector2 gradLL = GradientTable[noiseLL];
	const Vector2 gradLR = GradientTable[noiseLR];
	const Vector2 gradUR = GradientTable[noiseUR];
	const Vector2 gradUL = GradientTable[noiseUL];

	const r32 dotLL = dot(gradLL, UV);
	const r32 dotLR = dot(gradLR, Vector2(antiUV.x, UV.y));
	const r32 dotUR = dot(gradUR, antiUV);
	const r32 dotUL = dot(gradUL, Vector2(UV.x, antiUV.y));

	const r32 xweight = UV.x * UV.x * (3.0f - 2.0f * UV.x);
	const r32 yweight = UV.y * UV.y * (3.0f - 2.0f * UV.y);

	const r32 xValL = dotLL + xweight * (dotLR - dotLL);
	const r32 xValU = dotUL + xweight * (dotUR - dotUL);
	return xValL + yweight * (xValU - xValL);
}

void NoiseGenerator::DumpAndClearStats()
{
    printf("Grad stats:\n");
    for (u32 i = 0; i < NumGradients; i++)
    {
        printf("%ld\n", GradCounts[i]);
        GradCounts[i] = 0;
    }

    printf("Raw test:\n");
    static RandomGenerator test(123432);

    for (i32 i = 0; i < 100000000; i++)
    {
		GradCounts[test.Get2D((i32)test.Random(), (i32)test.Random()) & GradientMask]++;
	}

    for (u32 i = 0; i < NumGradients; i++)
    {
        printf("%ld\n", GradCounts[i]);
        GradCounts[i] = 0;
    }
}
