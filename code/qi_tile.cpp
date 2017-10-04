//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Basic tile and world functionality
//

#include "basictypes.h"

#include "qi.h"
#include "qi_tile.h"
#include <stdio.h>

TileChunk_s*
GetChunk(World_s* world, const WorldPos_s* pos)
{
	const i32 chunkX  = pos->x.tile >> TILE_CHUNK_BITS;
	const i32 chunkY  = pos->y.tile >> TILE_CHUNK_BITS;
	const u32 absIdxX = chunkX + WORLD_CHUNKS_DIM / 2;
	const u32 absIdxY = chunkY + WORLD_CHUNKS_DIM / 2;

	if (absIdxX >= WORLD_CHUNKS_DIM || absIdxY > WORLD_CHUNKS_DIM)
		return nullptr;

	return &world->chunks[absIdxX + WORLD_CHUNKS_DIM * absIdxY];
}

void
SetTileValue(MemoryArena_s* tileArena, World_s* world, const WorldPos_s* pos, const u32 value)
{
	TileChunk_s* chunk = GetChunk(world, pos);
	Assert(chunk);

	if (chunk->tiles == nullptr)
		chunk->tiles = (u32*)MemoryArena_Alloc(tileArena, TILE_CHUNK_DIM * TILE_CHUNK_DIM * sizeof(u32));

	Assert(chunk->tiles);

	const u32 tileX = pos->x.tile & TILE_CHUNK_MASK;
	const u32 tileY = pos->y.tile & TILE_CHUNK_MASK;
	Assert(tileX < TILE_CHUNK_DIM && tileY < TILE_CHUNK_DIM);
	chunk->tiles[tileX + tileY * TILE_CHUNK_DIM] = value;
}

u32
GetTileValue(World_s* world, const WorldPos_s* pos)
{
	TileChunk_s* chunk = GetChunk(world, pos);

	if (chunk->tiles == nullptr)
		return TILE_INVALID;

	const u32 tileX = pos->x.tile & TILE_CHUNK_MASK;
	const u32 tileY = pos->y.tile & TILE_CHUNK_MASK;
	Assert(tileX < TILE_CHUNK_DIM && tileY < TILE_CHUNK_DIM);
    return chunk->tiles[tileX + tileY * TILE_CHUNK_DIM];
}

void NormalizePos(WorldPos_s* pos)
{
    i32 xTileAdd = (i32)pos->x.offset;
	pos->x.tile += xTileAdd;
    pos->x.offset -= (r32)xTileAdd;

    i32 yTileAdd = (i32)pos->y.offset;
	pos->y.tile += yTileAdd;
    pos->y.offset -= (r32)yTileAdd;
}

void
AddSubtileOffset(WorldPos_s* pos, const Vec2_s offset)
{
    pos->x.offset += offset.x;
    pos->y.offset += offset.y;
    NormalizePos(pos);
}
