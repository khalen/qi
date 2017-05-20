//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Implementation file for the base QI API
//

#include "basictypes.h"

#include "qi.h"
#include "qi_sound.h"

struct GameGlobals_s
{
	int       xOff;
	int       yOff;
	Memory_s* memory;
};

GameGlobals_s* game;

template<typename T>
T*
GameAllocate()
{
	Assert(game->memory && (uintptr_t)game->memory->permanentPos & 0xF == 0
	       && (game->memory->permanentSize
	           - (size_t)(game->memory->permanentPos - game->memory->permanentStorage > allocSize)));
	size_t allocSize = sizeof(T) + ((sizeof(T) + 15) & 0xF);
	T*     result    = (T*)game->memory->permanentPos;
	game->memory->permanentPos += allocSize;
	return result;
}

void
DrawGradient(Bitmap_s* osb, int xOff, int yOff)
{
	u32* xel = osb->pixels;
	for (u32 y = 0; y < osb->height; y++)
	{
		for (u32 x = 0; x < osb->width; x++)
		{
			u32 col = (((x + xOff) & 0xFF)) << 16 | (((y + yOff) & 0xFF)) << 8 | 0x10;
			*xel++  = col;
		}
	}
}

internal void
InitGameGlobals(Memory_s* memory)
{
	Assert(memory && (memory->permanentSize - (size_t)(memory->permanentPos - memory->permanentStorage)
	                  > sizeof(GameGlobals_s)));

	// This can't use GameAlloc<>(), because we haven't allocated the game globals struct yet.
	size_t allocSize = sizeof(GameGlobals_s) + ((sizeof(GameGlobals_s) + 15) & 0xF);
	game             = (GameGlobals_s*)memory->permanentPos;
	memory->permanentPos += allocSize;
	game->memory = memory;
}

internal void
UpdateGameState(Input_s* input)
{
	Assert(game);
	for (int controllerIdx = 0; controllerIdx < CONTROLLER_MAX_COUNT; controllerIdx++)
	{
		const Controller_s* controller = &input->controllers[controllerIdx];
		game->xOff -= (int)(5 * controller->leftStick.dir.x);
		game->yOff += (int)(5 * controller->leftStick.dir.y);
	}
}

void
Qi_GameUpdateAndRender(Memory_s* memory, Input_s* input, Bitmap_s* screenBitmap)
{
	if (game == nullptr)
	{
		InitGameGlobals(memory);
		Qis_Init();
	}

	UpdateGameState(input);
	DrawGradient(screenBitmap, game->xOff, game->yOff);
}
