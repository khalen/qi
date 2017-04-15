//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Implementation file for the base QI API
//

#include "basictypes.h"

#include "qi.h"


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

