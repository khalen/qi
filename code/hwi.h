//
// Created by Jonathan Davis on 6/13/20.
//

#ifndef HWI_H
#define HWI_H

#include "math_util.h"

struct Bitmap;
struct Rect;
struct Brush;

enum BlendState
{
	BSTATE_None,
	BSTATE_SrcAlpha_OneMinusDstAlpha,
	BSTATE_Src_DstAlpha,
};

void Hwi_RegisterBitmap(Bitmap *bitmap, bool canBeRenderTarget = false);
void Hwi_UnregisterBitmap(Bitmap *bitmap);

void Hwi_SetScreenTarget(Bitmap *screenTarget);
void Hwi_PushRenderTarget(Bitmap *targetBitmap);
void Hwi_PopRenderTarget();

void Hwi_BlitStretched(Bitmap *srcBitmap, const Rect *srcRectPixels, const Rect *destRectPixels, ColorU tint = ColorU(255, 255, 255, 255));
void Hwi_Blit(Bitmap* srcBitmap, v2 srcXY, v2 destXY, v2 size, ColorU tint = ColorU(255, 255, 255, 255));

void Hwi_PushClipRect(Rect* clipRect);
void Hwi_PopClipRect();

void Hwi_PushBlendState(BlendState blend);
void Hwi_PopBlendState();

void Hwi_DrawLine(v2 p0, v2 p1, ColorU color = ColorU(255, 255, 255, 255));
void Hwi_DrawRect(const Rect* rect, ColorU color = ColorU(255, 255, 255, 255));
void Hwi_DrawBezier(v2 a, v2 b, v2 c, v2 d, ColorU color = ColorU(255, 255, 255, 255));

void Hwi_ResetState();

#endif // HWI_H
