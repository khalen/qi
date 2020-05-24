//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// QI engine platform independent sound API implementation file
//

#include "basictypes.h"

#include "sound.h"
#include "game.h"
#include "math_util.h"
#include "memory.h"

#include <math_util.h>
#include <string.h>
#if HAS(WIN32_BUILD)
#include <malloc.h>
#else
#include <sys/malloc.h>
#endif

struct SoundGlobals_s
{
	r32 toneHz;
	int toneVolume;
};
static SoundGlobals_s* g_sound;

internal void
GenTone(SoundBuffer_s* outputBuf, const u32 samplesToWrite, const r32 toneHz, const r32 toneVolume)
{
	const r32 sampleCycles = outputBuf->samplesPerSec / toneHz;
	const r32 dtPerSample  = Q_2PI * (1.0f / sampleCycles);

	i16* curSubSample = outputBuf->samples;
	for (u32 sampleIdx = 0; sampleIdx < samplesToWrite; sampleIdx++)
	{
		i16 sample = (i16)(sinf(outputBuf->theta) * toneVolume);
		outputBuf->theta += dtPerSample;
		if (outputBuf->theta > Q_2PI)
			outputBuf->theta -= Q_2PI;
		for (u32 channel = 0; channel < outputBuf->channels; channel++)
			*curSubSample++ = sample;
	}
}

SoundBuffer_s*
Qis_MakeSoundBuffer(Memory* memory, const int numSamples, const int channels, const int sampsPerSec)
{
	u32    sampleBufBytes = numSamples * channels * sizeof(i16);
	size_t totalBytes     = sizeof(SoundBuffer_s) + sampleBufBytes;

	u8* mem = (u8*)M_AllocRaw(memory, totalBytes);

	SoundBuffer_s* buffer  = (SoundBuffer_s*)M_AllocRaw(memory, totalBytes);
	buffer->numSamples     = numSamples;
	buffer->channels       = channels;
	buffer->bytesPerSample = channels * sizeof(i16);
	buffer->byteSize       = sampleBufBytes;
	buffer->samplesPerSec  = sampsPerSec;
	buffer->samples        = (i16*)(mem + sizeof(SoundBuffer_s));

	return buffer;
}

void
Qis_UpdateSound(SoundBuffer_s* soundBuffer, const u32 samplesToWrite)
{
#if 0
	GenTone(soundBuffer, samplesToWrite, (r32)g_sound->toneHz, (r32)g_sound->toneVolume);
#else
	memset(soundBuffer->samples, 0, samplesToWrite * soundBuffer->bytesPerSample);
#endif
}

static void Qis_Init(const SubSystem*, bool);

SubSystem SoundSubSystem = {
    "Sound",
    Qis_Init,
    sizeof(SoundGlobals_s),
    nullptr
};

internal void
Qis_Init(const SubSystem* sys, bool isReinit)
{
    Assert(sys->globalPtr);
	g_sound = (SoundGlobals_s*)sys->globalPtr;

	if (!isReinit)
	{
		memset(g_sound, 0, sizeof(*g_sound));
		g_sound->toneHz     = 261.2f;
		g_sound->toneVolume = 25000;
	}
}

static SoundFuncs_s s_sound = {
    Qis_MakeSoundBuffer, Qis_UpdateSound,
};
const SoundFuncs_s* sound = &s_sound;
