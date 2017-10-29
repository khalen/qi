#ifndef __QI_H

// qi.h
//
// Base engine definitions and declarations
//
// Part of the Qi engine and game, Copyright 2017, Jon Davis

#include "basictypes.h"
#include "qi_math.h"
#include "qi_debug.h"
#include <string.h>

#define GAME_DLL_NAME "qi.dll"

#define TARGET_FPS 30.0

#define GAME_IDEAL_RES_X 1920
#define GAME_IDEAL_RES_Y 1080

#define TILE_IDEAL_RES_X 60
#define TILE_IDEAL_RES_Y 60

#define GAME_DOWNRES_FACTOR 2

#define GAME_RES_X (GAME_IDEAL_RES_X / GAME_DOWNRES_FACTOR)
#define GAME_RES_Y (GAME_IDEAL_RES_Y / GAME_DOWNRES_FACTOR)

#define TILE_RES_X (TILE_IDEAL_RES_X / GAME_DOWNRES_FACTOR)
#define TILE_RES_Y (TILE_IDEAL_RES_Y / GAME_DOWNRES_FACTOR)

#define BASE_SCREEN_TILES_X (GAME_RES_X / TILE_RES_X)
#define BASE_SCREEN_TILES_Y (GAME_RES_Y / TILE_RES_Y)

#define SCREEN_SCALE 1.0f

struct Rect_s
{
	r32 left;
	r32 top;
	r32 width;
	r32 height;
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
	v2 minMax;
};

#define ANALOG_DEADZONE 0.1f
struct Analog_s
{
	Trigger_s trigger;
	v2    dir;
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
#define BUTTON_COUNT 17
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

			Button_s leftMouse;
			Button_s middleMouse;
			Button_s rightMouse;

            Button_s leftStickButton;
            Button_s rightStickButton;
		};
	};
};

struct Input_s
{
	Time_t       dT;
	Controller_s controllers[CONTROLLER_MAX_COUNT];
	v2       mouse;
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

struct MemoryArena_s
{
	u8*    base;
	size_t size;
	size_t curOffset;
};

void MemoryArena_Init( MemoryArena_s* arena, const size_t size );
u8* MemoryArena_Alloc( MemoryArena_s* arena, const size_t size );

struct SubSystem_s;
typedef void Qi_Init_SubSystem_f(const SubSystem_s* sys, bool isReInit);

struct SubSystem_s
{
	const char*          name;
	Qi_Init_SubSystem_f* initFunc;
	size_t               globalSize;
	void*                globalPtr;
};

#define MAX_SUBSYSTEMS 16

void* Qim_AllocRaw(Memory_s* memory, const size_t size);
template<typename T>
T*
Qim_New(Memory_s* memory)
{
	void* buf = Qim_AllocRaw(memory, sizeof(T));
	Assert(buf != nullptr);
	memset(buf, 0, sizeof(T));
	return reinterpret_cast<T*>(buf);
}

void* Qim_TransientAllocRaw(Memory_s* memory, const size_t size);
template<typename T>
T*
Qim_TransientNew(Memory_s* memory)
{
	void* buf = Qim_TransientAllocRaw(memory, sizeof(T));
	Assert(buf != nullptr);
	memset(buf, 0, sizeof(T));
	return reinterpret_cast<T*>(buf);
}

struct ThreadContext_s
{
	int dummy;
};

// Game / Platform Linkage
struct PlatFuncs_s;
struct GameFuncs_s;

// Functions provided by the base game layer
typedef void Qi_GameUpdateAndRender_f(ThreadContext_s* tc, Input_s* input, Bitmap_s* screenBitmap);
typedef void Qi_Init_f(const PlatFuncs_s* plat, Memory_s* memory);

struct SoundFuncs_s;

#if HAS(DEV_BUILD) || HAS(PROF_BUILD)
struct DebugFuncs_s;
#endif

struct GameFuncs_s
{
	const SoundFuncs_s* sound;
#if HAS(DEV_BUILD)
	const DebugFuncs_s* debug;
#endif

	Qi_Init_f*                Init;
	Qi_GameUpdateAndRender_f* UpdateAndRender;
};

// Functions to be provided by the platform layer
typedef void* QiPlat_ReadEntireFile_f(ThreadContext_s* tc, const char* fileName, size_t* fileSize);
typedef bool  QiPlat_WriteEntireFile_f(ThreadContext_s* tc, const char* fileName, const void* ptr, const size_t size);
typedef void  QiPlat_ReleaseFileBuffer_f(ThreadContext_s* tc, void* buffer);
typedef r64   QiPlat_WallSeconds_f(ThreadContext_s* tc);

struct PlatFuncs_s
{
	QiPlat_ReadEntireFile_f*    ReadEntireFile;
	QiPlat_WriteEntireFile_f*   WriteEntireFile;
	QiPlat_ReleaseFileBuffer_f* ReleaseFileBuffer;
	QiPlat_WallSeconds_f*       WallSeconds;
};

extern const PlatFuncs_s*  plat;
extern const SoundFuncs_s* sound;

#if HAS(DEV_BUILD) || HAS(PROF_BUILD)
extern const DebugFuncs_s* debug;
#endif

#define __QI_H
#endif // #ifndef __QI_H
