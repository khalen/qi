//
// Created by kash9 on 6/10/2020.
//

#ifndef QI_BITMAP_H
#define QI_BITMAP_H

#include "basictypes.h"
#include "debug.h"
#include "game.h"
#include "memory.h"
#include <cstdio>

struct Bitmap
{
	enum Flags
	{
		RenderTarget = 1,
	};

	enum Format
	{
		RGBA8 = 0,
		RGBA16,
		R8,
		RG8,
		RGB8,
	};

	enum FormatPixelBytes
	{
		PS_RGBA8  = 4,
		PS_RGBA16 = 8,
		PS_R8     = 1,
		PS_RG8    = 2,
		PS_RGB8   = 3,
	};

	// Always 32 bit, xel order is BB GG RR 00 (LE)
	u32 *pixels;

	// Dimensions are in pixels, not bytes
	u32    width;
	u32    height;
	u32    pitch;
	u32    byteSize;
	u32    flags;
	Format format;

	void *hardwareId;
};

struct ThreadContext;
struct MemoryArena;

void Bm_ReadBitmap(ThreadContext *thread, MemoryArena *memArena, Bitmap *result, const char *filename);
void Bm_CreateBitmap(MemoryArena *arena, Bitmap *result, const u32 width, const u32 height, Bitmap::Format format = Bitmap::Format::RGBA8, u32 flags = 0);

#endif // QI_BITMAP_H
