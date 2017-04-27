#ifndef __QI_H

// qi.h
//
// Base engine definitions and declarations
//
// Part of the Qi engine and game, Copyright 2017, Jon Davis

#include "basictypes.h"
#include "qi_math.h"

#if defined(QI_PERFORMANCE)
#define Assert(foo) (void)(foo)
#else
#define Assert(foo) if (!(foo)) *(volatile u32 *)0 = 0xdeadbeef
#endif

struct SimpleInput_s
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

#define CONTROLLER_MAX_COUNT 4

struct Button_s
{
	bool endedDown;
	int  halfTransitionCount;
};

struct Analog_s
{
	Vec2_s startEnd;
	Vec2_s minMax;
};

struct Controller_s
{
	bool     analog;
	Analog_s leftStickX;
	Analog_s leftStickY;
	Analog_s rightStickX;
	Analog_s rightStickY;
	Analog_s leftTrigger;
	Analog_s rightTrigger;

	union {
		Button_s buttons[10];

		struct
		{
			Button_s upButton;
			Button_s rightButton;
			Button_s downButton;
			Button_s leftButton;

			Button_s aButton;
			Button_s bButton;
			Button_s xButton;
			Button_s yButton;

			Button_s startButton;
			Button_s backButton;
 
			Button_s leftShoulder;
			Button_s rightShoulder;
		};
	};
};

struct Input_s
{
	Controller_s controllers[CONTROLLER_MAX_COUNT];
};

struct Memory_s
{
    size_t permanentSize;
    u8* permanentStorage;
    u8* permanentPos;

    size_t transientSize;
    u8* transientStorage;
    u8* transientPos;
};

struct SoundBuffer_s;

void Qi_GameUpdateAndRender(Memory_s* memory, Input_s* input, Bitmap_s* screenBitmap);

#define __QI_H
#endif // #ifndef __QI_H
