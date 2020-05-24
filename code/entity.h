#ifndef __QI_ENT_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Basic entity and entity manager defs
//

#include "basictypes.h"

#define QI_ENT_INDEX_BITS 24
#define QI_ENT_INDEX_MASK ((1 << (u32)QI_ENT_INDEX_BITS) - 1)

#define QI_ENT_GENERATION_BITS 8
#define QI_ENT_GENERATION_MASK (((1 << (u32)QI_ENT_GENERATION_BITS) - 1) << QI_ENT_INDEX_BITS)

struct Ent
{
	u32 code;
	u32 index() { return (code & QI_ENT_INDEX_MASK); }
    u32 generation() { return code >> QI_ENT_INDEX_BITS; }
};

struct EntManager
{
    static bool isAlive(const Ent ent);
    static Ent create();
    static void destroy(Ent ent);
};

#define __QI_ENT_H
#endif // #ifndef __QI_ENT_H
