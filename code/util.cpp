// Copyright (c) 2020 Quantum Immortality, LTD - JD
//

#include "basictypes.h"
#include <cstdio>
#include <cstdarg>
#include "game.h"
#include "memory.h"

static const size_t kMaxPrintBufferSize = 128 * 1024;

struct UtilGlobals
{
	char* vsBuffer;
	size_t vsBufferSize;
};

static UtilGlobals* g_util = nullptr;

static void InitUtilSubsystem(const SubSystem* sys, bool bIsReInit)
{
	g_util = (UtilGlobals*) sys->globalPtr;
	if (!bIsReInit)
	{
		memset(g_util, 0, sys->globalSize);
		g_util->vsBuffer = (char*)(g_util + 1);
		g_util->vsBufferSize = kMaxPrintBufferSize;
	}
}

SubSystem UtilSubSystem = { "Utils", InitUtilSubsystem, kMaxPrintBufferSize + sizeof(UtilGlobals), nullptr };

const char* VS(const char* msg, ...)
{
	va_list args;

	va_start(args, msg);
	const size_t result = vsnprintf(g_util->vsBuffer, g_util->vsBufferSize, msg, args);
	Assert(result < g_util->vsBufferSize);
	va_end(args);

	return g_util->vsBuffer;
}
