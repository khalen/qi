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

void Hwi_BlitStretched(Bitmap *srcBitmap, const Rect *srcRectPixels, const Rect *destRectPixels, Color_s tint);
void Hwi_Blit(Bitmap* srcBitmap, v2 srcXY, v2 destXY, v2 size);

void Hwi_PushBrush(Brush* brush);
void Hwi_PushColorBrush(Color_s color);
void Hwi_PopBrush(Brush *curBrush);

void Hwi_PushClipRect(Rect* clipRect);
void Hwi_PopClipRect();

void Hwi_MoveTo(v2 coord);
void Hwi_LineTo(v2 coord);
void Hwi_Rect(Rect* rect);
void Hwi_Bezier(v2 a, v2 b, v2 c, v2 d);

#endif // HWI_H
