#ifndef __QI_NOISE_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Noise and randomness
//

#include "basictypes.h"
#include "qi_vector.h"
#include "qi_math.h"

#include <limits>
#include <cmath>

constexpr inline static u32
RawNoise(const i32 iCoord, const u32 seed)
{
    constexpr u32 mix1 = 0xb5297a4d;
    constexpr u32 mix2 = 0x68e31da4;
    constexpr u32 mix3 = 0x1b56c4e9;

    u32 coord = (u32)iCoord;
    coord *= mix1;
    coord += seed;
    coord ^= coord >> 8;
    coord += mix2;
    coord ^= coord << 8;
    coord *= mix3;
    coord ^= coord >> 8;

    return coord;
}

constexpr inline static u32
RawNoise2D(const i32 xCoord, const i32 yCoord, const u32 seed)
{
    constexpr u32 largePrime = 198491317;
    return RawNoise(xCoord + yCoord * largePrime, seed);
}

class NoiseGenerator
{
    u32 Seed;

  public:
    NoiseGenerator(const u32 ISeed = 0)
        : Seed(ISeed)
    {
    }

    constexpr static size_t NumGradients = 0x20;
    constexpr static size_t GradientMask = 0x1F;
    static Vector2 GradientTable[NumGradients];

    static void InitGradients();

    inline u32 GetSeed() { return Seed; }
    inline void SetSeed(const u32 NewSeed) { Seed = NewSeed; }

    inline u32 Get(i32 coord) { return RawNoise(coord, Seed); }
    inline u32 Get2D(i32 xCoord, i32 yCoord) { return RawNoise2D(xCoord, yCoord, Seed); }
    inline r32 GetReal(i32 coord)
    {
        constexpr static r32 invMaxU32 = 1.0f / (float)std::numeric_limits<u32>::max();
        return Get(coord) * invMaxU32;
    }
    inline r32 GetRealRadians(i32 coord)
    {
        constexpr static r32 invMax2PI = (Q_PI * 2.0f) / (float)std::numeric_limits<u32>::max();
        return Get(coord) * invMax2PI;
    }
    inline r32 GetReal2D(i32 xCoord, i32 yCoord)
    {
        constexpr static r32 invMaxU32 = 1.0f / (float)std::numeric_limits<u32>::max();
        return Get2D(xCoord, yCoord) * invMaxU32;
    }
    inline r32 GetReal(i32 coord, r32 minVal, r32 maxVal) { return GetReal(coord) * (maxVal - minVal) + minVal; }
    inline r32 GetReal2D(i32 xCoord, i32 yCoord, r32 minVal, r32 maxVal)
    {
        return GetReal2D(xCoord, yCoord) * (maxVal - minVal) + minVal;
    }
    inline r32 GetRealRadians2D(i32 xCoord, i32 yCoord)
    {
        constexpr static r32 invMax2PI = (Q_PI * 2.0f) / (float)std::numeric_limits<u32>::max();
        return Get2D(xCoord, yCoord) * invMax2PI;
    }
    inline r32 GetRealFullRange(i32 coord) { return GetReal(coord, -1.0f, 1.0f); }
    inline r32 GetRealFullRange2D(i32 xCoord, i32 yCoord) { return GetReal2D(xCoord, yCoord, -1.0f, 1.0f); }

    inline Vector2 GetUnitDirection(i32 coord)
    {
        const r32 theta = GetRealRadians(coord);
        return Vector2(cos(theta), sin(theta));
    }
    inline Vector2 GetUnitDirection2D(i32 xCoord, i32 yCoord)
    {
        const r32 theta = GetRealRadians2D(xCoord, yCoord);
        return Vector2(cos(theta), sin(theta));
    }

    r32 Smoothed(r32 xCoord, r32 scale);
    r32 Smoothed2D(r32 xCoord, r32 yCoord, r32 scale);
    r32 Smoothed2D(const Vector2& coords, r32 scale);

    r32 Perlin(r32 xCoord, r32 scale);
    r32 Perlin2D(r32 xCoord, r32 yCoord, r32 scale);
    r32 Perlin2D(const Vector2& coords, r32 scale);

    r32 Simplex(r32 xCoord, r32 scale);
    r32 Simplex2D(r32 xCoord, r32 yCoord, r32 scale);
    r32 Simplex2D(const Vector2& coords, r32 scale);

    static void DumpAndClearStats();
};

class RandomGenerator : public NoiseGenerator
{
    u32 RandIdx;

  public:
    RandomGenerator(const u32 ISeed = 0)
        : NoiseGenerator(ISeed)
        , RandIdx(0)
    {
    }

    inline u32 Random() { return Get(RandIdx++); }
    inline u32 Random(i32 minValInclusive, i32 maxValInclusive)
    {
        return (Random() % (maxValInclusive + 1 - minValInclusive)) + minValInclusive;
    }
    inline u32 RandomBelow(i32 minValExclusive) { return Random() % minValExclusive; }

    inline r32  RandomReal() { return Random() / (r32)(std::numeric_limits<u32>::max()); }
    inline r32  RandomReal(r32 minVal, r32 maxVal) { return RandomReal() * (maxVal - minVal) + minVal; }
    inline bool RandomProbability(r32 probability) { return RandomReal() <= probability; }

    inline u32  GetCurrentRandomIndex() { return RandIdx; }
    inline void RandomReset(const u32 newRandIdx = 0) { RandIdx = newRandIdx; }
};

#define __QI_NOISE_H
#endif // #ifndef __QI_NOISE_H
