#ifndef __QI_MEMORY_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Memory allocation and manipulation
//

// Raw memory as handed to us on game initialization
struct Memory_s
{
	size_t permanentSize;
	u8*    permanentStorage;
	u8*    permanentPos;

	size_t transientSize;
	u8*    transientStorage;
	u8*    transientPos;
};

void* M_AllocRaw(Memory_s* memory, const size_t size);
template<typename T>
T*
M_New(Memory_s* memory)
{
	void* buf = M_AllocRaw(memory, sizeof(T));
	Assert(buf != nullptr);
	memset(buf, 0, sizeof(T));
	return reinterpret_cast<T*>(buf);
}

void* M_TransientAllocRaw(Memory_s* memory, const size_t size);
template<typename T>
T*
M_TransientNew(Memory_s* memory)
{
	void* buf = M_TransientAllocRaw(memory, sizeof(T));
	Assert(buf != nullptr);
	memset(buf, 0, sizeof(T));
	return reinterpret_cast<T*>(buf);
}

// Memory arena, simple incrementing allocator
struct MemoryArena_s
{
	u8*    base;
	size_t size;
	size_t curOffset;
};

void MA_Init( MemoryArena_s* arena, const size_t size );
u8* MA_Alloc( MemoryArena_s* arena, const size_t size );

// Malloc / free general buddy allocator
struct MemLink_s
{
    MemLink_s* next;
    MemLink_s* prev;
};

struct BuddyAllocator_s
{
    size_t size;
    size_t levels;
    u8* freeBits;
    u8* splitBits;
};

BuddyAllocator_s* BA_Init(Memory_s* memory, const size_t size, const bool isTransient = false);
void* BA_Alloc(BuddyAllocator_s* allocator, const size_t size);
void* BA_Realloc(BuddyAllocator_s* allocator, const size_t newSize);
void* BA_Calloc(BuddyAllocator_s* allocator, const size_t size);
void BA_Free(BuddyAllocator_s* allocator, void* block);

#define __QI_MEMORY_H
#endif // #ifndef __QI_MEMORY_H
