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

void
GenTone(i16*        samples,
        const u32   sampleOffset,
        const float toneHz,
        const float toneVolume,
        const u32   samplesPerSec,
        const u32   numSamples)
{
	const float cyclesPerSample = toneHz / samplesPerSec;
	const float dtPerSample     = cyclesPerSample * Q_2PI;
	float       t               = sampleOffset * dtPerSample;
	for (u32 sampleIdx = 0; sampleIdx < numSamples * 2; sampleIdx += 2, t += dtPerSample)
	{
		i16 sample             = (i16)(sinf(t) * toneVolume + 0.5f);
		samples[sampleIdx + 0] = sample;
		samples[sampleIdx + 1] = sample;
	}
}
