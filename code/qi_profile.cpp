//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Profiler drawing and helper functions
//

#include "has.h"
#include "basictypes.h"

#include "qi.h"
#include "qi_math.h"
#include "qi_profile.h"

#define BORDER_SIZE 10

void
QiProf_DrawProfileBox(Bitmap_s* bitmap, const u32 x, const u32 y, const u32 wid, const u32 hgt, const u32 color)
{
	Assert(x + wid < bitmap->width && y + hgt < bitmap->height);
	u32* curPtr = bitmap->pixels + x + y * bitmap->pitch;
	for (u32 y0 = 0; y0 < hgt; y0++)
	{
		for (u32 x0 = 0; x0 < wid; x0++)
		{
			curPtr[x0] = color;
		}
		curPtr += bitmap->pitch;
	}
}

void
QiProf_DrawTicks(Bitmap_s*  bitmap,
                 const u32* tickBuf,
                 const u32  tickCount,
                 const u32  largestTickValue,
                 const u32  yOff,
                 const u32  ySize,
                 const u32  color)
{
	r32 widthScale = bitmap->width - 2.0f * BORDER_SIZE;
	r32 tickScale  = widthScale / largestTickValue;
	for (u32 tick = 0; tick < tickCount; tick++)
	{
        u32 tickPos = (u32)(tickBuf[tick] * tickScale + 0.5f) + BORDER_SIZE;
		QiProf_DrawProfileBox(bitmap, tickPos, yOff + BORDER_SIZE, 1, ySize, color);
	}
}

void
QiProf_DrawBars(Bitmap_s*  bitmap,
                const u32* widthBuf,
                const u32  widthCount,
                const u32  maxWidth,
                const u32  yOff,
                const u32  height,
                const u32* colors,
                const u32  colorCount)
{
	r32 widthScale  = bitmap->width - 2.0f * BORDER_SIZE;
	r32 barScale    = widthScale / maxWidth;
	u32 xPos        = BORDER_SIZE;
	u32 rightBorder = bitmap->width - BORDER_SIZE;
	for (u32 bar = 0; bar < widthCount; bar++)
	{
		u32       barWidth = (u32)(widthBuf[bar] * barScale);
		const u32 x        = Qi_Clamp(xPos, 0u, rightBorder);
		const u32 xr       = Qi_Clamp<u32>(xPos + barWidth, 0u, rightBorder);
        QiProf_DrawProfileBox(bitmap, x, yOff, xr - x, height, colors[bar % colorCount]);
        xPos += barWidth;
	}
}
