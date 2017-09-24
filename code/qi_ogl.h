#ifndef __QI_OGL_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Header file for platform independent OGL code
//

#include "qi.h"
#include <glad/glad.h>

void QiOgl_Init();
void QiOgl_BitmapToTexture(GLuint tex, const Bitmap_s* bitmap);
void QiOgl_Clear();
void QiOgl_BlitBufferToScreen(const Bitmap_s* bitmap);

#define __QI_OGL_H
#endif // #ifndef __QI_OGL_H
