#ifndef __QI_H
#define __QI_H

// qi.h
//
// Base engine definitions and declarations
//
// Part of the Qi engine and game, Copyright 2017, Jon Davis

#include "basictypes.h"
#include "math_util.h"
#include "debug.h"
#include "bitmap.h"
#include "hwi.h"
#include "stringtable.h"
#include <string.h>

#define GAME_DLL_NAME "qi.dll"

#define TARGET_FPS 30.0

#define GAME_IDEAL_RES_X 1920
#define GAME_IDEAL_RES_Y 1080

#define TILE_IDEAL_RES_X 60
#define TILE_IDEAL_RES_Y 60

#define GAME_DOWNRES_FACTOR 1

#define GAME_RES_X (GAME_IDEAL_RES_X / GAME_DOWNRES_FACTOR)
#define GAME_RES_Y (GAME_IDEAL_RES_Y / GAME_DOWNRES_FACTOR)

#define TILE_RES_X (TILE_IDEAL_RES_X / GAME_DOWNRES_FACTOR)
#define TILE_RES_Y (TILE_IDEAL_RES_Y / GAME_DOWNRES_FACTOR)

#define BASE_SCREEN_TILES_X (GAME_RES_X / TILE_RES_X)
#define BASE_SCREEN_TILES_Y (GAME_RES_Y / TILE_RES_Y)

#define SCREEN_SCALE 1.0f

struct Rect
{
	r32 left;
	r32 top;
	r32 width;
	r32 height;
};

struct Rect16
{
	i16 left;
	i16 top;
	i16 width;
	i16 height;
};

#define CONTROLLER_MAX_COUNT 5
#define KBD                  (CONTROLLER_MAX_COUNT - 1)

#define MAX_SPRITES_PER_ATLAS 1024
#define MAX_ATLASES_PER_TABLE 32

struct SpriteFrame
{
	v2  topLeftUV;
	v2  bottomRightUV;
	r32 weight;
};

struct SpriteAtlas;

struct Sprite
{
	const SpriteAtlas *atlas;
	Symbol             name;
	ColorU             tint;
	iv2                size;
	iv2                origin;
	u32                numFrames;
	SpriteFrame *      frames; // If this is not a ref sprite, frames will point immediately after the Sprite struct.
};

struct SpriteAtlas
{
	char    imageFile[128];
	Symbol  name;
	Bitmap *bitmap;
	iv2     baseOrigin;
	iv2     baseSize;
	u32     numSprites;
	Sprite *sprites[MAX_SPRITES_PER_ATLAS];
};

struct SpriteAtlasTable
{
	SpriteAtlas atlases[MAX_ATLASES_PER_TABLE];
	u32         numAtlases;
};

struct MemoryArena;
SpriteAtlasTable *Game_GetAtlasTable();
void              Game_CopyAtlasTableUsingArena(MemoryArena *arena, SpriteAtlasTable *destAtlases, const SpriteAtlasTable *srcAtlases);
void              Game_CopyAtlases(const SpriteAtlasTable *srcAtlases);

struct KeyStore;
typedef u32 ValueRef;
void        Spr_ReadAtlasFromKeyStore(const KeyStore *ks, ValueRef ref, SpriteAtlas *atlas);
void        Spr_WriteAtlasToKeyStore(KeyStore **ks, const SpriteAtlas *atlas);
void        Spr_CreateSimpleTileSheet(SpriteAtlas *atlas, const char *name, u32 tileSizeX, u32 tileSizeY, u32 *tileCount);
void        Spr_Draw(Bitmap *dest, Sprite *sprite, i32 frame, i32 x, i32 y, ColorU tint = ColorU(255, 255, 255, 255));
void        Spr_DrawStretched(Bitmap *dest, Sprite *sprite, i32 frame, i32 x, i32 y, i32 w, i32 h, ColorU tint = ColorU(255, 255, 255, 255));

struct SpriteDLEntry
{
	Sprite *sprite;
	i32     x, y;
	i32     frame;
	ColorU  tint;
};
static_assert(sizeof(SpriteDLEntry) == 24, "Weirdly sized SpriteDLEntry");
void Spr_DrawList(Bitmap *dest, SpriteDLEntry *entries, u32 numEntries);

struct Button
{
	bool endedDown;
	int  halfTransitionCount;
};

#define TRIGGER_DEADZONE 0.1f
struct Trigger
{
	r32 reading;
	r32 __Pad0;
	v2  minMax;
};

#define ANALOG_DEADZONE 0.1f
struct Analog
{
	Trigger trigger;
	v2      dir;
};

struct Controller
{
	bool isAnalog;
	bool isConnected;
	bool __pad[2];

	union
	{
#define ANALOG_COUNT 2
		Analog analogs[ANALOG_COUNT];
		struct
		{
			Analog leftStick;
			Analog rightStick;
		};
	};

	union
	{
#define TRIGGER_COUNT 2
		Trigger triggers[TRIGGER_COUNT];
		struct
		{
			Trigger leftTrigger;
			Trigger rightTrigger;
		};
	};

	union
	{
#define BUTTON_COUNT 15
		Button buttons[BUTTON_COUNT];

		struct
		{
			Button upButton;
			Button rightButton;
			Button downButton;
			Button leftButton;

			Button aButton;
			Button bButton;
			Button xButton;
			Button yButton;

			Button startButton;
			Button backButton;

			Button leftShoulder;
			Button rightShoulder;

			Button leftStickButton;
			Button rightStickButton;

			Button padButton;
		};
	};
};

struct Mouse
{
	union
	{
#define MOUSE_BUTTON_COUNT 5
		v2     pos;
		Button buttons[MOUSE_BUTTON_COUNT];

		struct
		{
			Button leftButton;
			Button middleButton;
			Button rightButton;
			Button x1Button;
			Button x2Button;
		};
	};
};

struct Input
{
	Time_t     dT;
	Controller controllers[CONTROLLER_MAX_COUNT];
	Mouse      mouse;
};

struct SubSystem;
typedef void Qi_Init_SubSystem_f(const SubSystem *sys, bool isReInit);

struct SubSystem
{
	const char *         name;
	Qi_Init_SubSystem_f *initFunc;
	size_t               globalSize;
	void *               globalPtr;
};

#define MAX_SUBSYSTEMS 16

struct ThreadContext
{
	int dummy;
};

struct Memory;

// Game / Platform Linkage
struct PlatFuncs_s;
struct GameFuncs_s;

struct Bitmap;

// Functions provided by the base game layer
typedef void Qi_GameUpdateAndRender_f(ThreadContext *tc, Input *input, Bitmap *screenBitmap);
typedef void Qi_Init_f(const PlatFuncs_s *plat, Memory *memory);
typedef Hwi *Qi_GetHwi_f();
typedef void Qi_ToggleEditor_f();

struct SoundFuncs_s;

#if HAS(DEV_BUILD) || HAS(PROF_BUILD)
struct DebugFuncs_s;
#endif

struct GameFuncs_s
{
	const SoundFuncs_s *sound;
#if HAS(DEV_BUILD)
	const DebugFuncs_s *debug;
#endif

	Qi_Init_f *               Init;
	Qi_GameUpdateAndRender_f *UpdateAndRender;
	Qi_GetHwi_f *             GetHwi;
	Qi_ToggleEditor_f *       ToggleEditor;
};

// Functions to be provided by the platform layer
typedef void *QiPlat_ReadEntireFile_f(ThreadContext *tc, const char *fileName, size_t *fileSize);
typedef bool  QiPlat_WriteEntireFile_f(ThreadContext *tc, const char *fileName, const void *ptr, const size_t size);
typedef void  QiPlat_ReleaseFileBuffer_f(ThreadContext *tc, void *buffer);
typedef r64   QiPlat_WallSeconds_f(ThreadContext *tc);
typedef void  QiPlat_SetupMainExeLibraries_f();
struct ImGuiContext;
typedef ImGuiContext *QiPlat_GetGuiContext();

struct PlatFuncs_s
{
	QiPlat_ReadEntireFile_f *       ReadEntireFile;
	QiPlat_WriteEntireFile_f *      WriteEntireFile;
	QiPlat_ReleaseFileBuffer_f *    ReleaseFileBuffer;
	QiPlat_WallSeconds_f *          WallSeconds;
	QiPlat_SetupMainExeLibraries_f *SetupMainExeLibraries;
	QiPlat_GetGuiContext *          GetGuiContext;
};

extern const PlatFuncs_s * plat;
extern const SoundFuncs_s *sound;

#if HAS(DEV_BUILD) || HAS(PROF_BUILD)
extern const DebugFuncs_s *debug;
#endif

#endif // #ifndef __QI_H
