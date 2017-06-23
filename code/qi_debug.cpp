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

#include "qi_profile.cpp"

internal DebugFuncs_s s_debug =
{
    Qid_DrawBars,
    Qid_DrawProfileBox,
    Qid_DrawTicks,
};
const DebugFuncs_s* debug = &s_debug;
#endif
