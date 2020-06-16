//
// Created by kash9 on 6/10/2020.
//

#include "bitmap.h"
#include "util.h"
#include "hwi.h"

void Bm_CreateBitmap(MemoryArena *arena, Bitmap *result, const u32 width, const u32 height, Bitmap::Format format, u32 flags)
{
	Assert(arena && result);
	result->width    = width;
	result->height   = height;
	result->pitch    = width;
	result->byteSize = width * height * sizeof(u32);
	result->pixels   = (u32 *)MA_Alloc(arena, result->byteSize);
}

BEGIN_PACKED_DEFS

struct BmpFileHeader_s
{
	u16 type;
	u32 fileSize;
	u16 __reserved1;
	u16 __reserved2;
	u32 pixelDataOffset;
} PACKED;

struct BmpImageHeader_s
{
	u32 size;
	u32 width;
	u32 height;
	u16 planes;
	u16 bpp;
	u32 compression;
	u32 imageSize;
	u32 hResolution;
	u32 vResolution;
	u32 colors;
	u32 importantColors;
	u32 redMask;
	u32 greenMask;
	u32 blueMask;
} PACKED;

END_PACKED_DEFS

void Bm_ReadBitmap(ThreadContext *thread, MemoryArena *memArena, Bitmap *result, const char *filename)
{
	size_t readSize;
	u8 *   fileData = (u8 *)plat->ReadEntireFile(thread, filename, &readSize);
	Assert(fileData);

	const BmpFileHeader_s * fileHeader = (BmpFileHeader_s *)fileData;
	const BmpImageHeader_s *imgHeader  = (BmpImageHeader_s *)(fileData + sizeof(BmpFileHeader_s));
	u32 *                   srcXels    = nullptr;
	u32 *                   dstXels    = nullptr;
	u32                     rMask, gMask, bMask, aMask;
	u32                     rShift, gShift, bShift, aShift;

	if (fileHeader->type != 0x4D42) // 'BM' in ASCII
	{
		printf("Bad bmp type: 0x%x\n", fileHeader->type);
		goto end;
	}
	if (imgHeader->size < 40)
	{
		printf("Unexpected bmp header size: %d\n", imgHeader->size);
		goto end;
	}

	Bm_CreateBitmap(memArena, result, imgHeader->width, imgHeader->height);
	srcXels = (u32 *)(fileData + fileHeader->pixelDataOffset);
	dstXels = result->pixels;

	if (imgHeader->compression == 3)
	{
		rMask = imgHeader->redMask;
		gMask = imgHeader->greenMask;
		bMask = imgHeader->blueMask;
	}
	else
	{
		rMask = 0xFF0000;
		gMask = 0x00FF00;
		bMask = 0x0000FF;
	}

	aMask = ~(rMask | gMask | bMask);

	rShift = ShiftFromMask(rMask);
	gShift = ShiftFromMask(gMask);
	bShift = ShiftFromMask(bMask);
	aShift = ShiftFromMask(aMask);

	for (u32 idx = 0; idx < result->byteSize / sizeof(u32); idx++)
	{
		u32 src = srcXels[idx];
		u32 r   = (src & rMask) >> rShift;
		u32 g   = (src & gMask) >> gShift;
		u32 b   = (src & bMask) >> bShift;
		u32 a   = (src & aMask) >> aShift;

		dstXels[idx] = (a << 24) | (r << 16) | (g << 8) | b;
	}

	printf("Read %s: %d x %d\n", filename, result->width, result->height);
	gHwi->RegisterBitmap(result, false);
	gHwi->UploadBitmap(result);

end:
	plat->ReleaseFileBuffer(thread, fileData);
}
