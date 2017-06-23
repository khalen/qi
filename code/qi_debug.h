#ifndef __QI_DEBUG_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Debug funcs and struct def

#include "has.h"
#include "qi_profile.h"

#if HAS(DEV_BUILD) || HAS(PROF_BUILD)
struct DebugFuncs_s
{
#if HAS(PROF_BUILD)
	Qid_DrawBars_f*       DrawBars;
	Qid_DrawProfileBox_f* DrawProfileBox;
	Qid_DrawTicks_f*      DrawTicks;
#endif
};
#endif

#define __QI_DEBUG_H
#endif // #ifndef __QI_DEBUG_H
