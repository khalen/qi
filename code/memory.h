#ifndef __QI_MEMORY_H
#define __QI_MEMORY_H

#include "debug.h"
#include "bitmap.h"

#include <string.h>

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Memory allocation and manipulation
//

// Raw memory as handed to us on game initialization
struct Memory
{
	size_t permanentSize;
	u8*    permanentStorage;
	u8*    permanentPos;

	size_t transientSize;
	u8*    transientStorage;
	u8*    transientPos;
};

void* M_AllocRaw(Memory* memory, const size_t size);
template<typename T>
T*
M_New(Memory* memory)
{
	void* buf = M_AllocRaw(memory, sizeof(T));
	Assert(buf != nullptr);
	memset(buf, 0, sizeof(T));
	return reinterpret_cast<T*>(buf);
}

void* M_TransientAllocRaw(Memory* memory, const size_t size);
template<typename T>
T*
M_TransientNew(Memory* memory)
{
	void* buf = M_TransientAllocRaw(memory, sizeof(T));
	Assert(buf != nullptr);
	memset(buf, 0, sizeof(T));
	return reinterpret_cast<T*>(buf);
}

// Memory arena, simple incrementing allocator
struct MemoryArena
{
	u8*    base;
	size_t size;
	size_t curOffset;
};

void MA_Init(MemoryArena* arena, Memory* memory, const size_t size);
u8* MA_Alloc(MemoryArena* arena, const size_t size);

// Malloc / free general buddy allocator
struct MemLink
{
	MemLink* next;
	MemLink* prev;
};

struct BuddyAllocator
{
    u8* basePtr;
    size_t size;
    size_t actualSize;
    size_t freeSize;
    size_t maxLevel;
    size_t minSizeShift;
    size_t freeBitsOffset;
    size_t splitBitsOffset;
};
static_assert((sizeof(BuddyAllocator) & (sizeof(MemLink) - 1)) == 0, "Bad buddy allocator struct size");

BuddyAllocator* BA_InitBuffer(u8* buffer, const size_t size, const size_t smallestBlockSize);
BuddyAllocator* BA_Init(Memory* memory, const size_t size, const size_t smallestBlockSize, const bool isTransient = false);
void* BA_Alloc(BuddyAllocator* allocator, const size_t size);
void* BA_Realloc(BuddyAllocator* allocator, void* mem, const size_t newSize);
void* BA_Calloc(BuddyAllocator* allocator, const size_t size);
void BA_Free(BuddyAllocator* allocator, void* block);
size_t BA_DumpInfo(BuddyAllocator* allocator);

// Generic allocator support
struct Allocator;
typedef void* (ReallocFn)(Allocator* allocator, void* prevMem, const size_t newSize);
typedef void (FreeFn)(Allocator* allocator, void* mem);

struct Allocator
{
	ReallocFn* realloc;
	FreeFn* free;
};

struct MemoryArenaAllocator : public Allocator
{
	MemoryArena arena;
};
void MAA_Init(MemoryArenaAllocator* maa, const size_t arenaSize);

struct BuddyAllocatorAllocator : public Allocator
{
	BuddyAllocator* buddy;
};
void BAA_Init(BuddyAllocatorAllocator* baa, Memory* memory, const size_t size, const size_t smallestBlockSize, const bool isTransient = false);

#endif // #ifndef __QI_MEMORY_H

