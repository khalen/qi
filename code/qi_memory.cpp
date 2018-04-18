//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// File purpose
//

#include "basictypes.h"

#include "qi.h"
#include "qi_util.h"
#include "qi_memory.h"

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
	return (void*)(blockBytes + blockSizeOfLevel(allocator->size, level) / 2);
}

static inline MemLink_s**
getFreeLists(BuddyAllocator_s* allocator)
{
	return (MemLink_s**)((u8*)allocator + sizeof(*allocator));
}

static inline void
addFreeBlock(BuddyAllocator_s* allocator, void* block, const u32 level)
{
	MemLink_s** levelFreeLists = getFreeLists(allocator);
	MemLink_s*  memLink        = (MemLink_s*)block;
	memLink->next              = levelFreeLists[level];
	memLink->prev              = nullptr;

	if (levelFreeLists[level] != nullptr)
		levelFreeLists[level]->prev = memLink;

	levelFreeLists[level] = memLink;
}

static inline void
removeFreeBlock(BuddyAllocator_s* allocator, void* block, const u32 level)
{
	MemLink_s** levelFreeLists = getFreeLists(allocator);
	MemLink_s*  memLink        = (MemLink_s*)block;

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
	const u32 idx     = blockIndex(idxInLevel, level) >> 1;
	const u32 byteIdx = idx / 8;
	const u32 bitIdx  = idx % 8;
	return (*(allocator->freeBits + byteIdx) & (1 << bitIdx)) != 0;
}

static inline void
markAllocatedBlockIndex(BuddyAllocator_s* allocator, const u32 idxInLevel, const u32 level)
{
	const u32 idx     = blockIndex(idxInLevel, level) >> 1;
	const u32 byteIdx = idx / 8;
	const u32 bitIdx  = idx % 8;
	*(allocator->freeBits + byteIdx) ^= (1 << bitIdx);
}

static inline bool
isBlockIndexSplit(BuddyAllocator_s* allocator, const u32 idxInLevel, const u32 level)
{
	Assert(level < allocator->levels);
	const u32 idx     = blockIndex(idxInLevel, level);
	const u32 byteIdx = idx / 8;
	const u32 bitIdx  = idx % 8;
	return (*(allocator->splitBits + byteIdx) & (1 << bitIdx)) != 0;
}

static inline void
markSplitBlockIndex(BuddyAllocator_s* allocator, const u32 idxInLevel, const u32 level)
{
	// Make sure we're not trying to split a leaf block
	Assert(level < allocator->levels);
	const u32 idx     = blockIndex(idxInLevel, level);
	const u32 byteIdx = idx / 8;
	const u32 bitIdx  = idx % 8;
	Assert(!isBlockIndexSplit(allocator, idxInLevel, level));
	*(allocator->splitBits + byteIdx) |= (1 << bitIdx);
}

static inline void
markUnsplitBlockIndex(BuddyAllocator_s* allocator, const u32 idxInLevel, const u32 level)
{
	// Make sure we're not trying to split a leaf block
	Assert(level < allocator->levels);
	const u32 idx     = blockIndex(idxInLevel, level);
	const u32 byteIdx = idx / 8;
	const u32 bitIdx  = idx % 8;
	Assert(isBlockIndexSplit(allocator, idxInLevel, level));
	*(allocator->splitBits + byteIdx) &= ~(1 << bitIdx);
}

static inline void
splitBlock(BuddyAllocator_s* allocator, void* block, const u32 level)
{
	const u32 idxInLevel = indexInLevel(allocator, block, level);
	Assert(!isBlockIndexSplit(allocator, idxInLevel, level));
	markSplitBlockIndex(allocator, idxInLevel, level);

	u32*       blockBytes = (u32*)block;
	MemLink_s *linkA, *linkB;
	linkA = (MemLink_s*)blockBytes;
	linkB = (MemLink_s*)buddyPtr(allocator, blockBytes, level);

	addFreeBlock(allocator, linkB, level + 1);
	addFreeBlock(allocator, linkA, level + 1);
}

static inline void*
allocBlockOfLevel(BuddyAllocator_s* allocator, const u32 level)
{
	Assert(level <= allocator->levels);

	MemLink_s** levelFreeLists = getFreeLists(allocator);
	MemLink_s*  freeBlock      = nullptr;

	if (levelFreeLists[level] != nullptr)
	{
		freeBlock             = levelFreeLists[level];
		levelFreeLists[level] = freeBlock->next;
		markAllocatedBlockIndex(allocator, indexInLevel(allocator, freeBlock, level), level);
		return freeBlock;
	}
	else
	{
		void* freeInHigherLevel = allocBlockOfLevel(allocator, level - 1);
		if (freeInHigherLevel)
		{
			splitBlock(allocator, freeInHigherLevel, level - 1);
			void* freeBlock = allocBlockOfLevel(allocator, level);
			Assert(freeBlock);
		}
	}

	return freeBlock;
}

static inline u32
findLevelForBlock(BuddyAllocator_s* allocator, void* block)
{
	u32 curLevel = 0;
	while (isBlockIndexSplit(allocator, indexInLevel(allocator, block, curLevel), curLevel)
	       && curLevel < allocator->levels)
		curLevel++;

	return curLevel;
}

static inline void
freeBlockOfLevel(BuddyAllocator_s* allocator, void* block, const u32 level)
{
	Assert(level < allocator->levels);

	const u32 idxInLevel = indexInLevel(allocator, block, level);

	if (level > 0 && isBuddyAlsoFree(allocator, idxInLevel, level))
	{
		const u32 leftBlockIdx  = idxInLevel & ~1;
		const u32 rightBlockIdx = leftBlockIdx + 1;
		const u32 parentLevel   = level - 1;
		Assert(!isBlockIndexSplit(allocator, leftBlockIdx, level));
		Assert(!isBlockIndexSplit(allocator, rightBlockIdx, level));
		Assert(isBlockIndexSplit(allocator, idxInLevel >> 1, parentLevel));

		// Remove free buddy
		removeFreeBlock(allocator, buddyPtr(allocator, block, level), level);
		markUnsplitBlockIndex(allocator, idxInLevel >> 1, parentLevel);

		// Free parent
		freeBlockOfLevel(allocator, block, parentLevel);
	}
	else
	{
		addFreeBlock(allocator, block, level);
	}

	markAllocatedBlockIndex(allocator, idxInLevel, level);
}

void
BA_Free(BuddyAllocator_s* allocator, void* block, size_t blockSize)
{
	Assert(block);
	const u32 blockLevel = allocator->levels - BitScanRight(blockSize >> allocator->minSizeShift);
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
	Assert(blockSize <= 1 << allocator->minSizeShift);
	i32 blockLevel = allocator->levels - BitScanRight(blockSize >> allocator->minSizeShift);
	Assert(blockLevel > 0);

	return allocBlockOfLevel(allocator, blockLevel);
}

void
initFreeLists_r(BuddyAllocator_s* allocator, size_t level, size_t idxInLevel)
{
    if (level >= allocator->levels)
        return;

    const size_t childLevel = level + 1;
    // Split this level / idx
    const size_t leftChildIdx  = (1 << childLevel) + idxInLevel - 1;
    const size_t rightChildIdx = leftChildIdx + 1;
    // If left child is split but right is not, add right to free list and recurse, otherwise we're done
    if (isBlockIndexSplit(allocator, childLevel, leftChildIdx) && !isBlockIndexSplit(allocator, childLevel, rightChildIdx))
    {
        addFreeBlock(allocator, ptrInLevel(allocator, childLevel, rightChildIdx), rightChildIdx);
        initFreeLists_r(allocator, childLevel, leftChildIdx);
    }
}

void
initFreeLists(BuddyAllocator_s* allocator)
{
    for (ssize_t level = (ssize_t)allocator->levels - 1; level >= 0; level--)
    {
        for (size_t rightIdx = 0; rightIdx <= indexInLevel(allocator, allocator, (size_t)level); rightIdx++)
            markSplitBlockIndex(allocator, rightIdx, level);
    }

    initFreeLists_r(allocator, 0, 0);
}

BuddyAllocator_s*
BA_Init(Memory_s* memory, const size_t size, const size_t smallestBlockSize, const bool isTransient)
{
	// Make sure requested smallest block size is large enough to hold our free list link structure
	const size_t smallestBlock
	    = (smallestBlockSize < sizeof(MemLink_s)) ? sizeof(MemLink_s) : NextHigherPow2(smallestBlockSize);

    const size_t minSizeShift = BitScanRight(smallestBlock);

	// Round up actual size to a multiple of the smallest block size
	size_t actualSize = (size + smallestBlock - 1) & ~(smallestBlock - 1);

	// Total bytes of overhead = size of buddy alloc structure + size of level free list ptrs + 1 bit per block for free
	// bits + 1 bit per block except the smallest blocks for split bits
	size_t totalAllocatorSize = NextHigherPow2(actualSize);
	Assert((totalAllocatorSize % smallestBlock) == 0);

	size_t smallestBlockCount   = totalAllocatorSize >> minSizeShift;
	size_t totalAllocatorLevels = BitScanRight(smallestBlockCount);
	size_t freeBitsBytes        = (1 << totalAllocatorLevels) / (sizeof(u8) * 8);
	size_t splitBitsBytes       = (1 << (totalAllocatorLevels - 1)) / (sizeof(u8) * 8);
	size_t overheadBytes
	    = sizeof(BuddyAllocator_s) + totalAllocatorLevels * sizeof(MemLink_s) + freeBitsBytes + splitBitsBytes;
	overheadBytes = (overheadBytes + smallestBlock - 1) & ~(smallestBlock - 1);
	actualSize += overheadBytes;

	// Make sure the resulting size minus overhead is bigger than the initial requested size
	if (totalAllocatorSize - overheadBytes < actualSize)
	{
		totalAllocatorSize = NextHigherPow2(actualSize);
		Assert((totalAllocatorSize % smallestBlock) == 0);

		smallestBlockCount   = totalAllocatorSize >> minSizeShift;
		totalAllocatorLevels = BitScanRight(smallestBlockCount);
		freeBitsBytes        = (1 << totalAllocatorLevels) / (sizeof(u8) * 8);
		splitBitsBytes       = (1 << (totalAllocatorLevels - 1)) / (sizeof(u8) * 8);
		overheadBytes
		    = sizeof(BuddyAllocator_s) + totalAllocatorLevels * sizeof(MemLink_s) + freeBitsBytes + splitBitsBytes;
		overheadBytes = (overheadBytes + smallestBlock - 1) & ~(smallestBlock - 1);
	}
	Assert(totalAllocatorSize - overheadBytes >= actualSize);

	u8* allocatorMemory
	    = isTransient ? (u8*)M_TransientAllocRaw(memory, actualSize) : (u8*)M_AllocRaw(memory, actualSize);
	Assert(allocatorMemory);

	BuddyAllocator_s* allocator = (BuddyAllocator_s*)allocatorMemory;
    allocator->basePtr = allocatorMemory - (totalAllocatorSize - actualSize);
	allocatorMemory += sizeof(BuddyAllocator_s);
    memset(allocator, 0, sizeof(BuddyAllocator_s));

	allocator->size       = totalAllocatorSize;
	allocator->actualSize = actualSize;
	allocator->freeSize   = actualSize - overheadBytes;
    allocator->minSizeShift = minSizeShift;
	allocator->levels     = totalAllocatorLevels;

	allocator->freeBits   = allocatorMemory;
	allocatorMemory += freeBitsBytes;
    memset(allocator->freeBits, 0, freeBitsBytes);

	allocator->splitBits = allocatorMemory;
	allocatorMemory += splitBitsBytes;
    memset(allocator->splitBits, 0, splitBitsBytes);

    initFreeLists(allocator);

	Assert((overheadBytes % smallestBlock) == 0);
	const size_t overheadBlocks = overheadBytes / smallestBlock;

    void* firstBlock = BA_Alloc(allocator, smallestBlock);
    Assert(firstBlock == allocator);

    for (size_t i = 0; i < overheadBlocks - 1; i++)
        BA_Alloc(allocator, smallestBlock);

    Assert(allocatorMemory - (u8 *)allocator <= overheadBytes);
    return allocator;
}
