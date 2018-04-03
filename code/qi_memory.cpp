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
    return (1 << level) + idxInLevel;
}

static inline u32
indexInLevel(const BuddyAllocator_s* allocator, const void* ptr, const u32 level)
{
    const uintptr_t base = (uintptr_t)allocator;
    const uintptr_t test = (uintptr_t)ptr;
    return (test - base) / blockSizeOfLevel(allocator->size, level);
}

// idx = (p - base) / bs
// idx * bs = p - base
// idx * bs + base = p
static inline void*
ptrInLevel(const BuddyAllocator_s* allocator, const u32 index, const u32 level)
{
    const uintptr_t base = (uintptr_t)allocator;
    return (u8*)(base + index * blockSizeOfLevel(allocator->size, level));
}

static inline MemLink_s** getFreeLists(BuddyAllocator_s* allocator)
{
    return (MemLink_s**)((u8*)allocator + sizeof(*allocator));
}

static inline void
addFreeBlock(BuddyAllocator_s* allocator, void* block, const u32 level)
{
    MemLink_s** levelFreeLists = getFreeLists(allocator);
    MemLink_s* memLink = (MemLink_s*) block;
    memLink->next = levelFreeLists[level];
    memLink->prev = nullptr;

    if (levelFreeLists[level] != nullptr)
        levelFreeLists[level]->prev = memLink;

    levelFreeLists[level] = memLink;
}

static inline void
removeFreeBlock(BuddyAllocator_s* allocator, void* block, const u32 level)
{
    MemLink_s** levelFreeLists = getFreeLists(allocator);
    MemLink_s* memLink = (MemLink_s*) block;

    if(memLink == levelFreeLists[level])
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
    const u32 idx = blockIndex(idxInLevel, level) >> 1;
    const u32 byteIdx = idx / 8;
    const u32 bitIdx = idx % 8;
    return (*(allocator->freeBits + byteIdx) & (1 << bitIdx)) != 0;
}

static inline void
markAllocatedBlockIndex(BuddyAllocator_s* allocator, const u32 idxInLevel, const u32 level)
{
    const u32 idx = blockIndex(idxInLevel, level) >> 1;
    const u32 byteIdx = idx / 8;
    const u32 bitIdx = idx % 8;
    *(allocator->freeBits + byteIdx) ^= (1 << bitIdx);
}

static inline bool
isBlockIndexSplit(BuddyAllocator_s* allocator, const u32 idxInLevel, const u32 level)
{
    Assert(level < allocator->levels);
    const u32 idx = blockIndex(idxInLevel, level);
    const u32 byteIdx = idx / 8;
    const u32 bitIdx = idx % 8;
    return (*(allocator->splitBits + byteIdx) & (1 << bitIdx)) != 0;
}

static inline void
markSplitBlockIndex(BuddyAllocator_s* allocator, const u32 idxInLevel, const u32 level)
{
    // Make sure we're not trying to split a leaf block
    Assert(level < allocator->levels);
    const u32 idx = blockIndex(idxInLevel, level);
    const u32 byteIdx = idx / 8;
    const u32 bitIdx = idx % 8;
    Assert(!isBlockIndexSplit(allocator, idxInLevel, level));
    *(allocator->splitBits + byteIdx) |= (1 << bitIdx);
}

static inline void
markUnsplitBlockIndex(BuddyAllocator_s* allocator, const u32 idxInLevel, const u32 level)
{
    // Make sure we're not trying to split a leaf block
    Assert(level < allocator->levels);
    const u32 idx = blockIndex(idxInLevel, level);
    const u32 byteIdx = idx / 8;
    const u32 bitIdx = idx % 8;
    Assert(!isBlockIndexSplit(allocator, idxInLevel, level));
    *(allocator->splitBits + byteIdx) &= ~(1 << bitIdx);
}

static inline void
splitBlock(BuddyAllocator_s* allocator, void* block, const u32 level)
{
    const u32 idxInLevel = indexInLevel(allocator, block, level);
    Assert(!isBlockIndexSplit(allocator, idxInLevel, level));
    markSplitBlockIndex(allocator, idxInLevel, level);

    u32* blockBytes = (u32*)block;
    MemLink_s* linkA, *linkB;
    linkA = (MemLink_s*)blockBytes;
    // linkB = (MemLink_s*)(blockBytes + blockSizeOfLevel(allocator->size, level) / 2);
    linkB = (MemLink_s*)buddyPtr(allocator, blockBytes, level);

    addFreeBlock(allocator, linkB, level + 1);
    addFreeBlock(allocator, linkA, level + 1);
}

static inline void*
allocBlockOfLevel(BuddyAllocator_s* allocator, const u32 level)
{
    Assert(level <= allocator->levels);

    MemLink_s** levelFreeLists = getFreeLists(allocator);
    MemLink_s* freeBlock = nullptr;

    if (levelFreeLists[level] != nullptr)
    {
        freeBlock = levelFreeLists[level];
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
    while(isBlockIndexSplit(allocator, indexInLevel(allocator, block, curLevel), curLevel) && curLevel < allocator->levels)
        curLevel++;

    return curLevel;
}

static inline
void freeBlockOfLevel(BuddyAllocator_s* allocator, void* block, const u32 level)
{
    Assert(level < allocator->levels);

    const u32 idxInLevel = indexInLevel(allocator, block, level);

    if (level > 0 && isBuddyAlsoFree(allocator, idxInLevel, level))
    {
        const u32 leftBlockIdx = idxInLevel & ~1;
        const u32 rightBlockIdx = leftBlockIdx + 1;
        const u32 parentLevel = level - 1;
        Assert(!isBlockIndexSplit(allocator, leftBlockIdx, level));
        Assert(!isBlockIndexSplit(allocator, rightBlockIdx, level));
        Assert(isBlockIndexSplit(allocator, idxInLevel >> 1, parentLevel));
        markUnsplitBlockIndex(allocator, idxInLevel >> 1, parentLevel);
        freeBlockOfLevel(allocator, block, parentLevel);
    }
    else
    {

    }
    markAllocatedBlockIndex(allocator, idxInLevel, level);
}

void BA_Free(BuddyAllocator_s* allocator, void* block, size_t blockSize)
{
    Assert(block);
    const u32 blockLevel = (allocator->size / blockSize) - 1;
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
    Assert(blockSize < allocator->size >> allocator->levels);
    i32 blockLevel = allocator->levels - BitScanRight(blockSize);
    Assert(blockLevel > 0);

    return allocBlockOfLevel(allocator, blockLevel);
}
