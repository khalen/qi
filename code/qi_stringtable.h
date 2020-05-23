#ifndef __QI_STRINGTABLE_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// String hash table, useful for symbols or etc. Table is a contiguous block of memory that can be memcpy'd etc.
//

#include "basictypes.h"

#define QI_ST_FULL        -1
#define QI_ST_HASH_FACTOR 2.0f

typedef u32 Symbol;

struct StringTable
{
	u32 byteSize;    // Size of the string table including this header
	u32 count;       // Number of strings in the table
	u32 hashSlots;   // Total number of hash slots
	u32 stringBytes; // Bytes consumed by string data
};

static inline u32*
ST_HashTable(const StringTable* st)
{
	return (u32*)(st + 1);
}

static inline char*
ST_Strings(const StringTable* st)
{
	return (char*)(ST_HashTable(st) + st->hashSlots);
}

static inline u32
ST_StringCapacity(StringTable* st)
{
	return st->byteSize - sizeof(*st) - sizeof(u32) * st->hashSlots;
}

void        ST_Init(StringTable* st, u32 byteSize, u32 avgSymbolLen);
void        ST_Grow(StringTable* st, u32 newSize);
size_t      ST_Pack(StringTable* st);
const char* ST_ToString(const StringTable* st, u32 symbol);
Symbol      ST_Intern(StringTable* st, const char* str);
Symbol      ST_Find(const StringTable* st, const char* str);

#define __QI_STRINGTABLE_H
#endif // #ifndef __QI_STRINGTABLE_H
