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

template <typename T>
T DB_Get(const char* keyPath);

DataNode* DB_FindNode(const char* path, Symbol* pInt);
template <>
const char* DB_Get(const char* keyPath)
{
	Symbol key;
	DataNode* node = DB_FindNode(keyPath, &key);
	if (!node)
		return nullptr;

	return KS_GetKeyAsString(node->ks, KS_Root(node->ks), key);
}

template<>
IntValue
DB_Get(const char* keyPath)
{
	Symbol    key;
	DataNode* node = DB_FindNode(keyPath, &key);
	if (!node)
		return 0;

	return KS_GetKeyInt(node->ks, KS_Root(node->ks), key);
}

template<>
RealValue
DB_Get(const char* keyPath)
{
	Symbol    key;
	DataNode* node = DB_FindNode(keyPath, &key);
	if (!node)
		return 0;

	return KS_GetKeyReal(node->ks, KS_Root(node->ks), key);
}

template<>
bool
DB_Get(const char* keyPath)
{
	Symbol    key;
	DataNode* node = DB_FindNode(keyPath, &key);
	if (!node)
		return 0;

	return KS_GetKeyBool(node->ks, KS_Root(node->ks), key);
}

template<>
ValueRef
DB_Get(const char* keyPath)
{
	Symbol    key;
	DataNode* node = DB_FindNode(keyPath, &key);
	if (!node)
		return 0;

	return KS_ObjectGetValue(node->ks, KS_Root(node->ks), key);
}

DataNode*
DB_FindOrCreateChild_r(const char** path, DataNode* root, Symbol** keyp)
{
	return nullptr;
}

DataNode*
DB_FindNode(const char* path, Symbol* key)
{
	return DB_FindOrCreateChild_r(&path, gdb->root, &key);
}

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
