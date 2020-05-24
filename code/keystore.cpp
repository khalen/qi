//
// Copyright 2020, Quantum Immortality LTD
//

#include "basictypes.h"
#include <stdlib.h>
#include <stdio.h>
#include "stringtable.h"
#include "debug.h"
#include "memory.h"
#include "keystore.h"
#include "game.h"

// Repository for game configuration data, using a global symbol table for key values.
static const size_t kGlobalSymbolTableSize = 64 * 1024;
static const size_t kConfigDataHeapSize = 4 * 1024 * 1024;

// Module
struct KeyStoreGlobals
{
	BuddyAllocator* allocator;
	StringTable* symbolTable;
	KeyStore* store;
};
static KeyStoreGlobals* gks = nullptr;

void KS_InitSubsystem(const SubSystem* sys, bool isReinit);
SubSystem KeyStoreSubsystem
    = {"KeyStore", KS_InitSubsystem, sizeof(KeyStoreGlobals) + kGlobalSymbolTableSize + kConfigDataHeapSize, nullptr};

void
KS_InitSubsystem(const SubSystem* sys, bool isReinit)
{
    Assert(gks == nullptr);

    gks = (KeyStoreGlobals*)sys->globalPtr;
    if (!isReinit)
	{
		u8* stringTableBasePtr = (u8*)(gks + 1);
		gks->symbolTable       = (StringTable*)stringTableBasePtr;
		ST_Init(gks->symbolTable, kGlobalSymbolTableSize, 15);

		u8* dataStoreBasePtr = stringTableBasePtr + kGlobalSymbolTableSize;
		gks->allocator       = BA_InitBuffer(dataStoreBasePtr, kConfigDataHeapSize, 32);
	}
}

// Implementation
struct DataBlock;
struct KeyValue;

struct KeyStore
{
	u32      sizeBytes;
	u32      usedBytes;
	ValueRef root;
};

static u8*
KS__ValuePtr(const KeyStore* ks, const ValueRef ref)
{
	return (u8*)ks + ValueRefOffset(ref);
}

static DataBlock*
KS__GetBlock(const KeyStore* ks, const ValueRef ref)
{
	return (DataBlock*)KS__ValuePtr(ks, ref);
}

KeyStore*
KS_Create(const size_t initialSize)
{
	const size_t actualSize = initialSize + sizeof(KeyStore);
	KeyStore*    ks         = (KeyStore*)BA_Alloc(gks->allocator, actualSize);
	ks->sizeBytes           = (u32)actualSize;
	ks->usedBytes           = (u32)sizeof(KeyStore);
	KS_SetRoot(ks, NilValue);

	return ks;
}

static ValueRef
KS__Write(KeyStore** ksp, ValueType type, void* data, size_t sizeBytes, size_t extraSizeBytes)
{
	KeyStore*    ks          = *ksp;
	const size_t neededSpace = sizeBytes + extraSizeBytes;
	while (ks->sizeBytes - ks->usedBytes < neededSpace)
	{
		const size_t newBufSize = (ks->sizeBytes - ks->usedBytes) * 2;
		ks                      = (KeyStore*)BA_Realloc(gks->allocator, ks, newBufSize);
		ks->sizeBytes           = newBufSize;
		*ksp                    = ks;
	}
	u8* dataPtr = (u8*)ks + ks->usedBytes;
	ks->usedBytes += neededSpace;
	memcpy(dataPtr, data, sizeBytes);
	memset(dataPtr + sizeBytes, 0, extraSizeBytes);
	return MakeValueRef(dataPtr - (u8*)ks, type);
}

struct KeyValue
{
	Symbol   key;
	ValueRef value;
};

struct DataBlock
{
#if HAS(DEV_BUILD)
	ValueType type; // For debugging - note this is the type of the -block- not the values in the block
#endif
	u32 usedElems;
	u32 sizeElems;
	u32 nextBlock;
};
u32 DB_ElemCount(const DataBlock* db);
