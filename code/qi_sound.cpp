//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// QI engine platform independent sound API implementation file
//

#include "basictypes.h"

#include "qi.h"
#include "qi_sound.h"
#include "qi_math.h"

#include <math.h>
#include <string.h>
#include <malloc.h>

SoundBuffer_s*
MakeSoundBuffer(SoundBuffer_s* buffer,
                const int      numSamples,
                const int      channels,
                const int      sampsPerSec)
{
	memset(buffer, 0, sizeof(SoundBuffer_s));
	buffer->numSamples     = numSamples;
	buffer->channels       = channels;
	buffer->bytesPerSample = channels * sizeof(i16);
	buffer->byteSize       = buffer->numSamples * buffer->bytesPerSample;
	buffer->samples        = (i16*)malloc(buffer->byteSize);
	buffer->samplesPerSec  = sampsPerSec;

	memset(buffer->samples, 0, buffer->byteSize);

	return buffer;
}

void
FreeSoundBuffer(SoundBuffer_s* buffer)
{
	if (buffer && buffer->samples)
	{
		free(buffer->samples);
	}
	memset(buffer, 0, sizeof(*buffer));
}

void
GenTone(SoundBuffer_s* outputBuf, const u32 samplesToWrite, const r32 toneHz, const r32 toneVolume)
{
	const r32 sampleCycles = outputBuf->samplesPerSec / toneHz;
	const r32 dtPerSample  = Q_2PI * (1.0f / sampleCycles);

	i16* curSubSample = outputBuf->samples;
	for (u32 sampleIdx = 0; sampleIdx < samplesToWrite; sampleIdx++ )
	{
		i16 sample = (i16)(sinf(outputBuf->theta) * toneVolume + 0.5);
        outputBuf->theta += dtPerSample;
        if ( outputBuf->theta > Q_2PI )
            outputBuf->theta -= Q_2PI;
		for (u32 channel = 0; channel < outputBuf->channels; channel++)
			*curSubSample++ = sample;
	}
}
