#ifndef __QI_TILE_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Basic tile and chunk definitions
//

#include "qi.h"

#define TILE_SIZE_METERS_X 1.4f
#define TILE_SIZE_METERS_Y 1.4f

#define SCREEN_TILE_WIDTH 16
#define SCREEN_TILE_HEIGHT 9

inline constexpr const u32
SmallestBitcountForSize(const u32 sz)
{
	u32 rval = 1;
	while ((1 << rval) < sz)
		rval++;
	return rval;
}

#define TILE_CHUNK_DIM 64
#define TILE_CHUNK_BITS SmallestBitcountForSize(TILE_CHUNK_DIM)
#define TILE_CHUNK_MASK ((1 << TILE_CHUNK_BITS) - 1)

#define TILE_FRAC_DIM 16384
#define TILE_FRAC_BITS SmallestBitcountForSize(TILE_FRAC_DIM)
#define TILE_FRAC_MULT (r32)(1 << TILE_FRAC_BITS)
#define TILE_FRAC_MASK ((1 << TILE_FRAC_BITS) - 1)

// Global invalid value for tile (ie. lookup off the edge of the world)
#define TILE_INVALID 0
#define TILE_EMPTY 1
#define TILE_FULL 2

// TODO: Implement real sparse lookup so we don't need this explicit limit
#define WORLD_CHUNKS_DIM 1024

struct WorldCoord_s
{
	i32 tile;
    r32 offset;
};

// Player / tile definitions
struct WorldPos_s
{
	union {
		struct
		{
			WorldCoord_s x;
			WorldCoord_s y;
			WorldCoord_s z;
		};
		WorldCoord_s coords[3];
	};
};

struct TileChunk_s
{
	u32* tiles;
};

struct World_s
{
	TileChunk_s chunks[WORLD_CHUNKS_DIM * WORLD_CHUNKS_DIM];
};

void SetTileValue(MemoryArena_s* tileArena, World_s* world, const WorldPos_s* pos, const u32 value);
u32  GetTileValue(World_s* world, const WorldPos_s* pos);
void AddSubtileOffset(WorldPos_s* pos, const Vec2_s offset);

#define __QI_TILE_H
#endif // #ifndef __QI_TILE_H
