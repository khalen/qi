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

	Bitmap_s testBitmaps[5];
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

template<typename T>
T*
MemoryArena_PushStruct(MemoryArena_s* arena)
{
	T* newT = (T*)MemoryArena_Alloc(arena, sizeof(T));
	return newT;
}

internal void ReadBitmap(ThreadContext_s* thread, MemoryArena_s* memArena, Bitmap_s* result, const char* filename);

void
CreateBitmap(MemoryArena_s* arena, Bitmap_s* result, const u32 width, const u32 height)
{
	Assert(arena && result);
	result->width    = width;
	result->height   = height;
	result->pitch    = width;
	result->byteSize = width * height * sizeof(u32);
	result->pixels   = (u32*)MemoryArena_Alloc(arena, result->byteSize);
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

	ReadBitmap(nullptr, &g_game->tileArena, &g_game->testBitmaps[0], "test/test_background.bmp");
	ReadBitmap(nullptr, &g_game->tileArena, &g_game->testBitmaps[1], "test/test_scene_layer_00.bmp");
	ReadBitmap(nullptr, &g_game->tileArena, &g_game->testBitmaps[2], "test/test_scene_layer_01.bmp");
	ReadBitmap(nullptr, &g_game->tileArena, &g_game->testBitmaps[3], "test/test_scene_layer_02.bmp");
	ReadBitmap(nullptr, &g_game->tileArena, &g_game->testBitmaps[4], "test/test_scene_layer_03.bmp");
}

internal void
UpdateGameState(Input_s* input)
{
	Assert(g_game);

	// Controller 0 left stick
	Analog_s* ctr0        = &input->controllers[KBD].analogs[0];
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

	u32* xel = bitmap->pixels + y0 * bitmap->pitch;
	for (i32 y = y0; y < y1; y++)
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

BEGIN_PACKED_DEFS

struct BmpFileHeader_s
{
	u16 type;
	u32 fileSize;
	u16 __reserved1;
	u16 __reserved2;
	u32 pixelDataOffset;
} PACKED;

struct BmpImageHeader_s
{
	u32 size;
	u32 width;
	u32 height;
	u16 planes;
	u16 bpp;
	u32 compression;
	u32 imageSize;
	u32 hResolution;
	u32 vResolution;
	u32 colors;
	u32 importantColors;
} PACKED;

END_PACKED_DEFS

internal void
ClipRectToBmp(const Bitmap_s* bmp, i32* bx, i32* by, i32* bw, i32* bh)
{
	i32 bx1 = *bx + *bw;
	i32 by1 = *by + *bh;

	if (*bx < 0)
		*bx = 0;

	if (*by < 0)
		*by = 0;

	if (bx1 < 0)
		bx1 = 0;

	if (bx1 > (i32)bmp->width)
		bx1 = (i32)bmp->width;

	if (by1 < 0)
		by1 = 0;

	if (by1 > (i32)bmp->height)
		by1 = (i32)bmp->height;

	*bw = bx1 - *bx;
	*bh = by1 - *by;
}

internal void
BltBmpFixed(ThreadContext_s* thread, Bitmap_s* dest, i32 dx, i32 dy, const Bitmap_s* src)
{
	i32 sx = 0, sy = 0;
	i32 sw = src->width, sh = src->height;

	// Clip to dest bmp
	if (dx < 0)
	{
		sx -= dx;
		sw += dx;
		dx = 0;
	}
	if (dy < 0)
	{
		sy -= dy;
		sy += dy;
		dy = 0;
	}
	if (dx + sw >= dest->width)
	{
		sw -= (dx + sw - dest->width);
	}
	if (dy + sh >= dest->height)
	{
		sh -= (dy + sh - dest->height);
	}
	Assert(sx >= 0 && sy >= 0 && sx + sw < dest->width && sy + sh < dest->height);

	for (i32 y = 0; y < sh; y++)
	{
		u32* const srcXelRow  = src->pixels + (sy + y) * src->pitch;
		u32* const destXelRow = dest->pixels + (dy + y) * dest->pitch;
		for (i32 x = 0; x < sw; x++)
			destXelRow[x + dx] = srcXelRow[x + sx];
	}
}

internal void
BltBmp(ThreadContext_s* thread,
       Bitmap_s*        dest,
       r32              rdx,
       r32              rdy,
       r32              rdw,
       r32              rdh,
       Bitmap_s*        src,
       r32              rsx,
       r32              rsy,
       r32              rsw,
       r32              rsh)
{
	r32 rdx1 = ceil(rdx + rdw);
	r32 rdy1 = ceil(rdy + rdh);
	rdx      = floor(rdx);
	rdy      = floor(rdy);

	r32 rsx1 = ceil(rsx + rsw);
	r32 rsy1 = ceil(rsy + rsh);
	rsx      = floor(rsx);
	rsy      = floor(rsy);

    i32 dx = (i32)rdx;
    i32 dy = (i32)rdy;
    i32 dw = (i32)(rdx1 - rdx);
    i32 dh = (i32)(rdy1 - rdy);

    i32 sx = (i32)rsx;
    i32 sy = (i32)rsy;
    i32 sw = (i32)(rsx1 - rsx);
    i32 sh = (i32)(rsy1 - rsy);

	// Adjust for starting offset for clipped initial rects
	i32 sdx = (sw << 16) / dw;
	i32 sdy = (sh << 16) / dh;

	// Clip src rect to src bitmap
	ClipRectToBmp(src, &sx, &sy, &sw, &sh);

	i32 idx = dx, idy = dy;
	ClipRectToBmp(dest, &dx, &dy, &dw, &dh);

	if (dw == 0 || dh == 0 || sw == 0 || sh == 0)
		return;

	Assert(sx >= 0 && sy >= 0 && sw < src->width && sh < src->height);
	Assert(dx >= 0 && dy >= 0 && dw < dest->width && dh < dest->height);

	i32 curSy = sy << 16;
	curSy += sdy * (dy - idy);

	for (i32 y = 0; y < dh; y++, curSy += sdy)
	{
		i32 curSx = sx << 16;
		curSx += sdx * (dx - idx);

		u32* srcXelRow  = src->pixels + (curSy >> 16) * src->pitch;
		u32* destXelRow = dest->pixels + (dy + y) * dest->pitch;
		for (i32 x = 0; x < dw; x++, curSx += sdx)
			destXelRow[x + dx] = srcXelRow[curSx >> 16];
	}
}

internal void
ReadBitmap(ThreadContext_s* thread, MemoryArena_s* memArena, Bitmap_s* result, const char* filename)
{
	size_t readSize;
	u8*    fileData = (u8*)plat->ReadEntireFile(thread, filename, &readSize);
	Assert(fileData);

	const BmpFileHeader_s*  fileHeader = (BmpFileHeader_s*)fileData;
	const BmpImageHeader_s* imgHeader  = (BmpImageHeader_s*)(fileData + sizeof(BmpFileHeader_s));
	u8*                     srcXels    = nullptr;
	u8*                     dstXels    = nullptr;

	if (fileHeader->type != 0x4D42) // 'BM' in ASCII
	{
		printf("Bad bmp type: 0x%x\n", fileHeader->type);
		goto end;
	}
	if (imgHeader->size < 40)
	{
		printf("Unexpected bmp header size: %d\n", imgHeader->size);
		goto end;
	}

	CreateBitmap(memArena, result, imgHeader->width, imgHeader->height);
	srcXels = fileData + fileHeader->pixelDataOffset;
	dstXels = (u8*)result->pixels;

	for (u32 idx = 0; idx < result->byteSize; idx += sizeof(u32))
	{
		dstXels[idx + 0] = srcXels[idx + 3];
		dstXels[idx + 1] = srcXels[idx + 2];
		dstXels[idx + 2] = srcXels[idx + 1];
		dstXels[idx + 3] = srcXels[idx + 0];
	}

	printf("Read %s: %d x %d\n", filename, result->width, result->height);

end:
	plat->ReleaseFileBuffer(thread, fileData);
}

void
Qi_GameUpdateAndRender(ThreadContext_s*, Input_s* input, Bitmap_s* screenBitmap)
{
	g_game->screenWid = screenBitmap->width;
	g_game->screenHgt = screenBitmap->height;

	Assert(g_game && g_game->isInitialized);
	UpdateGameState(input);

	// Clear screen
	// DrawRectangle(screenBitmap, 0.0f, 0.0f, (r32)screenBitmap->width, (r32)screenBitmap->height, 0.0f, 0.0f, 1.0f);

	const r32 tilePixelWid = screenBitmap->width / (float)TILEMAP_WID;
	const r32 tilePixelHgt = screenBitmap->height / (float)TILEMAP_HGT;

	const r32 playerOffsetPixelsX = g_game->playerPos.x.offset * tilePixelWid;
	const r32 playerOffsetPixelsY = g_game->playerPos.y.offset * tilePixelHgt;

	BltBmp(nullptr,
	       screenBitmap,
	       0,
	       0,
	       screenBitmap->width,
	       screenBitmap->height,
	       &g_game->testBitmaps[0],
	       20,
	       20,
	       300,
	       300);

	for (i32 row = -1; row < TILEMAP_HGT + 1; row++)
	{
		for (i32 col = -1; col < TILEMAP_WID + 1; col++)
		{
			const r32 sx = col * tilePixelWid - playerOffsetPixelsX;
			const r32 sy = row * tilePixelHgt - playerOffsetPixelsY;

			WorldPos_s tileCoord = {};
			tileCoord.x.tile     = col + (g_game->playerPos.x.tile - TILEMAP_WID / 2);
			tileCoord.y.tile     = row + (g_game->playerPos.y.tile - TILEMAP_HGT / 2);

			u32 tileValue = GetTileValue(&g_game->world, &tileCoord);
#if 1
			if (tileValue == TILE_INVALID)
				DrawRectangle(screenBitmap, sx, sy, tilePixelWid, tilePixelHgt, 1.0f, 0.2f, 0.2f);
			else if (tileValue == TILE_EMPTY)
				DrawRectangle(screenBitmap, sx, sy, tilePixelWid, tilePixelHgt, 0.3f, 0.3f, 0.3f);
			else
				DrawRectangle(screenBitmap, sx, sy, tilePixelWid, tilePixelHgt, .8f, .8f, .8f);
#else
			if (tileValue == TILE_INVALID)
				DrawRectangle(screenBitmap, sx, sy, tilePixelWid, tilePixelHgt, 1.0f, 0.2f, 0.2f);
			else if (tileValue != TILE_EMPTY)
				BltBmp(nullptr,
				       screenBitmap,
				       sx,
				       sy,
				       tilePixelWid,
				       tilePixelHgt,
				       &g_game->testBitmaps[2],
				       0,
				       0,
				       g_game->testBitmaps[2].width,
				       g_game->testBitmaps[2].height);
#endif
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
