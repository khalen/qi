//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Debug functionality
//

#include "has.h"

#include "game.h"
#include "debug.h"
#include "profile.h"

#include <string.h>

#if HAS(DEV_BUILD) || HAS(PROF_BUILD)

struct DebugGlobals_s
{
	u64 pad0;
};

internal DebugGlobals_s* g_debug;

void
Qi_Assert_Handler(const char* msg, const char* file, const int line)
{
#if HAS(OSX_BUILD)
	asm("int $3");
#else
	__debugbreak();
#endif
}

static void QiDebug_Init(const SubSystem* sys, bool);

SubSystem DebugSubSystem = {"Debug", QiDebug_Init, sizeof(DebugGlobals_s), nullptr};

internal void
QiDebug_Init(const SubSystem* sys, bool isReinit)
{
	Assert(sys->globalPtr);
	g_debug = (DebugGlobals_s*)sys->globalPtr;
	if (!isReinit)
		memset(g_debug, 0, sizeof(*g_debug));
}

internal DebugFuncs_s s_debug = {
#if HAS(PROF_BUILD)
    Qid_DrawBars,
    Qid_DrawProfileBox,
    Qid_DrawTicks,
#endif
};
const DebugFuncs_s* debug = &s_debug;
#endif
