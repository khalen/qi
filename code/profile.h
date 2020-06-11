#ifndef __QI_PROFILE_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// General low level profiling tools
//

#include "has.h"
#include "basictypes.h"
#include "bitmap.h"

#if HAS(PROF_BUILD) || HAS(DEV_BUILD)
struct Bitmap;

struct ThreadContext;

typedef void Qid_DrawTicks_f(ThreadContext* tc,
                             Bitmap*        screen,
                             const u32*       tickBuf,
                             const u32        tickCount,
                             const u32        largestTickValue,
                             const u32        yOff,
                             const u32        ySize,
                             const u32        color);

typedef void Qid_DrawBars_f(ThreadContext* tc,
                            Bitmap*        screen,
                            const u32*       widthBuf,
                            const u32        widthCount,
                            const u32        maxWidth,
                            const u32        yOff,
                            const u32        height,
                            const u32*       colors,
                            const u32        colorCount);

typedef void
Qid_DrawProfileBox_f(
    ThreadContext* tc, Bitmap* screen, const u32 x, const u32 y, const u32 wid, const u32 hgt, const u32 color);

extern Qid_DrawTicks_f Qid_DrawTicks;
extern Qid_DrawBars_f Qid_DrawBars;
extern Qid_DrawProfileBox_f Qid_DrawProfileBox;
#endif

#define __QI_PROFILE_H
#endif // #ifndef __QI_PROFILE_H
