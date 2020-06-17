//
// Created by kash9 on 6/10/2020.
//

#include "bitmap.h"
#include "util.h"
#include "debug.h"
#include "hwi.h"

#define STBI_ASSERT Assert
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void Bm_CreateBitmap(MemoryArena *arena, Bitmap *result, const u32 width, const u32 height, Bitmap::Format format, u32 flags)
{
	Assert(arena && result);
	result->width    = width;
	result->height   = height;
	result->pitch    = width;
	result->byteSize = width * height * sizeof(u32);
	result->pixels   = (u32 *)MA_Alloc(arena, result->byteSize);
}

void Bm_ReadBitmap(ThreadContext *thread, MemoryArena *memArena, Bitmap *result, const char *filename, bool forceOpaque)
{
	int wid, hgt, components;
	stbi_set_flip_vertically_on_load(true);
	u8* data = stbi_load(filename, &wid, &hgt, &components, 4);

	Bm_CreateBitmap(memArena, result, wid, hgt);
	u8* srcXels = (u8 *)data;
	u32* dstXels = result->pixels;

	for (u32 idx = 0; idx < result->byteSize / sizeof(u32); idx++)
	{
		u32 r   = srcXels[0];
		u32 g   = srcXels[1];
		u32 b   = srcXels[2];
		u32 a   = forceOpaque ? 0xFF : srcXels[3];

		dstXels[idx] = (a << 24) | (b << 16) | (g << 8) | r;
		srcXels += 4;
	}

	printf("Read %s: %d x %d\n", filename, result->width, result->height);
	gHwi->RegisterBitmap(result, false);
	gHwi->UploadBitmap(result);

	free(data);
}

