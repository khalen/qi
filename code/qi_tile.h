#ifndef __QI_TILE_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Basic tile and chunk definitions
//

#include "qi.h"
#define TILE_SIZE_METERS_X   1.4f
#define TILE_SIZE_METERS_Y   1.4f

#define SCREEN_TILE_WIDTH 16
#define SCREEN_TILE_HEIGHT 9
#define TILE_SIZE_DIM (GAME_RES_X / SCREEN_TILE_WIDTH)
#define TILE_SIZE_BITS 6
static_assert((1 << TILE_SIZE_BITS) >= TILE_SIZE_DIM, "Not enough bits for TILE_SIZE_BITS");

#define TILE_CHUNK_DIM 64
#define TILE_CHUNK_BITS 6
static_assert((1 << TILE_CHUNK_BITS) >= TILE_CHUNK_DIM, "Not enough bits for TILE_CHUNK_BITS");

#define TILE_FRAC_BITS 8
#define TILE_COORD_BITS (TILE_SIZE_BITS + TILE_FRAC_BITS)

// TODO: Implement real sparse lookup so we don't need this explicit limit
#define WORLD_CHUNKS_DIM 1024

struct WorldCoord_s
{
    union
    {
        struct
        {
            u64 subTile : TILE_FRAC_BITS;
            u64 tile : TILE_CHUNK_BITS;
            u64 chunk;
        };
        u64 value;
    };
};

// Player / tile definitions
struct WorldPos_s
{
    union
    {
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

void SetTileValue(World_s* world, const WorldPos_s* pos);
u32 GetTileValue(const World_s* world, const WorldPos_s* pos);

#define __QI_TILE_H
#endif // #ifndef __QI_TILE_H
