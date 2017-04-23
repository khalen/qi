#ifndef __QI_SOUND_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Sound API for QI engine
//

#include "basictypes.h"

struct SoundBuffer_s
{
	u32 channels;
	u32 bytesPerSample;
	u32 samplesPerSec;
	u32 numSamples;
	u32 byteSize;
	r32 theta;

	union {
		i16* samples;
		u8*  bytes;
	};
};

static_assert(sizeof(SoundBuffer_s) == 32, "Wrong size for SoundBuffer_s");

SoundBuffer_s* Qis_MakeSoundBuffer(const int numSamples, const int channels);
void Qis_FreeSoundBuffer(SoundBuffer_s* buffer);
void Qis_UpdateSound(SoundBuffer_s* soundBuffer, const u32 samplesToWrite);
void Qis_Init();

#define __QI_SOUND_H
#endif // #ifndef __QI_SOUND_H
