//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Debug functionality
//

#include "has.h"

#if HAS(DEV_BUILD) || HAS(PROF_BUILD)
#include "basictypes.h"

#include "qi.h"
#include "qi_debug.h"

#include <string.h>

#include "qi_profile.cpp"

struct DebugGlobals_s
{
    u64 pad0;
};

internal DebugGlobals_s* g_debug;

void Qi_Assert_Handler(const char* msg, const char* file, const int line)
{
    __debugbreak();
}

static void QiDebug_Init(const SubSystem_s* sys, bool);

SubSystem_s DebugSubSystem = {
    "Debug",
    QiDebug_Init,
    sizeof(DebugGlobals_s),
    nullptr
};

internal void
QiDebug_Init(const SubSystem_s* sys, bool isReinit)
{
    Assert(sys->globalPtr);
    g_debug = (DebugGlobals_s*)sys->globalPtr;
    if(!isReinit)
        memset(g_debug, 0, sizeof(*g_debug));
}

internal DebugFuncs_s s_debug =
{
    Qid_DrawBars,
    Qid_DrawProfileBox,
    Qid_DrawTicks,
};
const DebugFuncs_s* debug = &s_debug;
#endif
