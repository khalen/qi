//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// File purpose
//

#include "basictypes.h"

#include "game.h"
#include "util.h"
#include "memory.h"
#include <stdlib.h>
#include <stdio.h>

static inline size_t
blockSizeOfLevel(const size_t totalSize, const u32 level)
{
	return totalSize >> level;
}

static inline u32
blockIndex(const u32 idxInLevel, const u32 level)
{
	return (1u << level) + idxInLevel - 1;
}

static inline u32
indexInLevel(const BuddyAllocator* allocator, const void* ptr, const u32 level)
{
    if (level == 0)
        return 0;

	const uintptr_t base = (uintptr_t)allocator->basePtr;
	const uintptr_t test = (uintptr_t)ptr;
	return (test - base) / blockSizeOfLevel(allocator->size, level);
}

// idx = (p - base) / bs
// idx * bs = p - base
// idx * bs + base = p
static inline void*
ptrInLevel(const BuddyAllocator* allocator, const u32 index, const u32 level)
{
	const uintptr_t base = (uintptr_t)allocator->basePtr;
	return (u8*)(base + index * blockSizeOfLevel(allocator->size, level));
}

static inline void*
buddyPtr(const BuddyAllocator* allocator, const void* ptr, const u32 level)
{
	const uintptr_t blockBytes = (uintptr_t)ptr;
	return (void*)(blockBytes + blockSizeOfLevel(allocator->size, level));
}

static inline MemLink**
getFreeLists(BuddyAllocator* allocator)
{
	return (MemLink**)(((u8*)allocator) + sizeof(*allocator));
}

static inline u8*
getFreeBits(BuddyAllocator* allocator)
{
	return ((u8*)allocator) + allocator->freeBitsOffset;
}

static inline u8*
getSplitBits(BuddyAllocator* allocator)
{
	return ((u8*)allocator) + allocator->splitBitsOffset;
}

static inline bool
isBlockIndexSplit(BuddyAllocator* allocator, const u32 idxInLevel, const u32 level)
{
	Assert(level <= allocator->maxLevel);

	if (level == allocator->maxLevel)
		return false;

	const u32 idx	  = blockIndex(idxInLevel, level);
	const u32 byteIdx = idx / 8;
	const u32 bitIdx  = 7 - idx % 8;
	return (*(getSplitBits(allocator) + byteIdx) & (1 << bitIdx)) != 0;
}

static inline void
markSplitBlockIndex(BuddyAllocator* allocator, const u32 idxInLevel, const u32 level)
{
	// Make sure we're not trying to split a leaf block
	Assert(level < allocator->maxLevel);
	const u32 idx	  = blockIndex(idxInLevel, level);
	const u32 byteIdx = idx / 8;
	const u32 bitIdx  = 7 - idx % 8;
	Assert(!isBlockIndexSplit(allocator, idxInLevel, level));
	*(getSplitBits(allocator) + byteIdx) |= (1 << bitIdx);
}

static inline void
markUnsplitBlockIndex(BuddyAllocator* allocator, const u32 idxInLevel, const u32 level)
{
	// Make sure we're not trying to split a leaf block
	Assert(level < allocator->maxLevel);
	const u32 idx	  = blockIndex(idxInLevel, level);
	const u32 byteIdx = idx / 8;
	const u32 bitIdx  = 7 - idx % 8;
	Assert(isBlockIndexSplit(allocator, idxInLevel, level));
	*(getSplitBits(allocator) + byteIdx) &= ~(1 << bitIdx);
}

static inline bool
isBuddyAlsoFree(BuddyAllocator* allocator, const u32 idxInLevel, const u32 level)
{
    const u32 buddyIdxInLevel = idxInLevel ^ 1;
    const u32 idx	  = blockIndex(buddyIdxInLevel, level);
    const u32 byteIdx = idx / 8;
    const u32 bitIdx  = 7 - idx % 8;
    return (*(getFreeBits(allocator) + byteIdx) & (1 << bitIdx)) != 0;
}

static inline void
allocateBlockIndex(BuddyAllocator* allocator, const u32 idxInLevel, const u32 level)
{
    if (level == 0)
        return;

    const u32 idx	  = blockIndex(idxInLevel, level);
    const u32 byteIdx = idx / 8;
    const u32 bitIdx  = 7 - idx % 8;
    *(getFreeBits(allocator) + byteIdx) &= ~(1 << bitIdx);
}

static inline void
freeBlockIndex(BuddyAllocator* allocator, const u32 idxInLevel, const u32 level)
{
    if (level == 0)
        return;

    const u32 idx	  = blockIndex(idxInLevel, level);
    const u32 byteIdx = idx / 8;
    const u32 bitIdx  = 7 - idx % 8;
    *(getFreeBits(allocator) + byteIdx) |= (1 << bitIdx);
}

[[maybe_unused]] static inline void
toggleAllocatedBlockIndex(BuddyAllocator* allocator, const u32 idxInLevel, const u32 level)
{
    const u32 idx	  = blockIndex(idxInLevel, level);
    const u32 byteIdx = idx / 8;
    const u32 bitIdx  = 7 - idx % 8;
    *(getFreeBits(allocator) + byteIdx) ^= (1 << bitIdx);
}

static inline void
addFreeBlock(BuddyAllocator* allocator, void* block, const u32 level)
{
	Assert(level == 0 || isBlockIndexSplit(allocator, indexInLevel(allocator, block, level - 1), level - 1));

    const u32 idxInLevel = indexInLevel(allocator, block, level);
    freeBlockIndex(allocator, idxInLevel, level);

	MemLink** levelFreeLists = getFreeLists(allocator);
	MemLink*	memLink		   = (MemLink*)block;
	memLink->next			   = levelFreeLists[level];
	memLink->prev			   = nullptr;

	if (levelFreeLists[level] != nullptr)
		levelFreeLists[level]->prev = memLink;

	levelFreeLists[level] = memLink;
}

static inline void
removeFreeBlock(BuddyAllocator* allocator, void* block, const u32 level)
{
    const u32 idxInLevel = indexInLevel(allocator, block, level);
    allocateBlockIndex(allocator, idxInLevel, level);

	MemLink** levelFreeLists = getFreeLists(allocator);
	MemLink*	memLink		   = (MemLink*)block;

	if (memLink == levelFreeLists[level])
	{
		Assert(memLink->prev == nullptr);
		levelFreeLists[level] = memLink->next;
	}
	else
	{
		Assert(memLink->prev);
		memLink->prev->next = memLink->next;
	}

	if (memLink->next)
		memLink->next->prev = memLink->prev;
}

static inline void
splitBlock(BuddyAllocator* allocator, void* block, const u32 level)
{
	Assert(level < allocator->maxLevel);

	const u32 idxInLevel = indexInLevel(allocator, block, level);
	Assert(!isBlockIndexSplit(allocator, idxInLevel, level));
	markSplitBlockIndex(allocator, idxInLevel, level);

	u32*	   blockBytes = (u32*)block;
	MemLink *linkA, *linkB;
	linkA = (MemLink*)blockBytes;
	linkB = (MemLink*)buddyPtr(allocator, blockBytes, level + 1);

	addFreeBlock(allocator, linkB, level + 1);
	addFreeBlock(allocator, linkA, level + 1);
}

static inline void*
allocBlockOfLevel(BuddyAllocator* allocator, const u32 level)
{
	Assert(level <= allocator->maxLevel);

	MemLink** levelFreeLists = getFreeLists(allocator);
	MemLink*	freeBlock	   = nullptr;

	if (levelFreeLists[level] == nullptr)
	{
        if (level == 0)
        {
            printf("Out of memory in allocator %p\n", allocator);
            BA_DumpInfo(allocator);
            return nullptr;
        }

		void* freeInHigherLevel = allocBlockOfLevel(allocator, level - 1);
		if (freeInHigherLevel)
			splitBlock(allocator, freeInHigherLevel, level - 1);
	}
	if (levelFreeLists[level] != nullptr)
	{
		freeBlock			  = levelFreeLists[level];
		removeFreeBlock(allocator, freeBlock, level);
		Assert(level == 0 || isBlockIndexSplit(allocator, indexInLevel(allocator, freeBlock, level - 1), level - 1));
	}

	return freeBlock;
}

static inline u32
findLevelForBlock(BuddyAllocator* allocator, void* block)
{
	i32 curLevel = allocator->maxLevel;
	while (curLevel > 0)
	{
		if (isBlockIndexSplit(allocator, indexInLevel(allocator, block, curLevel - 1), curLevel - 1))
			return curLevel;
		curLevel--;
	}

	return 0;
}

static inline void
freeBlockOfLevel(BuddyAllocator* allocator, void* block, const u32 level)
{
	Assert(level <= allocator->maxLevel);

	const u32  idxInLevel = indexInLevel(allocator, block, level);
	const bool needsMerge = (level > 0 && isBuddyAlsoFree(allocator, idxInLevel, level));

	if (needsMerge)
	{
		const u32 leftBlockIdx	= idxInLevel & ~1;
		const u32 rightBlockIdx = leftBlockIdx + 1;
		void*	  leftBlock		= ptrInLevel(allocator, leftBlockIdx, level);
		void*	  rightBlock	= ptrInLevel(allocator, rightBlockIdx, level);
		void*	  buddy			= (block == leftBlock) ? rightBlock : leftBlock;

		const u32 parentLevel = level - 1;
		Assert(!isBlockIndexSplit(allocator, leftBlockIdx, level));
		Assert(!isBlockIndexSplit(allocator, rightBlockIdx, level));
		Assert(isBlockIndexSplit(allocator, idxInLevel >> 1, parentLevel));

		// Remove free buddy
		removeFreeBlock(allocator, (MemLink*)buddy, level);

		markUnsplitBlockIndex(allocator, idxInLevel >> 1, parentLevel);

		// Free parent
		freeBlockOfLevel(allocator, leftBlock, parentLevel);
	}
	else
	{
		addFreeBlock(allocator, block, level);
	}
}

void
BA_Free(BuddyAllocator* allocator, void* block, size_t blockSize)
{
	Assert(block);
	const u32 blockLevel = allocator->maxLevel - (BitScanRight(blockSize >> allocator->minSizeShift) - 1);
	freeBlockOfLevel(allocator, block, blockLevel);
}

void
BA_Free(BuddyAllocator* allocator, void* block)
{
	Assert(block);

	const u32 blockLevel = findLevelForBlock(allocator, block);
	BA_Free(allocator, block, allocator->size >> blockLevel);
}

void*
BA_Alloc(BuddyAllocator* allocator, const size_t requestedSize)
{
	size_t blockSize = NextHigherPow2(requestedSize);
	if (blockSize < 1 << allocator->minSizeShift)
		blockSize = (1 << allocator->minSizeShift);
	i32 blockLevel = allocator->maxLevel - (BitScanRight(blockSize >> allocator->minSizeShift) - 1);
	Assert(blockLevel > 0);

	return allocBlockOfLevel(allocator, blockLevel);
}

void*
BA_Calloc(BuddyAllocator* allocator, const size_t requestedSize)
{
	void* block = BA_Alloc(allocator, requestedSize);
	Assert(block);
	memset(block, 0, requestedSize);
	return block;
}

void*
BA_Realloc(BuddyAllocator* allocator, void* ptr, const size_t newSize)
{
	if (ptr == nullptr)
		return BA_Alloc(allocator, newSize);

	if (newSize == 0)
	{
		BA_Free(allocator, ptr);
		return nullptr;
	}

	size_t level	 = findLevelForBlock(allocator, ptr);
	size_t blockSize = blockSizeOfLevel(allocator->size, level);

	if (newSize <= blockSize)
		return ptr;

	// Don't free before allocating the new buffer, since freeing will alter the existing block
	void* newBlock = BA_Alloc(allocator, newSize);
	Assert(newBlock);

	memmove(newBlock, ptr, blockSize);
	BA_Free(allocator, ptr, blockSize);

	return newBlock;
}

void
initFreeLists_r(BuddyAllocator* allocator, size_t idxInLevel, size_t level)
{
	Assert(level >= 0);

	if (level >= allocator->maxLevel)
		return;

	// If I am not split, I already know my parent was split or we wouldn't have recursed. Therefore I am free, and
	// I don't need to proceed to my children.
	if (!isBlockIndexSplit(allocator, idxInLevel, level))
	{
		addFreeBlock(allocator, ptrInLevel(allocator, idxInLevel, level), level);
	}
	else
	{
		const size_t leftChildIdx  = idxInLevel * 2;
		const size_t rightChildIdx = leftChildIdx + 1;
		initFreeLists_r(allocator, rightChildIdx, level + 1);
		initFreeLists_r(allocator, leftChildIdx, level + 1);
	}
}

void
initFreeLists(BuddyAllocator* allocator, u8* memStart)
{
    if (allocator->basePtr < memStart)
    {
        for (ssize_t level = (ssize_t) allocator->maxLevel - 1; level >= 0; level--) {
            const size_t allocIndexInLevel = indexInLevel(allocator, memStart, (size_t) level);
            for (size_t rightIdx = 0; rightIdx <= allocIndexInLevel; rightIdx++)
                markSplitBlockIndex(allocator, rightIdx, level);
        }
    }

	initFreeLists_r(allocator, 0, 0);

	// Special case a possible rightmost minimum size block
	const size_t allocIndexInLevel = indexInLevel(allocator, memStart, allocator->maxLevel);
	if (allocIndexInLevel & 1)
	{
		addFreeBlock(allocator, ptrInLevel(allocator, allocIndexInLevel, allocator->maxLevel), allocator->maxLevel);
	}
}

BuddyAllocator*
BA_InitBuffer(u8* buffer, const size_t size, const size_t smallestBlockSize)
{
	Assert(buffer);

	const size_t smallestBlock = NextHigherPow2(smallestBlockSize);
	Assert(smallestBlock >= sizeof(MemLink));

	// Total bytes of overhead = size of buddy alloc structure + size of level free list ptrs + 1 bit per block for free
	// bits + 1 bit per block except the smallest blocks for split bits
	size_t totalAllocatorSize = NextHigherPow2(size);
	Assert((totalAllocatorSize % smallestBlock) == 0);

	const size_t minSizeShift		  = BitScanRight(smallestBlock) - 1;
	size_t		 smallestBlockCount	  = totalAllocatorSize >> minSizeShift;
	size_t		 totalAllocatorLevels = BitScanRight(smallestBlockCount) - 1;
    size_t		 splitBitsBytes		  = (1 << totalAllocatorLevels) / (sizeof(u8) * 8);
    size_t		 freeBitsBytes		  = (1 << (totalAllocatorLevels + 1)) / (sizeof(u8) * 8);
	size_t		 overheadBytes
		= sizeof(BuddyAllocator) + (totalAllocatorLevels + 1) * sizeof(MemLink*) + freeBitsBytes + splitBitsBytes;

	u8* allocatorMemory = buffer;

	// Set up allocator in temp location so allocation shenanigans do not stomp on its metadata
	u8*				  tempAllocatorMemory = allocatorMemory + size - overheadBytes;
	BuddyAllocator* allocator			  = (BuddyAllocator*)tempAllocatorMemory;
	memset(allocator, 0, overheadBytes);

	allocator->basePtr		= allocatorMemory - (totalAllocatorSize - size);
	allocator->size			= totalAllocatorSize;
	allocator->actualSize	= size;
	allocator->freeSize		= size - overheadBytes;
	allocator->minSizeShift = minSizeShift;
	allocator->maxLevel		= totalAllocatorLevels;
	tempAllocatorMemory += sizeof(BuddyAllocator) + (totalAllocatorLevels + 1) * sizeof(MemLink*);

	allocator->freeBitsOffset = tempAllocatorMemory - ((u8*)allocator);
	tempAllocatorMemory += freeBitsBytes;

	allocator->splitBitsOffset = tempAllocatorMemory - ((u8*)allocator);
	tempAllocatorMemory += splitBitsBytes;

	Assert(tempAllocatorMemory - (u8*)allocator == (ssize_t)overheadBytes);

	initFreeLists(allocator, allocatorMemory);
#if 0
	printf("Created allocator. Before overhead allocationss:\n");
	BA_DumpInfo(allocator);
#endif

	overheadBytes = (overheadBytes + smallestBlock - 1) & ~(smallestBlock - 1);
	Assert((overheadBytes % smallestBlock) == 0);
	const size_t overheadBlocks = overheadBytes / smallestBlock;

	void* firstBlock = BA_Alloc(allocator, smallestBlock);
	Assert(firstBlock == allocatorMemory);

	for (size_t i = 0; i < overheadBlocks - 1; i++)
		BA_Alloc(allocator, smallestBlock);

	// Copy allocator and initialized bit sets into its final location at the beginning of the actual memory arena
	memcpy(firstBlock, allocator, overheadBytes);

	return allocator;
}

BuddyAllocator*
BA_Init(Memory* memory, const size_t size, const size_t smallestBlockSize, const bool isTransient)
{
	// Make sure requested smallest block size is large enough to hold our free list link structure
	const size_t smallestBlock
		= (smallestBlockSize < sizeof(MemLink)) ? sizeof(MemLink) : NextHigherPow2(smallestBlockSize);

	// Round up to actual size to a multiple of the smallest block size
	size_t actualSize = (size + smallestBlock - 1) & ~(smallestBlock - 1);

	// Total bytes of overhead = size of buddy alloc structure + size of level free list ptrs + 1 bit per block for free
	// bits + 1 bit per block except the smallest blocks for split bits
	size_t totalAllocatorSize = NextHigherPow2(actualSize);
	Assert((totalAllocatorSize % smallestBlock) == 0);

	u8* allocatorMemory
		= isTransient ? (u8*)M_TransientAllocRaw(memory, actualSize) : (u8*)M_AllocRaw(memory, actualSize);
	Assert(allocatorMemory);

	return BA_InitBuffer(allocatorMemory, actualSize, smallestBlock);
}

size_t
BA_DumpInfo(BuddyAllocator* allocator)
{
	printf("\nAllocator info:");
	MemLink** freeLists = getFreeLists(allocator);
	size_t		totalFree = 0;
	for (size_t i = 0; i <= allocator->maxLevel; i++)
	{
		size_t bSize = blockSizeOfLevel(allocator->size, i);
		printf("\nFreelist %zu, block size 0x%zx:", i, bSize);
		MemLink* freeList;
		for (freeList = freeLists[i]; freeList; freeList = freeList->next)
		{
			printf(" %p", freeList);
			totalFree += bSize;
		}
	}
	printf("\nTotal free size: %zu", totalFree);
    size_t		 splitBitsBytes		  = (1 << allocator->maxLevel) / (sizeof(u8) * 8);
    size_t		 freeBitsBytes		  = (1 << (allocator->maxLevel + 1)) / (sizeof(u8) * 8);

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

	printf("\nSplit bits: ");
	u8* b = getSplitBits(allocator);
	for (size_t i = 0; i < splitBitsBytes; i++)
    {
	    printf( " " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(b[i]));
    }
    printf("\nFree bits:  ");
    b = getFreeBits(allocator);
    for (size_t i = 0; i < freeBitsBytes; i++)
    {
        printf( " " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(b[i]));
    }
    printf( "\n");
	return totalFree;
}

void*
M_AllocRaw(Memory* memory, const size_t size)
{
	size_t allocSize = (size + 15) & ~0xFull;
	Assert(memory && ((uintptr_t)memory->permanentPos & 0xF) == 0
		   && (memory->permanentSize - (size_t)(memory->permanentPos - memory->permanentStorage) > allocSize));
	void* result = memory->permanentPos;
	memory->permanentPos += allocSize;
	return result;
}

void*
M_TransientAllocRaw(Memory* memory, const size_t size)
{
	size_t allocSize = (size + 15) & ~0xFull;
	Assert(memory && ((uintptr_t)memory->transientPos & 0xF) == 0
		   && (memory->transientSize - (size_t)(memory->transientPos - memory->transientStorage) > allocSize));
	void* result = memory->transientPos;
	memory->transientPos += allocSize;
	return result;
}

void MA_InitBuffer(MemoryArena *arena, void *memory, const size_t size)
{
	arena->size      = size;
	arena->curOffset = 0;
	arena->base      = (u8*)memory;
}

void MA_Init(MemoryArena *arena, Memory *memory, const size_t size)
{
	MA_InitBuffer(arena, M_AllocRaw(memory, size), size);
}

u8*
MA_Alloc(MemoryArena* arena, const size_t reqSize)
{
    const size_t size = (reqSize + 15) & ~0xFull;

    Assert(arena && arena->curOffset + size <= arena->size);
    u8* mem = arena->base + arena->curOffset;
    arena->curOffset += size;
    return mem;
}

void MA_Reset(MemoryArena* arena)
{
	arena->curOffset = 0;
#if HAS(DEV_BUILD)
	memset(arena->base, 0, arena->size);
#endif
}

static void*
BAA_Realloc_(Allocator* alloc, void* prevMem, const size_t newSize)
{
    BuddyAllocatorAllocator* baa = (BuddyAllocatorAllocator*)alloc;
    return BA_Realloc(baa->buddy, prevMem, newSize);
}

static void
BAA_Free_(Allocator* alloc, void* mem)
{
    BuddyAllocatorAllocator* baa = (BuddyAllocatorAllocator*)alloc;
    return BA_Free(baa->buddy, mem);
}

void
BAA_Init(BuddyAllocatorAllocator* baa, Memory* memory, size_t size, size_t smallestBlockSize, const bool isTransient)
{
	baa->buddy = BA_Init(memory, size, smallestBlockSize, isTransient);
	baa->realloc = BAA_Realloc_;
	baa->free = BAA_Free_;
}

static void* MAA_Realloc_(Allocator* alloc, void*, const size_t newSize)
{
    if (newSize == 0)
        return nullptr;

    MemoryArenaAllocator* maa = (MemoryArenaAllocator*) alloc;
	return MA_Alloc(&maa->arena, newSize);
}

static void MAA_Free_(Allocator*, void*)
{
}

void MAA_Init(MemoryArenaAllocator* maa, Memory* memory, const size_t size)
{
	MA_Init(&maa->arena, memory, size);
    maa->realloc = MAA_Realloc_;
    maa->free = MAA_Free_;
}