//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Basic tile and world functionality
//

#include "basictypes.h"

#include "qi.h"
#include "qi_memory.h"
#include "qi_tile.h"
#include <stdio.h>

TileChunk_s*
GetChunk(World_s* world, const i32 tileX, const i32 tileY)
{
	const i32 chunkX  = tileX >> TILE_CHUNK_BITS;
	const i32 chunkY  = tileY >> TILE_CHUNK_BITS;
	const u32 absIdxX = chunkX + WORLD_CHUNKS_DIM / 2;
	const u32 absIdxY = chunkY + WORLD_CHUNKS_DIM / 2;

	if (absIdxX >= WORLD_CHUNKS_DIM || absIdxY > WORLD_CHUNKS_DIM)
		return nullptr;

	return &world->chunks[absIdxX + WORLD_CHUNKS_DIM * absIdxY];
}

TileChunk_s*
GetChunk(World_s* world, const WorldPos_s* pos)
{
	return GetChunk(world, pos->x.tile, pos->y.tile);
}

void
SetTileValue(MemoryArena* tileArena, World_s* world, const WorldPos_s* pos, const u32 value)
{
	TileChunk_s* chunk = GetChunk(world, pos);
	Assert(chunk);

	if (chunk->tiles == nullptr)
		chunk->tiles = (u32*)MA_Alloc(tileArena, TILE_CHUNK_DIM * TILE_CHUNK_DIM * sizeof(u32));

	Assert(chunk->tiles);

	const u32 tileX = pos->x.tile & TILE_CHUNK_MASK;
	const u32 tileY = pos->y.tile & TILE_CHUNK_MASK;
	Assert(tileX < TILE_CHUNK_DIM && tileY < TILE_CHUNK_DIM);
	chunk->tiles[tileX + tileY * TILE_CHUNK_DIM] = value;
}

u32
GetTileValue(World_s* world, const i32 iTileX, const i32 iTileY)
{
	TileChunk_s* chunk = GetChunk(world, iTileX, iTileY);

	if (chunk->tiles == nullptr)
		return TILE_INVALID;

	const u32 tileX = iTileX & TILE_CHUNK_MASK;
	const u32 tileY = iTileY & TILE_CHUNK_MASK;
	Assert(tileX < TILE_CHUNK_DIM && tileY < TILE_CHUNK_DIM);
	return chunk->tiles[tileX + tileY * TILE_CHUNK_DIM];
}

u32
GetTileValue(World_s* world, const WorldPos_s* pos)
{
	return GetTileValue(world, pos->x.tile, pos->y.tile);
}

void
NormalizePos(WorldPos_s* pos)
{
	i32 xTileAdd = (i32)roundf(pos->x.offset);
	pos->x.tile += xTileAdd;
	pos->x.offset -= (r32)xTileAdd;

	i32 yTileAdd = (i32)roundf(pos->y.offset);
	pos->y.tile += yTileAdd;
	pos->y.offset -= (r32)yTileAdd;
}

void
AddSubtileOffset(WorldPos_s* pos, const v2 offset)
{
	pos->x.offset += offset.x;
	pos->y.offset += offset.y;
	NormalizePos(pos);
}

void
WorldPosSub(WorldPos_s* dest, const WorldPos_s* a, const WorldPos_s* b)
{
	for (i32 c = 0; c < 3; c++)
	{
		dest->coords[c].tile   = a->coords[c].tile - b->coords[c].tile;
		dest->coords[c].offset = a->coords[c].offset - b->coords[c].offset;
	}
}

v2
WorldPosToMeters(WorldPos_s* pos)
{
	return V2((pos->x.tile + pos->x.offset) * TILE_SIZE_METERS_X, (pos->y.tile + pos->y.offset) * TILE_SIZE_METERS_Y);
}

v2
MetersToScreenPixels(const v2& posMeters)
{
	const r32 tilePixelWid	  = TILE_RES_X / SCREEN_SCALE;
	const r32 tilePixelHgt	  = TILE_RES_Y / SCREEN_SCALE;
	const r32 metersToXelsWid = tilePixelWid / TILE_SIZE_METERS_X;
	const r32 metersToXelsHgt = tilePixelHgt / TILE_SIZE_METERS_Y;

	return V2(posMeters.x * metersToXelsWid, posMeters.y * metersToXelsHgt);
}

v2
WorldPosToScreenPixels(WorldPos_s* pos)
{
	const v2 meters = WorldPosToMeters(pos);
	return MetersToScreenPixels(meters);
}
