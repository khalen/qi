#ifndef __QI_HASH_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Hash functions (Lua style)
//

struct HashLength
{
    u32 hash;
    u32 length;
};

static inline HashLength Hash_String(const char* str)
{
    u32 hash = 0;
    const char* s = str;
    for (; *s; s++)
        hash = hash ^ ((hash << 5) + (hash >> 2) + (u8)*s);

    HashLength result = {hash, s - str};
    return result;
}

#define __QI_HASH_H
#endif // #ifndef __QI_HASH_H
