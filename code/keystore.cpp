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


