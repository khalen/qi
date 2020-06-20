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

struct Hwi
{
	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;

	virtual void RegisterBitmap(Bitmap *bitmap, bool canBeRenderTarget = false) = 0;
	virtual void UnregisterBitmap(Bitmap *bitmap) = 0;
	virtual void UploadBitmap(Bitmap *bitmap) = 0;

	virtual void SetScreenTarget(Bitmap *screenTarget) = 0;
	virtual void PushRenderTarget(Bitmap *targetBitmap) = 0;
	virtual void PopRenderTarget() = 0;

	virtual void BlitStretchedUV(Bitmap *srcBitmap, v2 bluv, v2 truv, const Rect *destRectPixels, ColorU tint = ColorU(255, 255, 255, 255)) = 0;
	virtual void BlitStretched(Bitmap *srcBitmap, const Rect *srcRectPixels, const Rect *destRectPixels, ColorU tint = ColorU(255, 255, 255, 255)) = 0;
	virtual void Blit(Bitmap *srcBitmap, v2 srcXY, v2 destXY, v2 size, ColorU tint = ColorU(255, 255, 255, 255)) = 0;

	virtual void PushClipRect(Rect *clipRect) = 0;
	virtual void PopClipRect() = 0;

	virtual void PushBlendState(BlendState blend) = 0;
	virtual void PopBlendState() = 0;

	virtual void DrawLine(v2 p0, v2 p1, ColorU color = ColorU(255, 255, 255, 255)) = 0;
	virtual void DrawRect(const Rect *rect, ColorU color = ColorU(255, 255, 255, 255)) = 0;
	virtual void FillRect(const Rect *rect, ColorU color = ColorU(255, 255, 255, 255)) = 0;
	virtual void DrawBezier(v2 a, v2 b, v2 c, v2 d, ColorU color = ColorU(255, 255, 255, 255)) = 0;

	virtual void ResetState() = 0;

	virtual void Finalize() = 0;
};

extern Hwi* gHwi;

#endif // HWI_H
