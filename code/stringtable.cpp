//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// String table implementation
//

#include "basictypes.h"

#include "debug.h"
#include "stringtable.h"
#include "hash.h"

#include <stdlib.h>
#include <string.h>
#include <memory.h>

void
RebuildHashTable(StringTable* st)
{
	const char* strings = ST_Strings(st);
	const char* cur     = strings + 1;

	u32* hashTable = ST_HashTable(st);
	memset(hashTable, 0, sizeof(u32) * st->hashSlots);

	while (cur < strings + st->stringBytes)
	{
		const HashLength hl  = Hash_String(cur);
		u32              idx = hl.hash % st->hashSlots;
		while (hashTable[idx])
			idx = (idx + 1) % st->hashSlots;
		hashTable[idx] = cur - strings;
		cur            = cur + hl.length + 1;
	}
}

void
ST_Init(StringTable* st, u32 byteSize, u32 avgStringLen)
{
	memset(st, 0, sizeof(*st));
	st->byteSize = byteSize;

	r32 estBytesPerString = avgStringLen + 1 + sizeof(u32) * QI_ST_HASH_FACTOR;
	r32 estNumStrings     = (byteSize - sizeof(*st)) / estBytesPerString;
	st->hashSlots         = (u32)estNumStrings;

	memset(ST_HashTable(st), 0, st->hashSlots * sizeof(u32));

	// Reserve string 0 as "empty" hash slot
	ST_Strings(st)[0] = 0;
	st->stringBytes   = 1;
}

void
ST_Grow(StringTable* st, u32 byteSize)
{
	Assert(byteSize >= st->byteSize);

	const char* oldStrings = ST_Strings(st);
	st->byteSize           = byteSize;

	r32 avgStringLen   = (st->count > 0) ? (st->stringBytes / (r32)st->count) : 15.0f;
	r32 bytesPerString = avgStringLen + 1.0f + sizeof(u32) * QI_ST_HASH_FACTOR;
	r32 numStrings     = (byteSize - sizeof(*st)) / bytesPerString;
	st->hashSlots
	    = (u32)(numStrings * QI_ST_HASH_FACTOR) > st->hashSlots ? (u32)numStrings * QI_ST_HASH_FACTOR : st->hashSlots;

	char* newStrings = ST_Strings(st);
	memmove(newStrings, oldStrings, st->stringBytes);

	RebuildHashTable(st);
}

size_t
ST_Pack(StringTable* st)
{
	const char* oldStrings = ST_Strings(st);
	st->hashSlots          = (u32)(st->count * QI_ST_HASH_FACTOR);

	if (st->hashSlots < 1)
		st->hashSlots = 1;

	if (st->hashSlots < st->count + 1)
		st->hashSlots = st->count + 1;

	char* newStrings = ST_Strings(st);
	memmove(newStrings, oldStrings, st->stringBytes);

	RebuildHashTable(st);
	st->byteSize = (newStrings + st->stringBytes) - (char*)st;

	return st->byteSize;
}

const char*
ST_ToString(const StringTable* st, Symbol symbol)
{
	return ST_Strings(st) + symbol;
}

Symbol
ST_Find(const StringTable* st, const char* str)
{
	// Empty string always maps to 0
	if (*str == 0)
		return 0;

	const HashLength hl        = Hash_String(str);
	const char*      strings   = ST_Strings(st);
	const u32*       hashTable = ST_HashTable(st);

	u32 idx = hl.hash % st->hashSlots;
	while (hashTable[idx])
	{
		if (!strcmp(str, strings + hashTable[idx]))
			return hashTable[idx];

		idx = (idx + 1) % st->hashSlots;
	}

	return QI_ST_FULL;
}

Symbol
ST_Intern(StringTable* st, const char* str)
{
	// Empty string always maps to 0
	if (*str == 0)
		return 0;

	const HashLength hl        = Hash_String(str);
	char*            strings   = ST_Strings(st);
	u32*             hashTable = ST_HashTable(st);

	u32 idx = hl.hash % st->hashSlots;
	while (hashTable[idx])
	{
		if (!strcmp(str, strings + hashTable[idx]))
			return hashTable[idx];

		idx = (idx + 1) % st->hashSlots;
	}

	if (st->count + 1 > st->hashSlots)
		return QI_ST_FULL;

	if ((r32)st->hashSlots / (st->count + 1) < QI_ST_HASH_FACTOR)
		return QI_ST_FULL;

	if (st->stringBytes + hl.length + 1 > ST_StringCapacity(st))
		return QI_ST_FULL;

	Symbol result = st->stringBytes;
	char*  dest   = strings + st->stringBytes;

	hashTable[idx] = result;
	memcpy(dest, str, hl.length + 1);
	st->stringBytes += hl.length + 1;

	return result;
}
