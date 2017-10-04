//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Implementation file for the base QI API
//

#include "basictypes.h"

#include "qi.h"
#include "qi_tile.h"
#include "qi_math.h"
#include <stdio.h>

#define TILEMAP_WID (17 * 1)
#define TILEMAP_HGT (9 * 1)

#define SCREEN_WORLD_SIZE_X (TILEMAP_WID * TILE_SIZE_METERS_X)
#define SCREEN_WORLD_SIZE_Y (TILEMAP_HGT * TILE_SIZE_METERS_Y)

struct GameGlobals_s
{
	bool        isInitialized;
	Memory_s*   memory;
	SubSystem_s gameSubsystems[MAX_SUBSYSTEMS];

	i32 screenWid;
	i32 screenHgt;

	World_s       world;
	WorldPos_s    playerPos;
	MemoryArena_s tileArena;
};

GameGlobals_s* g_game;

void*
Qim_AllocRaw(Memory_s* memory, const size_t size)
{
	size_t allocSize = (size + 15) & ~0xFull;
	Assert(memory && ((uintptr_t)memory->permanentPos & 0xF) == 0
	       && (memory->permanentSize - (size_t)(memory->permanentPos - memory->permanentStorage) > allocSize));
	void* result = memory->permanentPos;
	memory->permanentPos += allocSize;
	return result;
}

template<typename T>
T*
GameAllocate()
{
	return Qim_New<T>(g_game->memory);
}

void
MemoryArena_Init(MemoryArena_s* arena, const size_t size)
{
	arena->size      = size;
	arena->curOffset = 0;
	arena->base      = (u8*)Qim_AllocRaw(g_game->memory, size);
}

u8*
MemoryArena_Alloc(MemoryArena_s* arena, const size_t size)
{
	Assert(arena && arena->curOffset + size <= arena->size);
	u8* mem = arena->base + arena->curOffset;
	arena->curOffset += size;
	return mem;
}

internal void
DrawGradient(Bitmap_s* osb, int xOff, int yOff)
{
	u32* xel = osb->pixels;
	for (u32 y = 0; y < osb->height; y++)
	{
		for (u32 x = 0; x < osb->width; x++)
		{
			u32 col = (((x + xOff) & 0xFF)) << 16 | (((y + yOff) & 0xFF)) << 8 | 0xc0;
			*xel++  = col;
		}
	}
}

static void InitGameGlobals(const SubSystem_s*, bool);

SubSystem_s GameSubSystem = {"Game", InitGameGlobals, sizeof(GameGlobals_s), nullptr};

internal void
InitGameGlobals(const SubSystem_s* sys, bool isReInit)
{
	Assert(sys->globalPtr);

	if (isReInit)
		return;

    MemoryArena_Init(&g_game->tileArena, MB(32));

	g_game->playerPos.x.tile = 10;
	g_game->playerPos.y.tile = 10;

	for (i32 scrY = -32; scrY < 32; scrY++)
	{
		for (i32 scrX = -32; scrX < 32; scrX++)
		{
			for (i32 tileY = 0; tileY < TILEMAP_HGT; tileY++)
			{
				for (i32 tileX = 0; tileX < TILEMAP_WID; tileX++)
				{
					u32 value = TILE_EMPTY;
					if ((tileX == 0 || tileX == (TILEMAP_WID - 1) || tileY == 0 || tileY == (TILEMAP_HGT - 1))
					    && ((tileX != TILEMAP_WID / 2) && (tileY != TILEMAP_HGT / 2)))
						value = TILE_FULL;
					WorldPos_s worldPos = {};
					worldPos.x.tile     = scrX * TILEMAP_WID + tileX;
					worldPos.y.tile     = scrY * TILEMAP_HGT + tileY;
					SetTileValue(&g_game->tileArena, &g_game->world, &worldPos, value);
				}
			}
		}
	}
}

internal void
UpdateGameState(Input_s* input)
{
	Assert(g_game);

	// Controller 0 left stick
	Analog_s* ctr0 = &input->controllers[KBD].analogs[0];
	const r32 playerSpeed = 5.0f * input->dT; // Meters / sec

	if (ctr0->trigger.reading > 0.0f)
	{
		Vec2_s dPlayerPos = {};
		Vec2MulScalar(&dPlayerPos, &ctr0->dir, ctr0->trigger.reading * playerSpeed);
		AddSubtileOffset(&g_game->playerPos, dPlayerPos);
	}
}

extern SubSystem_s SoundSubSystem;
extern SubSystem_s DebugSubSystem;

internal SubSystem_s* s_subSystems[] = {
    &GameSubSystem,
    &SoundSubSystem,
    &DebugSubSystem,
};

internal void
InitGameSystems(Memory_s* memory)
{
	g_game         = (GameGlobals_s*)memory->permanentStorage;
	g_game->memory = memory;

	bool isReload = g_game->isInitialized;

	for (i32 i = 0; i < countof(s_subSystems); i++)
	{
		SubSystem_s* sys       = &g_game->gameSubsystems[i];
		void*        curMemPtr = sys->globalPtr;
		*sys                   = *s_subSystems[i];

		if (curMemPtr == nullptr)
			sys->globalPtr = Qim_AllocRaw(memory, sys->globalSize);
		else
			sys->globalPtr = curMemPtr;

		sys->initFunc(sys, isReload);
	}

	if (!g_game->isInitialized)
	{
		g_game->isInitialized = true;
	}
}

static u32
RoundReal(r32 val)
{
	return (u32)(val + 0.5f);
}

static i32
RoundIReal(r32 val)
{
	return (i32)(val + 0.5f);
}

static u32
PackColor(r32 r, r32 g, r32 b)
{
	return (RoundReal(r * 255.0f) << 16) | (RoundReal(g * 255.0f) << 8) | (RoundReal(b * 255.0f) << 0);
}

internal void
RenderRectangle(Bitmap_s* bitmap, i32 x0, i32 y0, i32 x1, i32 y1, r32 r, r32 g, r32 b)
{
	const u32 color = PackColor(r, g, b);

	x0 = Qi_Clamp<i32>(x0, 0, bitmap->width);
	x1 = Qi_Clamp<i32>(x1, x0, bitmap->width);
	y0 = Qi_Clamp<i32>(y0, 0, bitmap->height);
	y1 = Qi_Clamp<i32>(y1, y0, bitmap->height);

	y0       = bitmap->height - y0;
	y1       = bitmap->height - y1;
	u32* xel = bitmap->pixels + y1 * bitmap->pitch;
	for (i32 y = y1; y < y0; y++)
	{
		for (i32 x = x0; x < x1; x++)
			xel[x] = color;
		xel += bitmap->pitch;
	}
}

internal void
DrawRectangle(Bitmap_s* bitmap, r32 x, r32 y, r32 width, r32 height, r32 r, r32 g, r32 b)
{
	RenderRectangle(bitmap, RoundIReal(x), RoundIReal(y), RoundIReal(x + width), RoundIReal(y + height), r, g, b);
}

void
Qi_GameUpdateAndRender(ThreadContext_s*, Input_s* input, Bitmap_s* screenBitmap)
{
	g_game->screenWid = screenBitmap->width;
	g_game->screenHgt = screenBitmap->height;

	Assert(g_game && g_game->isInitialized);
	UpdateGameState(input);

	// Clear screen
	DrawRectangle(screenBitmap, 0.0f, 0.0f, (r32)screenBitmap->width, (r32)screenBitmap->height, 0.0f, 0.0f, 1.0f);

	const r32 tilePixelWid = screenBitmap->width / (float)TILEMAP_WID;
	const r32 tilePixelHgt = screenBitmap->height / (float)TILEMAP_HGT;

	const r32 playerOffsetPixelsX = g_game->playerPos.x.offset * tilePixelWid;
	const r32 playerOffsetPixelsY = g_game->playerPos.y.offset * tilePixelHgt;

	for (i32 row = -1; row < TILEMAP_HGT + 1; row++)
	{
		for (i32 col = -1; col < TILEMAP_WID + 1; col++)
		{
            const r32 sx = col * tilePixelWid - playerOffsetPixelsX;
			const r32 sy = row * tilePixelHgt - playerOffsetPixelsY;

            WorldPos_s tileCoord = {};
            tileCoord.x.tile = col + (g_game->playerPos.x.tile - TILEMAP_WID / 2);
            tileCoord.y.tile = row + (g_game->playerPos.y.tile - TILEMAP_HGT / 2);

            u32 tileValue = GetTileValue(&g_game->world, &tileCoord);
            if (tileValue == TILE_INVALID)
				DrawRectangle(screenBitmap, sx, sy, tilePixelWid, tilePixelHgt, 1.0f, 0.2f, 0.2f);
			else if (tileValue == TILE_EMPTY)
				DrawRectangle(screenBitmap, sx, sy, tilePixelWid, tilePixelHgt, 0.3f, 0.3f, 0.3f);
			else
				DrawRectangle(screenBitmap, sx, sy, tilePixelWid, tilePixelHgt, 0.9f, 0.7f, 0.9f);
		}
	}

	r32 playerR   = 0.5f;
	r32 playerG   = 1.0f;
	r32 playerB   = 0.0f;
	r32 playerWid = tilePixelWid * 0.75f;
	r32 playerHgt = tilePixelHgt;
	r32 playerX   = (screenBitmap->width / 2.0f) - playerWid / 2.0f;
	r32 playerY   = screenBitmap->height / 2.0f;
	DrawRectangle(screenBitmap, playerX, playerY, playerWid, playerHgt, playerR, playerG, playerB);
}

const PlatFuncs_s* plat;
void
Qi_Init(const PlatFuncs_s* platFuncs, Memory_s* memory)
{
	plat = platFuncs;

	if (g_game == nullptr)
	{
		InitGameSystems(memory);
		Assert(g_game && g_game->isInitialized);
	}
}

internal GameFuncs_s s_game = {
    sound,
    debug,
    Qi_Init,
    Qi_GameUpdateAndRender,
};
const GameFuncs_s* game = &s_game;

// Interface to platform code
extern "C" {
PLAT_EXPORT const GameFuncs_s*
                  Qi_GetGameFuncs()
{
	return game;
}
}
