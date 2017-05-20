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
#define Assert(foo) \
	if (!(foo))     \
	*(volatile u32*)0 = 0xdeadbeef
#endif

#define TARGET_FPS 30.0

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

#define CONTROLLER_MAX_COUNT 5
#define KBD (CONTROLLER_MAX_COUNT - 1)

struct Button_s
{
	bool endedDown;
	int  halfTransitionCount;
};

#define TRIGGER_DEADZONE 0.1f
struct Trigger_s
{
	r32    reading;
	r32    __Pad0;
	Vec2_s minMax;
};

#define ANALOG_DEADZONE 0.1f
struct Analog_s
{
	Trigger_s trigger;
	Vec2_s    dir;
};

struct Controller_s
{
	bool isAnalog;
	bool isConnected;

	union {
#define ANALOG_COUNT 2
		Analog_s analogs[ANALOG_COUNT];
		struct
		{
			Analog_s leftStick;
			Analog_s rightStick;
		};
	};

	union {
#define TRIGGER_COUNT 2
		Trigger_s triggers[TRIGGER_COUNT];
		struct
		{
			Trigger_s leftTrigger;
			Trigger_s rightTrigger;
		};
	};

	union {
#define BUTTON_COUNT 12
		Button_s buttons[BUTTON_COUNT];

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
	u8*    permanentStorage;
	u8*    permanentPos;

	size_t transientSize;
	u8*    transientStorage;
	u8*    transientPos;
};

struct SoundBuffer_s;

void Qi_GameUpdateAndRender(Memory_s* memory, Input_s* input, Bitmap_s* screenBitmap);

// Functions to be provided by the platform layer
void* QiPlat_ReadEntireFile(const char* fileName, size_t* fileSize);
bool QiPlat_WriteEntireFile(const char* fileName, const void* ptr, const size_t size);
void QiPlat_ReleaseFileBuffer(void* buffer);
r64 QiPlat_WallSeconds();

#define __QI_H
#endif // #ifndef __QI_H
