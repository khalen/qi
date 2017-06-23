//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Implementation file for the base QI API
//

#include "basictypes.h"

#include "qi.h"

#include "qi_debug.cpp"
#include "qi_math.cpp"
#include "qi_sound.cpp"

struct GameGlobals_s
{
	int       xOff;
	int       yOff;
	Memory_s* memory;
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

internal void
InitGameGlobals(Memory_s* memory)
{
	Assert(memory && (memory->permanentSize - (size_t)(memory->permanentPos - memory->permanentStorage)
	                  > sizeof(GameGlobals_s)));

	// This can't use G_GameAlloc<>(), because we haven't allocated the g_game globals struct yet.
	size_t allocSize = sizeof(GameGlobals_s) + ((sizeof(GameGlobals_s) + 15) & 0xF);
	g_game           = (GameGlobals_s*)memory->permanentPos;
	memory->permanentPos += allocSize;
	g_game->memory = memory;
}

internal void
UpdateGameState(Input_s* input)
{
	Assert(g_game);
	for (int controllerIdx = 0; controllerIdx < CONTROLLER_MAX_COUNT; controllerIdx++)
	{
		const Controller_s* controller = &input->controllers[controllerIdx];
		g_game->xOff -= (int)(5 * controller->leftStick.dir.x);
		g_game->yOff += (int)(5 * controller->leftStick.dir.y);
	}
}

void
Qi_GameUpdateAndRender(Memory_s* memory, Input_s* input, Bitmap_s* screenBitmap)
{
	if (g_game == nullptr)
	{
		InitGameGlobals(memory);
		Qis_Init();
	}

	UpdateGameState(input);
	DrawGradient(screenBitmap, g_game->xOff, g_game->yOff);
}

const PlatFuncs_s* plat;
void
Qi_Init(const PlatFuncs_s* platFuncs)
{
	plat = platFuncs;
}

internal GameFuncs_s s_game = {
    sound, debug, Qi_Init, Qi_GameUpdateAndRender,
};
const GameFuncs_s* game = &s_game;

// Interface to platform code
extern "C"
{
PLAT_EXPORT const GameFuncs_s* Qi_GetGameFuncs()
{
    return game;
}
}
