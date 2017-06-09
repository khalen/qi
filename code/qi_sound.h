#ifndef __QI_SOUND_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Sound API for QI engine
//

#include "basictypes.h"

#define QI_SOUND_SAMPLES_PER_SECOND 48000
#define QI_SOUND_BYTES_PER_CHANNEL sizeof(i16)
#define QI_SOUND_CHANNELS 2
#define QI_SOUND_BYTES_PER_SAMPLE (QI_SOUND_BYTES_PER_CHANNEL * QI_SOUND_CHANNELS)
#define QI_SOUND_REQUESTED_LATENCY_MS 33

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
