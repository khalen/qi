#ifndef __QI_OGL_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Header file for platform independent OGL code
//

#include "game.h"
#include "bitmap.h"
#include <glad/glad.h>

class ImDrawData;

void QiOgl_Init();
void QiOgl_Clear();
void QiOgl_DrawImGui(ImDrawData* drawData);
void QiOgl_SetScreenBitmap(Bitmap* bitmap);
void QiOgl_BeginFrame();
void QiOgl_EndFrame();

#define __QI_OGL_H
#endif // #ifndef __QI_OGL_H
