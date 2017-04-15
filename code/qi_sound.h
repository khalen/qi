#ifndef __QI_SOUND_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Sound API for QI engine
//

void GenTone(i16*        samples,
             const u32   sampleOffset,
             const float toneHz,
             const float toneVolume,
             const u32   samplesPerSec,
             const u32   numSamples);

#define __QI_SOUND_H
#endif // #ifndef __QI_SOUND_H
