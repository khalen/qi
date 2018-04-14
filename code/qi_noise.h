#ifndef __QI_NOISE_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Noise and randomness
//

#include "basictypes.h"

constexpr inline static u32
RawNoise(const i32 iCoord, const u32 seed)
{
	constexpr u32 mix1 = 0xb5297a4d;
	constexpr u32 mix2 = 0x68e31da4;
	constexpr u32 mix3 = 0x1b56c4e9;

    u32 coord = (u32) iCoord;
	coord *= mix1;
	coord += seed;
	coord ^= coord >> 8;
	coord += mix2;
	coord ^= coord << 8;
	coord *= mix3;
	coord ^= coord >> 8;

	return coord;
}

constexpr inline static i64
RawNoise64()

class NoiseGenerator
{
	u32 Seed;

  public:
	NoiseGenerator(const u32 ISeed = 0)
	    : Seed(ISeed)
	{
	}

	inline u32 Get(i32 coord) { return RawNoise(coord, Seed); }
	inline u32 GetReal(i32 coord) { return GetNoise(coord) / (r32)UINT_MAX; }
	inline r32 GetReal(i32 coord, r32 minVal, r32 maxVal) { return GetReal() * (maxVal - minVal) + minVal; }
	inline r32 GetRealFullRange(i32 coord) { return GetReal(coord, -1.0f, 1.0f); }
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

	inline u32 Random() { return Get((i32)RandIdx++, Seed); }
	inline u32 Random(i32 minVal, i32 maxVal) { return (Random() % (maxVal - minVal)) + minVal; }

	inline r32 RandomReal() { return Random() / (r32) UINT_MAX; }
	inline r32 RandomReal(r32 minVal, r32 maxVal) { return RandomReal() * (maxVal - minVal) + minVal; }

	inline u32  GetCurrentRandomIndex() { return RandIdx; }
	inline void RandomReset(const u32 newRandIdx = 0) { RandIdx = newRandIdx; }
};

#define __QI_NOISE_H
#endif // #ifndef __QI_NOISE_H
