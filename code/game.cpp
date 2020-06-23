//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Implementation file for the base QI API
//

#include "basictypes.h"

#include "game.h"
#include "memory.h"
#include "tile.h"
#include "math_util.h"
#include "gjk.h"
#include "util.h"
#include "noise.h"
#include "keystore.h"
#include "qed_parse.h"
#include "bitmap.h"
#include "hwi.h"
#include "editor.h"
#include <stdio.h>
#include <imgui.h>

#define ROOM_WID BASE_SCREEN_TILES_X
#define ROOM_HGT BASE_SCREEN_TILES_Y

#define SCREEN_NUM_TILES_X (ROOM_WID * SCREEN_SCALE)
#define SCREEN_NUM_TILES_Y (ROOM_HGT * SCREEN_SCALE)

#define SCREEN_WORLD_SIZE_X (SCREEN_NUM_TILES_X * TILE_SIZE_METERS_X)
#define SCREEN_WORLD_SIZE_Y (SCREEN_NUM_TILES_Y * TILE_SIZE_METERS_Y)

#define PLAYER_RADIUS_X (0.5f * TILE_SIZE_METERS_X)
#define PLAYER_RADIUS_Y (0.25f * TILE_SIZE_METERS_Y)

#define MAX_SPRITE_ATLASES 32

struct GameGlobals_s
{
	bool      isInitialized;
	Memory *  memory;
	SubSystem gameSubsystems[MAX_SUBSYSTEMS];

	i32 screenWid;
	i32 screenHgt;

	World_s     world;
	WorldPos_s  playerPos;
	v2          dPlayerPos;
	WorldPos_s  cameraPos;
	MemoryArena tileArena;
	MemoryArena spriteArena;
	MemoryArena assetArena;

	Bitmap testBitmaps[5];
	Bitmap playerBmps[4][3];
	i32    playerFacingIdx;

	BuddyAllocator *testAlloc;
	Bitmap* screenBitmap;
	Bitmap testBitmap;

	bool enableEditor;

	SpriteAtlas atlases[MAX_SPRITE_ATLASES];
	u32         numAtlases;
};

GameGlobals_s *g_game;

template<typename T>
T *GameAllocate()
{
	return M_New<T>(g_game->memory);
}

template<typename T>
T *MemoryArena_PushStruct(MemoryArena *arena)
{
	T *newT = (T *)MA_Alloc(arena, sizeof(T));
	return newT;
}

static void InitGameGlobals(const SubSystem *, bool);

internal void TestKeyStore(void)
{
#if 0
	KeyStore* ks = KS_Create("TestKS");

	ValueRef subObj = KS_AddObject(&ks, 0);
	KS_ObjectSetValue(&ks, subObj, "fooInt", KS_AddInt(&ks, 12345));
	KS_ObjectSetValue(&ks, subObj, "fooInt", KS_AddInt(&ks, 54321));
	KS_ObjectSetValue(&ks, subObj, "strVal", KS_AddString(&ks, "This is a test of the emergency broadcast system.  This is only a test. In the event of an actual emergency asdjajhrg a hahr k a a efahefg g a kefagsh"));
	KS_ObjectSetValue(&ks, subObj, "symVal", KS_AddSymbol(&ks, "IAmASymbol"));
	ValueRef subSubArray = KS_AddArray(&ks, 2);
	KS_ArrayPush(&ks, subSubArray, KS_AddReal(&ks, 1.5432));
	KS_ArrayPush(&ks, subSubArray, KS_AddString(&ks, "Lincoln was shot and killed"));
	Assert(KS_ArrayCount(ks, subSubArray) == 2);
	for (u32 i = 0; i < KS_ArrayCount(ks, subSubArray); i++)
	{
		printf("Array %d: 0x%x", i, KS_ArrayElem(ks, subSubArray, i));
	}
	ValueRef subSubSubArray = KS_AddArray(&ks, 0);
	KS_ArrayPush(&ks, subSubSubArray, KS_AddInt(&ks, 99999));
	KS_ArrayPush(&ks, subSubArray, subSubSubArray);
	KS_ObjectSetValue(&ks, subObj, "nestedArrayTest", subSubArray);
	KS_ObjectSetValue(&ks, KS_Root(ks), "testObj", subObj);
	KS_ObjectSetValue(&ks, KS_Root(ks), "testInt", KS_AddInt(&ks, 911911911));

	char* tmpBuf = (char *)g_game->memory->transientPos;
	u32 len = KS_ValueToString(ks, KS_Root(ks), tmpBuf, g_game->memory->transientSize, true);
	plat->WriteEntireFile(nullptr, "test.qed", tmpBuf, len);

	KeyStore* ksf = nullptr;
	const char* error = QED_LoadFile(&ksf, "Test", "test.qed");
	if (error != nullptr)
	{
		printf("Error loading test.qed: %s", error);
	}

	tmpBuf = (char*)g_game->memory->transientPos;
	len    = KS_ValueToString(ksf, KS_Root(ksf), tmpBuf, g_game->memory->transientSize, true);
	plat->WriteEntireFile(nullptr, "test1.qed", tmpBuf, len);

	KS_Free(&ks);
	KS_Free(&ksf);

	ksf   = nullptr;
	error = QED_LoadFile(&ksf, "Sample", "sample.qed");
	if (error != nullptr)
	{
		printf("Error loading sample.qed: %s", error);
	}

	tmpBuf = (char*)g_game->memory->transientPos;
	len    = KS_ValueToString(ksf, KS_Root(ksf), tmpBuf, g_game->memory->transientSize, true);
	plat->WriteEntireFile(nullptr, "sampleout.qed", tmpBuf, len);

	KS_Free(&ksf);
#endif
}

#if 0
internal void
PV(const char* msg, const v2& v)
{
	printf("%s %f %f\n", msg, v.x, v.y);
}
#endif

#define MAX_DEBUG_SHAPES 50
PolyShape_s debugShapes[MAX_DEBUG_SHAPES];
u32         debugShapeColors[MAX_DEBUG_SHAPES];
u32         numDebugShapes;

static inline void PlotPt(Bitmap *bitmap, i32 x, i32 y, u32 rgba)
{
	if (x < 0 || x >= (i32)bitmap->width || y < 0 || y >= (i32)bitmap->height)
		return;

	*(bitmap->pixels + x + bitmap->pitch * y) = rgba;
}

void DrawDebugLine(Bitmap *screen, v2 &p0, v2 &p1, u32 rgba)
{
#if 0
	i32 x0 = p0.x + 0.5f;
	i32 x1 = p1.x + 0.5f;
	i32 y0 = p0.y + 0.5f;
	i32 y1 = p1.y + 0.5f;

	if (abs(x1 - x0) < abs(y1 - y0))
	{
		if (y1 < y0)
		{
			Swap(x1, x0);
			Swap(y1, y0);
		}
		i32 dxdy = (y1 == y0) ? 0 : ((x1 - x0) * 0xFFFF) / (y1 - y0);
		x0 <<= 16;
		for (i32 y = y0; y < y1; y++)
		{
			PlotPt(screen, x0 >> 16, y, rgba);
			x0 += dxdy;
		}
	}
	else
	{
		if (x1 < x0)
		{
			Swap(x1, x0);
			Swap(y1, y0);
		}
		i32 dydx = (x1 == x0) ? 0 : ((y1 - y0) * 0xFFFF) / (x1 - x0);
		y0 <<= 16;
		for (i32 x = x0; x < x1; x++)
		{
			PlotPt(screen, x, y0 >> 16, rgba);
			y0 += dydx;
		}
	}
#else
	if (screen != g_game->screenBitmap)
		gHwi->PushRenderTarget(screen);
	gHwi->DrawLine(p0, p1, ColorU(rgba));
	if (screen != g_game->screenBitmap)
		gHwi->PopRenderTarget();
#endif
}

void DrawDebugPoly(Bitmap *screen, PolyShape_s *poly, u32 rgba)
{
	v2 halfScreen   = V2(screen->width / 2.0f, screen->height / 2.0f);
	v2 camPosMeters = WorldPosToMeters(&g_game->cameraPos);
	for (u32 pi = 0; pi < poly->numPoints; pi++)
	{
		u32 pn          = (pi + 1) % poly->numPoints;
		v2  p0Cam       = MetersToScreenPixels(poly->points[pi] - camPosMeters);
		v2  p1Cam       = MetersToScreenPixels(poly->points[pn] - camPosMeters);
		v2  p0CamScreen = p0Cam + halfScreen;
		v2  p1CamScreen = p1Cam + halfScreen;
		DrawDebugLine(screen, p0CamScreen, p1CamScreen, rgba);
	}
}

void DrawDebugShapes(Bitmap *screen)
{
	for (u32 shape = 0; shape < numDebugShapes; shape++)
		DrawDebugPoly(screen, &debugShapes[shape], debugShapeColors[shape]);
}

void AddDebugShape(PolyShape_s *poly, r32 r, r32 g, r32 b)
{
	u32 rgba                         = ((u32)(r * 255.0f + 0.5f) << 24) | ((u32)(g * 255 + 0.5f) << 16) | ((u32)(b * 255 + 0.5f) << 8) | 0xFF;
	debugShapes[numDebugShapes]      = *poly;
	debugShapeColors[numDebugShapes] = rgba;
	numDebugShapes++;
}

void MakeSimplexShapes(PolyShape_s *shapea, PolyShape_s *shapeb, GjkResult_s *result)
{
	shapea->numPoints = shapeb->numPoints = result->simplex.numPts;
	for (u32 i = 0; i < result->simplex.numPts; i++)
	{
		shapea->points[i] = result->simplex.pts[i].sa;
		shapeb->points[i] = result->simplex.pts[i].sb;
	}
}

internal void UpdateGameState(Bitmap *screen, Input *input)
{
	numDebugShapes = 0;
	Assert(g_game);

	// Controller 0 left stick
	Analog *  ctr0        = &input->controllers[KBD].analogs[0];
	const r32 ddPlayerMag = 35.0f + (50.0f * input->controllers[KBD].leftStickButton.endedDown);

	v2 ddPlayer = ctr0->dir * ctr0->trigger.reading * ddPlayerMag;
	ddPlayer += -8.7f * g_game->dPlayerPos;

	v2 playerOffset = {};
	playerOffset    = (ddPlayer * 0.5f * input->dT * input->dT) + g_game->dPlayerPos * input->dT;

	WorldPos_s oldPlrPos = g_game->playerPos;
	WorldPos_s newPlrPos = g_game->playerPos;
	AddSubtileOffset(&newPlrPos, playerOffset);

	g_game->dPlayerPos += ddPlayer * input->dT;

	// Compute bounding box of total player move in camera space
	WorldPos_s oldPlrPosUL = oldPlrPos;
	WorldPos_s oldPlrPosLR = oldPlrPos;
	AddSubtileOffset(&oldPlrPosUL, V2(-PLAYER_RADIUS_X, -PLAYER_RADIUS_Y));
	AddSubtileOffset(&oldPlrPosLR, V2(PLAYER_RADIUS_X, PLAYER_RADIUS_Y));

	WorldPos_s newPlrPosUL = newPlrPos;
	WorldPos_s newPlrPosLR = newPlrPos;
	AddSubtileOffset(&newPlrPosUL, V2(-PLAYER_RADIUS_X, -PLAYER_RADIUS_Y));
	AddSubtileOffset(&newPlrPosLR, V2(PLAYER_RADIUS_X, PLAYER_RADIUS_Y));

	i64 tileMinX, tileMinY, tileMaxX, tileMaxY;
	tileMinX = Min(oldPlrPosUL.x.tile, newPlrPosLR.x.tile);
	tileMinY = Min(oldPlrPosUL.y.tile, newPlrPosLR.y.tile);
	tileMaxX = Max(oldPlrPosUL.x.tile, newPlrPosLR.x.tile);
	tileMaxY = Max(oldPlrPosUL.y.tile, newPlrPosLR.y.tile);

	bool collided = false;

	// BoxShape_s plrCollideShape(WorldPosToMeters(&newPlrPos), V2(PLAYER_RADIUS_X, PLAYER_RADIUS_Y));
	// AddDebugShape(&plrCollideShape, 1.0f, 0.0f, 1.0f);

	for (i32 y = tileMinY; y <= tileMaxY; y++)
	{
		for (i32 x = tileMinX; x <= tileMaxX; x++)
		{
			u32 tileVal = GetTileValue(&g_game->world, x, y);

			if (tileVal == TILE_EMPTY)
				continue;

			GjkResult_s result;
			EllipseShape_s plrCollideShape(WorldPosToMeters(&newPlrPos), V2(PLAYER_RADIUS_X, PLAYER_RADIUS_Y));
			//BoxShape_s plrCollideShape(WorldPosToMeters(&newPlrPos), V2(PLAYER_RADIUS_X, PLAYER_RADIUS_Y));
			BoxShape_s tileShape(V2((r32)x * TILE_SIZE_METERS_X, (r32)y * TILE_SIZE_METERS_Y), V2(TILE_SIZE_METERS_X / 2, TILE_SIZE_METERS_Y / 2));
			AddDebugShape(&tileShape, 1.0f, 1.0f, 0.0f);
			PolyShape_s gjkResulta, gjkResultb;
			if (Qi_Gjk(&result, &plrCollideShape, &tileShape))
			{
				collided = true;
				AddSubtileOffset(&newPlrPos, result.penetrationNormal * result.penetrationDepth);
				g_game->dPlayerPos += -result.penetrationNormal * g_game->dPlayerPos.dot(result.penetrationNormal);

				MakeSimplexShapes(&gjkResulta, &gjkResultb, &result);
				AddDebugShape(&gjkResulta, 0.0f, 1.0f, 0.0f);
				AddDebugShape(&gjkResultb, 0.0f, 1.0f, 0.5f);
			}
			else
			{
				MakeSimplexShapes(&gjkResulta, &gjkResultb, &result);
				AddDebugShape(&gjkResulta, 0.0f, 1.0f, 1.0f);
				AddDebugShape(&gjkResultb, 0.5f, 1.0f, 1.0f);
			}
		}
	}

	g_game->playerPos = newPlrPos;

	// Offset camera half a tile since we want it to be centered in a screen
	g_game->cameraPos.x.offset = g_game->cameraPos.y.offset = -0.5f;
	g_game->cameraPos.z.offset                              = 0.0f;

	i64 playerXScreen = (i64)floor((r64)g_game->playerPos.x.tile / SCREEN_NUM_TILES_X);
	i64 playerYScreen = (i64)floor((r64)g_game->playerPos.y.tile / SCREEN_NUM_TILES_Y);

	g_game->cameraPos.x.tile = (playerXScreen * SCREEN_NUM_TILES_X) + SCREEN_NUM_TILES_X / 2;
	g_game->cameraPos.y.tile = (playerYScreen * SCREEN_NUM_TILES_Y) + SCREEN_NUM_TILES_Y / 2;
	g_game->cameraPos.z.tile = g_game->playerPos.z.tile;

	g_game->cameraPos = g_game->playerPos;

	if (ctr0->dir.y < 0)
		g_game->playerFacingIdx = 0;
	if (ctr0->dir.x > 0)
		g_game->playerFacingIdx = 1;
	if (ctr0->dir.y > 0)
		g_game->playerFacingIdx = 2;
	if (ctr0->dir.x < 0)
		g_game->playerFacingIdx = 3;
}

internal void VerifyLoad(const char *msg)
{
	if (!msg)
		return;

	fprintf(stderr, "Load failed: %s", msg);
	exit(1);
}

SpriteAtlas *Game_GetAtlases(u32 *numAtlases)
{
	Assert(numAtlases);
	*numAtlases = g_game->numAtlases;
	return g_game->atlases;
}

static void LoadFrame(const KeyStore* ks, const SpriteAtlas *atlas, const Sprite *sprite, SpriteFrame *frame, ValueRef frameRef)
{
	const r32 iAtlasW = 1.0f / atlas->bitmap->width;
	const r32 iAtlasH = 1.0f / atlas->bitmap->height;
	const r32 iSpriteW = 1.0f / sprite->size.x;
	const r32 iSpriteH = 1.0f / sprite->size.y;

	// Atlas coords are specified top left corner, width and height
	// Must convert to bottom left, top right in UVs
	iv2 pos = KS_GetKeySmallInt2(ks, frameRef, "pos");
	frame->topLeftUV.x = pos.x * iAtlasW;
	frame->topLeftUV.y = pos.y * iAtlasH;
	frame->bottomRightUV.x = (pos.x + sprite->size.x) * iAtlasW;
	frame->bottomRightUV.y = (pos.y + sprite->size.y) * iAtlasH;

	frame->weight = (r32)KS_GetKeyReal(ks, frameRef, "weight", 1.0);
	printf("    Frame %2.4f  %2.4f  %2.4f  %2.4f\n", frame->topLeftUV.x, frame->topLeftUV.y, frame->bottomRightUV.x, frame->bottomRightUV.y);
}

const Sprite *Spr_FindSprite(const SpriteAtlas *atlas, Symbol name)
{
	for (u32 i = 0; i < atlas->numSprites; i++)
	{
		if (atlas->sprites[i]->name == name)
			return atlas->sprites[i];
	}
	return nullptr;
}

internal Sprite* LoadSprite(const KeyStore* ks, const SpriteAtlas* atlas, ValueRef spriteRef)
{
	ValueRef frames = KS_ObjectGetValue(ks, spriteRef, "frames");
	u32 frameCount = 0;

	if (frames != NilValue)
		frameCount = KS_ArrayCount(ks, frames);

	Sprite *sprite = (Sprite *)MA_Alloc(&g_game->spriteArena, sizeof(Sprite) + frameCount * sizeof(SpriteFrame));
	memset(sprite, 0, sizeof(Sprite));

	Symbol spriteName = KS_GetKeySymbol(ks, spriteRef, "name", ST_Intern(KS_GetStringTable(), "(unnamed)"));
	printf("Loading sprite: %s\n", ST_ToString(KS_GetStringTable(),spriteName));

	// See if we're a ref to another sprite so we can reuse its frames
	Symbol refName = KS_GetKeySymbol(ks, spriteRef, "ref");
	if (ST_Valid(refName))
	{
		printf("  Ref sprite: %s\n", ST_ToString(KS_GetStringTable(), refName));
		const Sprite* other = Spr_FindSprite(atlas, refName);
		Assert(other != nullptr);
		*sprite = *other;
	}
	else
	{
		AssertMsg(frames != NilValue, "Non ref sprite '%s' must have frames", ST_ToString(KS_GetStringTable(), spriteName));

		sprite->size   = KS_GetKeySmallInt2(ks, spriteRef, "size", atlas->baseSize);
		sprite->origin = KS_GetKeySmallInt2(ks, spriteRef, "origin", atlas->baseOrigin);

		sprite->numFrames = KS_ArrayCount(ks, frames);
		sprite->frames = (SpriteFrame *)(sprite + 1);
		u32 i;
		for (i = 0; i < sprite->numFrames; i++)
		{
			ValueRef frame = KS_ArrayElem(ks, frames, i);
			LoadFrame(ks, atlas, sprite, &sprite->frames[i], frame);
		}

		// Normalize frame weights
		r32 weightSum = 0.0f;
		for (i = 0; i < sprite->numFrames; i++)
			weightSum += sprite->frames[i].weight;

		const r32 invWeightSum = 1.0f / weightSum;
		for (i = 0; i < sprite->numFrames; i++)
			sprite->frames[i].weight *= invWeightSum;
	}

	sprite->name = spriteName;
	sprite->tint = ColorU((u32)KS_GetKeyInt(ks, spriteRef, "tint", (IntValue)0xFFFFFFFF));
	printf("Tint: %02x %02x %02x %02x\n", sprite->tint.r, sprite->tint.g, sprite->tint.b, sprite->tint.a);
	sprite->atlas = atlas;

	return sprite;
}

void Spr_ReadAtlasFromKeyStore(const KeyStore *ks, ValueRef avr, SpriteAtlas *atlas)
{
	Assert(atlas);
	atlas->name = KS_GetKeySymbol(ks, avr, "name", ST_Intern(KS_GetStringTable(), "(unnamed)"));
	strncpy(atlas->imageFile, KS_GetKeyString(ks, avr, "imageFile"), sizeof(atlas->imageFile));

	printf("Reading atlas %s from %s\n", ST_ToString(KS_GetStringTable(), atlas->name), atlas->imageFile);
	atlas->bitmap = Bm_MakeBitmapFromFile(nullptr, &g_game->spriteArena, atlas->imageFile);

	ValueRef spriteArr  = KS_ObjectGetValue(ks, avr, "sprites");
	u32      numSprites = KS_ArrayCount(ks, spriteArr);
	atlas->baseSize   = KS_GetKeySmallInt2(ks, avr, "baseSize", iv2(32, 32));
	atlas->baseOrigin = KS_GetKeySmallInt2(ks, avr, "baseOrigin", iv2(0, 0));

	for (atlas->numSprites = 0; atlas->numSprites < numSprites; atlas->numSprites++)
	{
		ValueRef spriteRef = KS_ArrayElem(ks, spriteArr, atlas->numSprites);
		atlas->sprites[atlas->numSprites] = LoadSprite(ks, atlas, spriteRef);
	}
}

internal void LoadSprites()
{
	if (g_game->numAtlases)
	{
		for (i32 i = 0; i < g_game->numAtlases; i++)
		{
			gHwi->UnregisterBitmap(g_game->atlases[i].bitmap);
		}
		g_game->numAtlases = 0;

		MA_Reset(&g_game->spriteArena);
	}

	KeyStore *atlasKs = nullptr;
	VerifyLoad(QED_LoadFile(&atlasKs, "sprites", "qed/sprites/tiledefs.qed"));

	Assert(atlasKs);

	ValueRef root       = KS_Root(atlasKs);
	root = KS_ObjectGetValue(atlasKs, root, "atlases");
	g_game->numAtlases = KS_ArrayCount(atlasKs, root);
	Assert(g_game->numAtlases < MAX_SPRITE_ATLASES);

	for (u32 ai = 0; ai < g_game->numAtlases; ++ai)
	{
		ValueRef avr = KS_ArrayElem(atlasKs, root, ai);
		Spr_ReadAtlasFromKeyStore(atlasKs, avr, &g_game->atlases[ai]);
	}

	KS_Free(&atlasKs);
}

internal void InitGameGlobals(const SubSystem *sys, bool isReInit)
{
	Assert(sys->globalPtr);

	if (isReInit)
		return;

	MA_Init(&g_game->tileArena, g_game->memory, MB(32));
	MA_Init(&g_game->spriteArena, g_game->memory, MB(32));
	MA_Init(&g_game->assetArena, g_game->memory, MB(512));

	Bm_CreateBitmap(&g_game->assetArena, &g_game->testBitmap, 128, 128, Bitmap::Format::RGBA8, 0);
	for (u32 y = 0; y < 128; y++)
	{
		for (u32 x = 0; x < 128; x++)
		{
			ColorU color(x * 2, y * 2, 0, 255);
			g_game->testBitmap.pixels[y * 128 + x] = (u32)color;
		}
	}
	gHwi->RegisterBitmap(&g_game->testBitmap);
	gHwi->UploadBitmap(&g_game->testBitmap);

	TestKeyStore();

	LoadSprites();

	g_game->playerPos.x.tile = 10;
	g_game->playerPos.y.tile = 10;

	for (i32 scrY = -32; scrY < 32; scrY++)
	{
		for (i32 scrX = -32; scrX < 32; scrX++)
		{
			for (i32 tileY = 0; tileY < ROOM_HGT; tileY++)
			{
				for (i32 tileX = 0; tileX < ROOM_WID; tileX++)
				{
					u32 value = TILE_EMPTY;
					if ((tileX == 0 || tileX == (ROOM_WID - 1) || tileY == 0 || tileY == (ROOM_HGT - 1)) && ((tileX != ROOM_WID / 2) && (tileY != ROOM_HGT / 2)))
						value = TILE_FULL;
					WorldPos_s worldPos = {};
					worldPos.x.tile     = scrX * ROOM_WID + tileX;
					worldPos.y.tile     = scrY * ROOM_HGT + tileY;
					SetTileValue(&g_game->tileArena, &g_game->world, &worldPos, value);
				}
			}

			for (i32 junk = 0; junk < 10; junk++)
			{
				i32 rxo = (rand() % (ROOM_WID - 2)) + 1;
				i32 ryo = (rand() % (ROOM_HGT - 2)) + 1;

				WorldPos_s worldPos = {};
				worldPos.x.tile     = scrX * ROOM_WID + rxo;
				worldPos.y.tile     = scrY * ROOM_HGT + ryo;
				SetTileValue(&g_game->tileArena, &g_game->world, &worldPos, TILE_FULL);
			}
		}
	}

	Bm_ReadBitmap(nullptr, &g_game->tileArena, &g_game->testBitmaps[0], "test/test_scene_layer_00.bmp");
	Bm_ReadBitmap(nullptr, &g_game->tileArena, &g_game->testBitmaps[1], "test/test_scene_layer_01.bmp");
	Bm_ReadBitmap(nullptr, &g_game->tileArena, &g_game->testBitmaps[2], "test/test_scene_layer_02.bmp");
	Bm_ReadBitmap(nullptr, &g_game->tileArena, &g_game->testBitmaps[3], "test/test_scene_layer_03.bmp");
	Bm_ReadBitmap(nullptr, &g_game->tileArena, &g_game->testBitmaps[4], "test/test_scene_layer_04.bmp", true);

	Bm_ReadBitmap(nullptr, &g_game->tileArena, &g_game->playerBmps[0][0], "test/test_hero_back_head.bmp");
	Bm_ReadBitmap(nullptr, &g_game->tileArena, &g_game->playerBmps[0][1], "test/test_hero_back_cape.bmp");
	Bm_ReadBitmap(nullptr, &g_game->tileArena, &g_game->playerBmps[0][2], "test/test_hero_back_torso.bmp");

	Bm_ReadBitmap(nullptr, &g_game->tileArena, &g_game->playerBmps[1][0], "test/test_hero_right_head.bmp");
	Bm_ReadBitmap(nullptr, &g_game->tileArena, &g_game->playerBmps[1][1], "test/test_hero_right_cape.bmp");
	Bm_ReadBitmap(nullptr, &g_game->tileArena, &g_game->playerBmps[1][2], "test/test_hero_right_torso.bmp");

	Bm_ReadBitmap(nullptr, &g_game->tileArena, &g_game->playerBmps[2][0], "test/test_hero_front_head.bmp");
	Bm_ReadBitmap(nullptr, &g_game->tileArena, &g_game->playerBmps[2][1], "test/test_hero_front_cape.bmp");
	Bm_ReadBitmap(nullptr, &g_game->tileArena, &g_game->playerBmps[2][2], "test/test_hero_front_torso.bmp");

	Bm_ReadBitmap(nullptr, &g_game->tileArena, &g_game->playerBmps[3][0], "test/test_hero_left_head.bmp");
	Bm_ReadBitmap(nullptr, &g_game->tileArena, &g_game->playerBmps[3][1], "test/test_hero_left_cape.bmp");
	Bm_ReadBitmap(nullptr, &g_game->tileArena, &g_game->playerBmps[3][2], "test/test_hero_left_torso.bmp");
}

static u32 RoundReal(r32 val)
{
	return (u32)(val + 0.5f);
}

static i32 RoundIReal(r32 val)
{
	return (i32)(val + 0.5f);
}

static u32 PackColor(r32 r, r32 g, r32 b)
{
	return (RoundReal(r * 255.0f) << 16u) | (RoundReal(g * 255.0f) << 8u) | (RoundReal(b * 255.0f) << 0u);
}

internal void RenderRectangle(Bitmap *bitmap, i32 x0, i32 y0, i32 x1, i32 y1, r32 r, r32 g, r32 b)
{
#if 0
	const u32 color = PackColor(r, g, b);

	x0 = Qi_Clamp<i32>(x0, 0, bitmap->width);
	x1 = Qi_Clamp<i32>(x1, x0, bitmap->width);
	y0 = Qi_Clamp<i32>(y0, 0, bitmap->height);
	y1 = Qi_Clamp<i32>(y1, y0, bitmap->height);

	u32 *xel = bitmap->pixels + y0 * bitmap->pitch;
	for (i32 y = y0; y < y1; y++)
	{
		for (i32 x = x0; x < x1; x++)
			xel[x] = color;
		xel += bitmap->pitch;
	}
#else
	const Color color(r, g, b, 1.0f);
	Rect        rect;
	rect.left   = x0;
	rect.top    = y0;
	rect.width  = x1 - x0;
	rect.height = y1 - y0;

	if (bitmap != g_game->screenBitmap)
		gHwi->PushRenderTarget(bitmap);
	gHwi->FillRect(&rect, color);
	if (bitmap != g_game->screenBitmap)
		gHwi->PopRenderTarget();
#endif
}

internal void DrawRectangle(Bitmap *bitmap, r32 x, r32 y, r32 width, r32 height, r32 r, r32 g, r32 b)
{
	RenderRectangle(bitmap, RoundIReal(x), RoundIReal(y), RoundIReal(x + width), RoundIReal(y + height), r, g, b);
}

internal void ClipRectToBmp(const Bitmap *bmp, i32 *bx, i32 *by, i32 *bw, i32 *bh)
{
	i32 bx1 = *bx + *bw;
	i32 by1 = *by + *bh;

	if (*bx < 0)
		*bx = 0;

	if (*by < 0)
		*by = 0;

	if (bx1 < 0)
		bx1 = 0;

	if (bx1 > (i32)bmp->width)
		bx1 = (i32)bmp->width;

	if (by1 < 0)
		by1 = 0;

	if (by1 > (i32)bmp->height)
		by1 = (i32)bmp->height;

	*bw = bx1 - *bx;
	*bh = by1 - *by;
}

static inline void BlendPixel(u32 &dest, const u32 src)
{
	const u64 a = (src >> 24) + 1;

	const u64 destGRB = (((u64)dest & 0x00FF00) << 24) | (dest & 0xFF00FF);
	const u64 srcGRB  = (((u64)src & 0x00FF00) << 24) | (src & 0xFF00FF);

	u64 dGRB = srcGRB - destGRB;
	dGRB *= a;
	dGRB >>= 8;

	const u64 GRB = destGRB + dGRB;
	dest          = (u32)(GRB & 0xFF00FF) | (u32)((GRB & 0xFF00000000ull) >> 24);
}

internal void BltBmpFixed(ThreadContext *thread, Bitmap *dest, i32 dx, i32 dy, const Bitmap *src)
{
	i32 sx = 0, sy = 0;
	i32 sw = src->width, sh = src->height;

	// Clip to dest bmp
	if (dx < 0)
	{
		sx -= dx;
		sw += dx;
		dx = 0;
	}
	if (dy < 0)
	{
		sy -= dy;
		sy += dy;
		dy = 0;
	}
	if (dx + sw >= (i32)dest->width)
	{
		sw -= (dx + sw - dest->width);
	}
	if (dy + sh >= (i32)dest->height)
	{
		sh -= (dy + sh - dest->height);
	}
	Assert(sx >= 0 && sy >= 0 && sx + sw <= dest->width && sy + sh <= dest->height);

	for (i32 y = 0; y < sh; y++)
	{
		u32 *const srcXelRow  = src->pixels + (sy + y) * src->pitch;
		u32 *const destXelRow = dest->pixels + (dy + y) * dest->pitch;
		for (i32 x = 0; x < sw; x++)
			BlendPixel(destXelRow[x + dx], srcXelRow[x]);
	}
}

internal void BltBmpStretched(ThreadContext *thread, Bitmap *dest, r32 rdx, r32 rdy, r32 rdw, r32 rdh, Bitmap *src, r32 rsx, r32 rsy, r32 rsw, r32 rsh)
{
#if 0
	r32 rdx1 = ceil(rdx + rdw);
	r32 rdy1 = ceil(rdy + rdh);
	rdx      = floor(rdx);
	rdy      = floor(rdy);

	r32 rsx1 = ceil(rsx + rsw);
	r32 rsy1 = ceil(rsy + rsh);
	rsx      = floor(rsx);
	rsy      = floor(rsy);

	i32 dx = (i32)rdx;
	i32 dy = (i32)rdy;
	i32 dw = (i32)(rdx1 - rdx);
	i32 dh = (i32)(rdy1 - rdy);

	i32 sx = (i32)rsx;
	i32 sy = (i32)rsy;
	i32 sw = (i32)(rsx1 - rsx);
	i32 sh = (i32)(rsy1 - rsy);

	// Adjust for starting offset for clipped initial rects
	i32 sdx = (sw << 16) / dw;
	i32 sdy = (sh << 16) / dh;

	// Clip src rect to src bitmap
	ClipRectToBmp(src, &sx, &sy, &sw, &sh);

	i32 idx = dx, idy = dy;
	ClipRectToBmp(dest, &dx, &dy, &dw, &dh);

	if (dw == 0 || dh == 0 || sw == 0 || sh == 0)
		return;

	Assert(sx >= 0 && sy >= 0 && sw <= src->width && sh <= src->height);
	Assert(dx >= 0 && dy >= 0 && dw <= dest->width && dh <= dest->height);

	i32 curSy = sy << 16;
	curSy += sdy * (dy - idy);

	for (i32 y = 0; y < dh; y++, curSy += sdy)
	{
		i32 curSx = sx << 16;
		curSx += sdx * (dx - idx);

		u32 *srcXelRow  = src->pixels + (curSy >> 16) * src->pitch;
		u32 *destXelRow = dest->pixels + (dy + y) * dest->pitch;
		for (i32 x = 0; x < dw; x++, curSx += sdx)
			BlendPixel(destXelRow[x + dx], srcXelRow[curSx >> 16]);
	}
#else
	Rect srcRect, destRect;
	srcRect.left   = rsx;
	srcRect.top    = rsy;
	srcRect.width  = rsw;
	srcRect.height = rsh;

	destRect.left   = rdx;
	destRect.top    = rdy;
	destRect.width  = rdw;
	destRect.height = rdh;

	if (dest != g_game->screenBitmap)
		gHwi->PushRenderTarget(dest);
	gHwi->BlitStretched(src, &srcRect, &destRect);
	if (dest != g_game->screenBitmap)
		gHwi->PopRenderTarget();
#endif
}

internal void BltBmpStretchedFixed(ThreadContext *thread, Bitmap *dest, r32 rdx, r32 rdy, r32 rdw, r32 rdh, Bitmap *src)
{
	BltBmpStretched(thread, dest, rdx, rdy, rdw, rdh, src, 0, 0, src->width, src->height);
}

void Spr_DrawSpriteFrame(const Sprite *sprite, u32 frameIdx, const r32 dx, const r32 dy, const r32 wid, const r32 hgt)
{
	Assert(sprite);
	const Bitmap* atlasBmp = sprite->atlas->bitmap;
	Assert(frameIdx < sprite->numFrames);
	const SpriteFrame* frame = &sprite->frames[frameIdx];
	Rect dest;
	dest.left = dx;
	dest.top = dy;
	dest.width = wid;
	dest.height = hgt;
	gHwi->BlitStretchedUV((Bitmap *)atlasBmp, frame->topLeftUV, frame->bottomRightUV, &dest, sprite->tint);
}

void Qi_GameUpdateAndRender(ThreadContext *, Input *input, Bitmap *screenBitmap)
{
	static NoiseGenerator noise(1234);

	g_game->screenBitmap = screenBitmap;
	g_game->screenWid    = screenBitmap->width;
	g_game->screenHgt    = screenBitmap->height;

	Assert(g_game && g_game->isInitialized);
	UpdateGameState(screenBitmap, input);

	// Clear screen
	// DrawRectangle(screenBitmap, 0.0f, 0.0f, (r32)screenBitmap->width, (r32)screenBitmap->height, 0.0f, 0.0f, 1.0f);

	const r32 tilePixelWid = TILE_RES_X / SCREEN_SCALE;
	const r32 tilePixelHgt = TILE_RES_Y / SCREEN_SCALE;

	const i32 numScreenTilesX = (i32)(screenBitmap->width / tilePixelWid + 0.5f);
	const i32 numScreenTilesY = (i32)(screenBitmap->height / tilePixelHgt + 0.5f);

	const r32 cameraOffsetPixelsX = g_game->cameraPos.x.offset * tilePixelWid;
	const r32 cameraOffsetPixelsY = g_game->cameraPos.y.offset * tilePixelHgt;

	static bool debugOpen = false;
	static bool drawBG    = true;

	if (g_game->enableEditor)
	{
		Editor_UpdateAndRender();
	}

#if 1
	if (drawBG)
		for (i32 i = 4; i >= 0; i--)
			BltBmpStretchedFixed(nullptr, screenBitmap, 0, 0, screenBitmap->width, screenBitmap->height, &g_game->testBitmaps[i]);
	BltBmpStretchedFixed(nullptr, screenBitmap, 0, 0, 128, 128, &g_game->testBitmap);
	Rect dest = { screenBitmap->width - 128.0f, screenBitmap->height - 128.0f, 128.0f, 128.0f };
	gHwi->BlitStretchedUV(&g_game->testBitmap, v2(0.0f, 0.0f), v2(1.0f, 1.0f), &dest);

#elif 0
	for (i32 j = 0; j < (i32)screenBitmap->height; j++)
	{
		for (i32 i = 0; i < (i32)screenBitmap->width; i++)
		{
			// const r32 col = noise.Perlin2D(Vector2(i, j), 500.0f) * 0.5f + 0.5f;
			// const r32 col = noise.Smoothed2D(Vector2(i, j), 50.0f) * 0.5f + 0.5f;
			const r32 col = noise.Simplex2D(Vector2(i, j), 10.0f) * 0.5f + 0.5f;
			// const r32 col = noise.Smoothed(i, 100.0f) * 0.5f + 0.5f;
			// const r32 col = noise.GetReal(i);
			// printf("%f\n", col);
			PlotPt(screenBitmap, i, j, PackColor(col, col, col));
		}
	}
#else
	RandomGenerator rnd(1);
	SpriteAtlas* atlas = &g_game->atlases[0];

	for (i32 row = -1; row < numScreenTilesY + 1; row++)
	{
		for (i32 col = -1; col < numScreenTilesX + 1; col++)
		{
			const r32 sx        = col * tilePixelWid - tilePixelWid / 2 - cameraOffsetPixelsX;
			const r32 sy        = row * tilePixelHgt - tilePixelHgt / 2 - cameraOffsetPixelsY;
			u32       spriteIdx = rnd.RandomBelow(atlas->numSprites);
			Sprite *  sprite    = atlas->sprites[spriteIdx];
			Spr_DrawSpriteFrame(sprite, rnd.RandomBelow(sprite->numFrames), sx, sy, tilePixelWid, tilePixelHgt);
		}
	}
#endif

	for (i32 row = -1; row < numScreenTilesY + 1; row++)
	{
		for (i32 col = -1; col < numScreenTilesX + 1; col++)
		{
			const r32 sx = col * tilePixelWid - tilePixelWid / 2 - cameraOffsetPixelsX;
			const r32 sy = row * tilePixelHgt - tilePixelHgt / 2 - cameraOffsetPixelsY;

			WorldPos_s tileCoord = {};
			tileCoord.x.tile     = col + g_game->cameraPos.x.tile - numScreenTilesX / 2;
			tileCoord.y.tile     = row + g_game->cameraPos.y.tile - numScreenTilesY / 2;

			u32 tileValue = GetTileValue(&g_game->world, &tileCoord);
#if 0
			if (tileCoord.x.tile == g_game->playerPos.x.tile && tileCoord.y.tile == g_game->playerPos.y.tile)
				DrawRectangle(screenBitmap, sx, sy, tilePixelWid, tilePixelHgt, 0.1f, 0.1f, 0.1f);
			else if (tileValue == TILE_INVALID)
				DrawRectangle(screenBitmap, sx, sy, tilePixelWid, tilePixelHgt, 1.0f, 0.2f, 0.2f);
			else if (tileValue == TILE_EMPTY)
				DrawRectangle(screenBitmap, sx, sy, tilePixelWid, tilePixelHgt, 0.3f, 0.3f, 0.3f);
			else
				DrawRectangle(screenBitmap, sx, sy, tilePixelWid, tilePixelHgt, .8f, .8f, .8f);
#else
			if (tileValue == TILE_INVALID)
				DrawRectangle(screenBitmap, sx, sy, tilePixelWid, tilePixelHgt, 1.0f, 0.2f, 0.2f);
			else if (tileValue != TILE_EMPTY)
				BltBmpStretched(
					nullptr, screenBitmap, sx, sy, tilePixelWid, tilePixelHgt, &g_game->testBitmaps[1], 0, 0, g_game->testBitmaps[1].width, g_game->testBitmaps[1].height);
#endif
		}
	}

	WorldPos_s playerCameraDelta = {};
	WorldPosSub(&playerCameraDelta, &g_game->playerPos, &g_game->cameraPos);

	const r32 playerOffsetPixelsX = (playerCameraDelta.x.tile + playerCameraDelta.x.offset) * tilePixelWid;
	const r32 playerOffsetPixelsY = (playerCameraDelta.y.tile + playerCameraDelta.y.offset) * tilePixelHgt;

	r32 playerR   = 0.5f;
	r32 playerG   = 1.0f;
	r32 playerB   = 0.0f;

	r32 playerPosX = playerOffsetPixelsX + screenBitmap->width / 2.0f;
	r32 playerPosY = playerOffsetPixelsY + screenBitmap->height / 2.0f;
	r32 playerX   = playerPosX - tilePixelWid / 2;
	r32 playerY   = (playerPosY - tilePixelHgt) + (tilePixelHgt * 0.25f);
	r32 playerWid = tilePixelWid;
	r32 playerHgt = tilePixelHgt;

	DrawRectangle(screenBitmap, playerX, playerY, playerWid, playerHgt, playerR, playerG, playerB);
	BltBmpStretchedFixed(nullptr, screenBitmap, playerX, playerY, playerWid, playerHgt, &g_game->playerBmps[g_game->playerFacingIdx][0]);
	BltBmpStretchedFixed(nullptr, screenBitmap, playerX, playerY, playerWid, playerHgt, &g_game->playerBmps[g_game->playerFacingIdx][1]);
	BltBmpStretchedFixed(nullptr, screenBitmap, playerX, playerY, playerWid, playerHgt, &g_game->playerBmps[g_game->playerFacingIdx][2]);
	DrawRectangle(screenBitmap, playerPosX - 2, playerPosY - 2, 4, 4, 0.0f, 0.0f, 1.0f);

	DrawDebugShapes(screenBitmap);
}

Hwi *Qi_GetHwi()
{
	return gHwi;
}

extern SubSystem SoundSubSystem;
extern SubSystem KeyStoreSubsystem;
extern SubSystem DebugSubSystem;
extern SubSystem UtilSubSystem;
extern SubSystem HardwareSubSystem;
extern SubSystem EditorSubSystem;

SubSystem GameSubSystem = {"Game", InitGameGlobals, sizeof(GameGlobals_s), nullptr};

internal SubSystem *s_subSystems[] = {
#if !HAS(RELEASE_BUILD)
	&DebugSubSystem,
#endif
	&HardwareSubSystem,
	&KeyStoreSubsystem,
	&SoundSubSystem,
	&UtilSubSystem,
	&GameSubSystem,
	&EditorSubSystem,
};

internal void InitGameSystems(Memory *memory)
{
	NoiseGenerator::InitGradients();

	g_game         = (GameGlobals_s *)memory->permanentStorage;
	g_game->memory = memory;

	bool isReload = g_game->isInitialized;
	if (!isReload)
	{
		void *initMemPtr = M_AllocRaw(memory, sizeof(GameGlobals_s));
		Assert(initMemPtr == (void *)g_game);
	}

	for (i32 i = 0; i < countof(s_subSystems); i++)
	{
		SubSystem *sys       = &g_game->gameSubsystems[i];
		void *     curMemPtr = sys->globalPtr;
		*sys                 = *s_subSystems[i];

		if (curMemPtr == nullptr)
			sys->globalPtr = M_AllocRaw(memory, sys->globalSize);
		else
			sys->globalPtr = curMemPtr;

		sys->initFunc(sys, isReload);
	}

	if (!g_game->isInitialized)
	{
		g_game->isInitialized = true;
	}
}

const PlatFuncs_s *plat;
void               Qi_Init(const PlatFuncs_s *platFuncs, Memory *memory)
{
	plat = platFuncs;
	plat->SetupMainExeLibraries();
	ImGui::SetCurrentContext(plat->GetGuiContext());

	if (g_game == nullptr)
	{
		InitGameSystems(memory);
		Assert(g_game && g_game->isInitialized);
	}
}

void Qi_ToggleEditor()
{
	g_game->enableEditor = !g_game->enableEditor;
}

internal GameFuncs_s s_game = {
	sound,
#if HAS(DEV_BUILD)
	debug,
#endif
	Qi_Init,
	Qi_GameUpdateAndRender,
	Qi_GetHwi,
	Qi_ToggleEditor,
};
const GameFuncs_s *game = &s_game;

// Interface to platform code
extern "C"
{
	PLAT_EXPORT const GameFuncs_s *Qi_GetGameFuncs() { return game; }
}
