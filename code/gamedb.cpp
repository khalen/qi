// Created by Jon Davis on 5/28/20.
//

#include "gamedb.h"
#include "memory.h"

const int kMaxDataNodeNameLength = 64;
const int kDataNodeHeapSize = 4 * 1024 * 1024;

struct DataNode
{
	char name[kMaxDataNodeNameLength];
	KeyStore* ks;
	DataNode* parent;
	u32 sizeChildren;
	u32 usedChildren;
};

struct GameDBGlobals
{
	BuddyAllocator *allocator;
	DataNode* root;
};

static GameDBGlobals* gdb = nullptr;

static void DB_Init(const SubSystem* sys, bool bIsReInit)
{
	gdb = (GameDBGlobals *)sys->globalPtr;
	if (!bIsReInit)
	{
		memset(gdb, 0, sizeof(GameDBGlobals));
		gdb->allocator = BA_InitBuffer((u8 *)(gdb + 1), kDataNodeHeapSize, 16);
	}
}

SubSystem DBSubSystem = { "GameDB", DB_Init, sizeof(GameDBGlobals) + kDataNodeHeapSize, nullptr };
