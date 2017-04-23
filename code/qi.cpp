//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Implementation file for the base QI API
//

#include "basictypes.h"

#include "qi.h"
#include "qi_sound.h"

struct
{
	int xOff;
	int yOff;
} g_game;

void
DrawGradient(Bitmap_s* osb, int xOff, int yOff)
{
	u32* xel = osb->pixels;
	for (u32 y = 0; y < osb->height; y++)
	{
		for (u32 x = 0; x < osb->width; x++)
		{
			u32 col = ((u8)x + xOff) << 16 | ((u8)y + yOff) << 8 | 0x10;
			*xel++  = col;
		}
	}
}

void
Qi_GameUpdateAndRender(Memory_s*      memory,
                       Input_s*       input,
                       Bitmap_s*      screenBitmap)
{
	DrawGradient(screenBitmap, g_game.xOff, g_game.yOff);
}
