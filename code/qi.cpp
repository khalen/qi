//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Implementation file for the base QI API
//

#include "basictypes.h"

#include "qi.h"
#include "qi_math.h"

#include "qi_debug.cpp"
#include "qi_math.cpp"
#include "qi_sound.cpp"

struct GameGlobals_s
{
	bool        isInitialized;
	Memory_s*   memory;
	SubSystem_s gameSubsystems[MAX_SUBSYSTEMS];
	Vec2_s      playerPos;
	i32         screenWid;
	i32         screenHgt;
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
InitGameGlobals(const SubSystem_s* sys, bool)
{
	Assert(sys->globalPtr);
}

internal void
UpdateGameState(Input_s* input)
{
	Assert(g_game);

	const r32 tileWid = g_game->screenWid / 16.0f;

	// Controller 0 left stick
	const r32 playerSpd = 2.0f * tileWid * input->dT;
	Analog_s* ctr0      = &input->controllers[KBD].analogs[0];
	if (ctr0->trigger.reading > 0.0f)
	{
		Vec2_s dPlayerPos = {};
		Vec2MulScalar(&dPlayerPos, &ctr0->dir, ctr0->trigger.reading * playerSpd);
		Vec2Add(&g_game->playerPos, &g_game->playerPos, &dPlayerPos);
	}
}

extern SubSystem_s SoundSubSystem;
extern SubSystem_s DebugSubSystem;

internal SubSystem_s* s_subSystems[] = {
    &GameSubSystem, &SoundSubSystem, &DebugSubSystem,
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
		g_game->playerPos.x = 20.0f;
		g_game->playerPos.y = 20.0f;
	}

	g_game->isInitialized = true;
}

internal u32
RoundReal(r32 val)
{
	return (u32)(val + 0.5f);
}

internal i32
RoundIReal(r32 val)
{
	return (i32)(val + 0.5f);
}

internal u32
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

	u32* xel = bitmap->pixels + y1 * bitmap->pitch;
	for (i32 y = y0; y < y1; y++)
	{
		xel -= bitmap->pitch;
		for (i32 x = x0; x < x1; x++)
			xel[x] = color;
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
	DrawRectangle(screenBitmap, 0.0f, 0.0f, (r32)screenBitmap->width, (r32)screenBitmap->height, 1.0f, 0.0f, 1.0f);

	u32 tiles[9][17] = {
	    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1},
	    {1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
	    {1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
	    {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	    {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1},
	    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	};

	const r32 tileWid = screenBitmap->width / 16.0f;
	const r32 tileHgt = screenBitmap->height / 9.0f;

	for (u32 row = 0; row < 9; row++)
	{
		for (u32 col = 0; col < 17; col++)
		{
			const r32 tx = col * tileWid - tileWid / 2.0f;
			const r32 ty = row * tileHgt;
			if (tiles[row][col] > 0)
				DrawRectangle(screenBitmap, tx, ty, tileWid, tileHgt, 1.0f, 1.0f, 1.0f);
			else
				DrawRectangle(screenBitmap, tx, ty, tileWid, tileHgt, 0.2f, 0.5f, 0.5f);
		}
	}

	r32 playerR   = 0.5f;
	r32 playerG   = 1.0f;
	r32 playerB   = 0.0f;
	r32 playerWid = tileWid * 0.75f;
	r32 playerHgt = tileHgt;
	r32 playerX   = g_game->playerPos.x - playerWid / 2.0f;
	r32 playerY   = g_game->playerPos.y - playerHgt;
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
    sound, debug, Qi_Init, Qi_GameUpdateAndRender,
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
