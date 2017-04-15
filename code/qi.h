#ifndef __QI_H

// qi.h
//
// Base engine definitions and declarations
//
// Part of the Qi engine and game, Copyright 2017, Jon Davis

#include "basictypes.h"

struct Vec2_s
{
	union {
		float f[2];
		struct
		{
			float x;
			float y;
		};
	};
};

struct Input_s
{
	// Dpad
	bool up, right, down, left;

	// Buttons
	bool aButton, bButton, xButton, yButton, startButton, backButton;
	bool leftShoulder, rightShoulder;

	// Analog
	float  leftTrigger, rightTrigger;
	Vec2_s leftStick;
	Vec2_s rightStick;
};

struct Rect_s
{
	i32 left;
	i32 top;
	i32 width;
	i32 height;
};

struct Bitmap_s
{
	// Always 32 bit, xel order is BB GG RR 00 (LE)
	u32* pixels;

	// Dimensions are in pixels, not bytes
	u32 width;
	u32 height;
	u32 pitch;
	u32 byteSize;
};


void DrawGradient(Bitmap_s* osb, int xOff, int yOff);
void GameUpdateAndRender( Bitmap_s* screenBitmap );

#define __QI_H
#endif // #ifndef __QI_H
