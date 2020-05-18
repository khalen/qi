//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// File purpose
//

#include "basictypes.h"

#include "qi.h"
#include "qi_util.h"
#include "qi_memory.h"
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
	return (1 << level) + idxInLevel - 1;
}

static inline u32
indexInLevel(const BuddyAllocator_s* allocator, const void* ptr, const u32 level)
{
	const uintptr_t base = (uintptr_t)allocator->basePtr;
	const uintptr_t test = (uintptr_t)ptr;
	return (test - base) / blockSizeOfLevel(allocator->size, level);
}

// idx = (p - base) / bs
// idx * bs = p - base
// idx * bs + base = p
static inline void*
ptrInLevel(const BuddyAllocator_s* allocator, const u32 index, const u32 level)
{
	const uintptr_t base = (uintptr_t)allocator->basePtr;
	return (u8*)(base + index * blockSizeOfLevel(allocator->size, level));
}

static inline void*
buddyPtr(const BuddyAllocator_s* allocator, const void* ptr, const u32 level)
{
	const uintptr_t blockBytes = (uintptr_t)ptr;
	return (void*)(blockBytes + blockSizeOfLevel(allocator->size, level));
}

static inline MemLink_s**
getFreeLists(BuddyAllocator_s* allocator)
{
	return (MemLink_s**)(((u8*)allocator) + sizeof(*allocator));
}

static inline u8*
getFreeBits(BuddyAllocator_s* allocator)
{
	return ((u8*)allocator) + allocator->freeBitsOffset;
}

static inline u8*
getSplitBits(BuddyAllocator_s* allocator)
{
	return ((u8*)allocator) + allocator->splitBitsOffset;
}

static inline bool
isBlockIndexSplit(BuddyAllocator_s* allocator, const u32 idxInLevel, const u32 level)
{
	Assert(level <= allocator->maxLevel);

	if (level == allocator->maxLevel)
		return false;

	const u32 idx	  = blockIndex(idxInLevel, level);
	const u32 byteIdx = idx / 8;
	const u32 bitIdx  = idx % 8;
	return (*(getSplitBits(allocator) + byteIdx) & (1 << bitIdx)) != 0;
}

static inline void
markSplitBlockIndex(BuddyAllocator_s* allocator, const u32 idxInLevel, const u32 level)
{
	// Make sure we're not trying to split a leaf block
	Assert(level < allocator->maxLevel);
	const u32 idx	  = blockIndex(idxInLevel, level);
	const u32 byteIdx = idx / 8;
	const u32 bitIdx  = idx % 8;
	Assert(!isBlockIndexSplit(allocator, idxInLevel, level));
	*(getSplitBits(allocator) + byteIdx) |= (1 << bitIdx);
}

static inline void
markUnsplitBlockIndex(BuddyAllocator_s* allocator, const u32 idxInLevel, const u32 level)
{
	// Make sure we're not trying to split a leaf block
	Assert(level < allocator->maxLevel);
	const u32 idx	  = blockIndex(idxInLevel, level);
	const u32 byteIdx = idx / 8;
	const u32 bitIdx  = idx % 8;
	Assert(isBlockIndexSplit(allocator, idxInLevel, level));
	*(getSplitBits(allocator) + byteIdx) &= ~(1 << bitIdx);
}

static inline void
addFreeBlock(BuddyAllocator_s* allocator, void* block, const u32 level)
{
	Assert(isBlockIndexSplit(allocator, indexInLevel(allocator, block, level - 1), level - 1));
	MemLink_s** levelFreeLists = getFreeLists(allocator);
	MemLink_s*	memLink		   = (MemLink_s*)block;
	memLink->next			   = levelFreeLists[level];
	memLink->prev			   = nullptr;

	if (levelFreeLists[level] != nullptr)
		levelFreeLists[level]->prev = memLink;

	levelFreeLists[level] = memLink;
}

static inline void
removeFreeBlock(BuddyAllocator_s* allocator, void* block, const u32 level)
{
	MemLink_s** levelFreeLists = getFreeLists(allocator);
	MemLink_s*	memLink		   = (MemLink_s*)block;

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

static inline bool
isBuddyAlsoFree(BuddyAllocator_s* allocator, const u32 idxInLevel, const u32 level)
{
	const u32 idx	  = blockIndex(idxInLevel, level) >> 1;
	const u32 byteIdx = idx / 8;
	const u32 bitIdx  = idx % 8;
	return (*(getFreeBits(allocator) + byteIdx) & (1 << bitIdx)) == 0;
}

static inline void
markAllocatedBlockIndex(BuddyAllocator_s* allocator, const u32 idxInLevel, const u32 level)
{
	const u32 idx	  = blockIndex(idxInLevel, level) >> 1;
	const u32 byteIdx = idx / 8;
	const u32 bitIdx  = idx % 8;
	*(getFreeBits(allocator) + byteIdx) ^= (1 << bitIdx);
}

static inline void
splitBlock(BuddyAllocator_s* allocator, void* block, const u32 level)
{
	Assert(level < allocator->maxLevel);

	const u32 idxInLevel = indexInLevel(allocator, block, level);
	Assert(!isBlockIndexSplit(allocator, idxInLevel, level));
	markSplitBlockIndex(allocator, idxInLevel, level);

	u32*	   blockBytes = (u32*)block;
	MemLink_s *linkA, *linkB;
	linkA = (MemLink_s*)blockBytes;
	linkB = (MemLink_s*)buddyPtr(allocator, blockBytes, level + 1);

	addFreeBlock(allocator, linkB, level + 1);
	addFreeBlock(allocator, linkA, level + 1);
}

static inline void*
allocBlockOfLevel(BuddyAllocator_s* allocator, const u32 level)
{
	if (level == 0)
	{
		printf("Out of memory in allocator %p\n", allocator);
		BA_DumpInfo(allocator);
		return nullptr;
	}

	Assert(level <= allocator->maxLevel);

	MemLink_s** levelFreeLists = getFreeLists(allocator);
	MemLink_s*	freeBlock	   = nullptr;

	if (levelFreeLists[level] == nullptr)
	{
		void* freeInHigherLevel = allocBlockOfLevel(allocator, level - 1);
		if (freeInHigherLevel)
			splitBlock(allocator, freeInHigherLevel, level - 1);
	}
	if (levelFreeLists[level] != nullptr)
	{
		freeBlock			  = levelFreeLists[level];
		levelFreeLists[level] = freeBlock->next;
		if (levelFreeLists[level])
			levelFreeLists[level]->prev = nullptr;
		Assert(isBlockIndexSplit(allocator, indexInLevel(allocator, freeBlock, level - 1), level - 1));
		markAllocatedBlockIndex(allocator, indexInLevel(allocator, freeBlock, level), level);
	}

	return freeBlock;
}

static inline u32
findLevelForBlock(BuddyAllocator_s* allocator, void* block)
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
freeBlockOfLevel(BuddyAllocator_s* allocator, void* block, const u32 level)
{
	Assert(level <= allocator->maxLevel);

	const u32  idxInLevel = indexInLevel(allocator, block, level);
	const bool needsMerge = (level > 0 && isBuddyAlsoFree(allocator, idxInLevel, level));
	markAllocatedBlockIndex(allocator, idxInLevel, level);

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
		removeFreeBlock(allocator, (MemLink_s*)buddy, level);

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
BA_Free(BuddyAllocator_s* allocator, void* block, size_t blockSize)
{
	Assert(block);
	const u32 blockLevel = allocator->maxLevel - (BitScanRight(blockSize >> allocator->minSizeShift) - 1);
	freeBlockOfLevel(allocator, block, blockLevel);
}

void
BA_Free(BuddyAllocator_s* allocator, void* block)
{
	Assert(block);

	const u32 blockLevel = findLevelForBlock(allocator, block);
	BA_Free(allocator, block, allocator->size >> blockLevel);
}

void*
BA_Alloc(BuddyAllocator_s* allocator, const size_t requestedSize)
{
	size_t blockSize = NextHigherPow2(requestedSize);
	if (blockSize < 1 << allocator->minSizeShift)
		blockSize = (1 << allocator->minSizeShift);
	i32 blockLevel = allocator->maxLevel - (BitScanRight(blockSize >> allocator->minSizeShift) - 1);
	Assert(blockLevel > 0);

	return allocBlockOfLevel(allocator, blockLevel);
}

void*
BA_Calloc(BuddyAllocator_s* allocator, const size_t requestedSize)
{
	void* block = BA_Alloc(allocator, requestedSize);
	Assert(block);
	memset(block, 0, requestedSize);
	return block;
}

void*
BA_Realloc(BuddyAllocator_s* allocator, void* ptr, const size_t newSize)
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
initFreeLists_r(BuddyAllocator_s* allocator, size_t idxInLevel, size_t level)
{
	Assert(level >= 0);

	if (level >= allocator->maxLevel)
		return;

	// If I am not split, I already know my parent was split or we wouldn't have recursed. Therefore I am free, and
	// I don't need to proceed to my children.
	if (!isBlockIndexSplit(allocator, idxInLevel, level))
	{
		addFreeBlock(allocator, ptrInLevel(allocator, idxInLevel, level), level);
		markAllocatedBlockIndex(allocator, idxInLevel, level);
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
initFreeLists(BuddyAllocator_s* allocator, u8* memStart)
{
	for (ssize_t level = (ssize_t)allocator->maxLevel - 1; level >= 0; level--)
	{
		const size_t allocIndexInLevel = indexInLevel(allocator, memStart, (size_t)level);
		for (size_t rightIdx = 0; rightIdx <= allocIndexInLevel; rightIdx++)
			markSplitBlockIndex(allocator, rightIdx, level);
	}

	initFreeLists_r(allocator, 0, 0);

	// Special case a possible rightmost minimum size block
	const size_t allocIndexInLevel = indexInLevel(allocator, memStart, allocator->maxLevel);
	if (allocIndexInLevel & 1)
	{
		addFreeBlock(allocator, ptrInLevel(allocator, allocIndexInLevel, allocator->maxLevel), allocator->maxLevel);
		markAllocatedBlockIndex(allocator, allocIndexInLevel, allocator->maxLevel);
	}
}

BuddyAllocator_s*
BA_InitBuffer(u8* buffer, const size_t size, const size_t smallestBlockSize)
{
	Assert(buffer);

	const size_t smallestBlock = NextHigherPow2(smallestBlockSize);
	Assert(smallestBlock >= sizeof(MemLink_s));

	// Total bytes of overhead = size of buddy alloc structure + size of level free list ptrs + 1 bit per block for free
	// bits + 1 bit per block except the smallest blocks for split bits
	size_t totalAllocatorSize = NextHigherPow2(size);
	Assert((totalAllocatorSize % smallestBlock) == 0);

	const size_t minSizeShift		  = BitScanRight(smallestBlock) - 1;
	size_t		 smallestBlockCount	  = totalAllocatorSize >> minSizeShift;
	size_t		 totalAllocatorLevels = BitScanRight(smallestBlockCount) - 1;
	size_t		 freeBitsBytes		  = (1 << totalAllocatorLevels) / (sizeof(u8) * 8);
	size_t		 splitBitsBytes		  = (1 << totalAllocatorLevels) / (sizeof(u8) * 8);
	size_t		 overheadBytes
		= sizeof(BuddyAllocator_s) + (totalAllocatorLevels + 1) * sizeof(MemLink_s*) + freeBitsBytes + splitBitsBytes;

	u8* allocatorMemory = buffer;

	// Set up allocator in temp location so allocation shenanigans do not stomp on its metadata
	u8*				  tempAllocatorMemory = allocatorMemory + size - overheadBytes;
	BuddyAllocator_s* allocator			  = (BuddyAllocator_s*)tempAllocatorMemory;
	memset(allocator, 0, overheadBytes);

	allocator->basePtr		= allocatorMemory - (totalAllocatorSize - size);
	allocator->size			= totalAllocatorSize;
	allocator->actualSize	= size;
	allocator->freeSize		= size - overheadBytes;
	allocator->minSizeShift = minSizeShift;
	allocator->maxLevel		= totalAllocatorLevels;
	tempAllocatorMemory += sizeof(BuddyAllocator_s) + (totalAllocatorLevels + 1) * sizeof(MemLink_s*);

	allocator->freeBitsOffset = tempAllocatorMemory - ((u8*)allocator);
	tempAllocatorMemory += freeBitsBytes;

	allocator->splitBitsOffset = tempAllocatorMemory - ((u8*)allocator);
	tempAllocatorMemory += splitBitsBytes;

	Assert(tempAllocatorMemory - (u8*)allocator == (ssize_t)overheadBytes);

	initFreeLists(allocator, allocatorMemory);

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

BuddyAllocator_s*
BA_Init(Memory_s* memory, const size_t size, const size_t smallestBlockSize, const bool isTransient)
{
	// Make sure requested smallest block size is large enough to hold our free list link structure
	const size_t smallestBlock
		= (smallestBlockSize < sizeof(MemLink_s)) ? sizeof(MemLink_s) : NextHigherPow2(smallestBlockSize);

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
BA_DumpInfo(BuddyAllocator_s* allocator)
{
	printf("Allocator info:\n");
	MemLink_s** freeLists = getFreeLists(allocator);
	size_t		totalFree = 0;
	for (size_t i = 0; i <= allocator->maxLevel; i++)
	{
		size_t bSize = blockSizeOfLevel(allocator->size, i);
		printf("Freelist %ld, block size 0x%lx:\n", i, bSize);
		MemLink_s* freeList;
		for (freeList = freeLists[i]; freeList; freeList = freeList->next)
		{
			printf("\t%p", freeList);
			totalFree += bSize;
		}
		printf("\n");
	}
	printf("Total free size: %ld\n", totalFree);
	return totalFree;
}

void*
M_AllocRaw(Memory_s* memory, const size_t size)
{
	size_t allocSize = (size + 15) & ~0xFull;
	Assert(memory && ((uintptr_t)memory->permanentPos & 0xF) == 0
		   && (memory->permanentSize - (size_t)(memory->permanentPos - memory->permanentStorage) > allocSize));
	void* result = memory->permanentPos;
	memory->permanentPos += allocSize;
	return result;
}

void*
M_TransientAllocRaw(Memory_s* memory, const size_t size)
{
	size_t allocSize = (size + 15) & ~0xFull;
	Assert(memory && ((uintptr_t)memory->transientPos & 0xF) == 0
		   && (memory->transientSize - (size_t)(memory->transientPos - memory->transientStorage) > allocSize));
	void* result = memory->transientPos;
	memory->transientPos += allocSize;
	return result;
}
