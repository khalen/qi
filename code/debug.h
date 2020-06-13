#ifndef __QI_DEBUG_H
#define __QI_DEBUG_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Debug funcs and struct def

#include "has.h"
#include "profile.h"

#if HAS(RELEASE_BUILD)
#define Assert(foo) (void)(foo)
#else
#if defined(__clang__)
#define ASSERT_HANDLER_FUNC __attribute__((analyzer_noreturn))
#else
#define ASSERT_HANDLER_FUNC
#endif

extern void Qi_Assert_Handler(const char *, const char *, const int) ASSERT_HANDLER_FUNC;
#define Assert(foo)                                      \
	do                                                   \
	{                                                    \
		if (!(foo))                                      \
			Qi_Assert_Handler(#foo, __FILE__, __LINE__); \
	} while (0)
#endif

#if HAS(DEV_BUILD) || HAS(PROF_BUILD)
struct DebugFuncs_s
{
#if HAS(PROF_BUILD)
	Qid_DrawBars_f *      DrawBars;
	Qid_DrawProfileBox_f *DrawProfileBox;
	Qid_DrawTicks_f *     DrawTicks;
#endif
};
#endif
#endif
