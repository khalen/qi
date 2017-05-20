#ifndef __QI_PROFILE_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// General low level profiling tools
//

#include "has.h"
#include "basictypes.h"

#if HAS(PROF_BUILD) || HAS(DEV_BUILD)
struct Bitmap_s;

void QiProf_DrawTicks(Bitmap_s*  screen,
                      const u32* tickBuf,
                      const u32  tickCount,
                      const u32  largestTickValue,
                      const u32  yOff,
                      const u32  ySize,
                      const u32  color);

void QiProf_DrawBars(Bitmap_s*  screen,
                     const u32* widthBuf,
                     const u32  widthCount,
                     const u32  maxWidth,
                     const u32  yOff,
                     const u32  height,
                     const u32* colors,
                     const u32  colorCount);

void QiProf_DrawProfileBox(Bitmap_s* screen, const u32 x, const u32 y, const u32 wid, const u32 hgt);
#endif

#define __QI_PROFILE_H
#endif // #ifndef __QI_PROFILE_H
