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

static DataNode* DN_GetChildren(DataNode* dn)
{
	return dn + 1;
}

template <typename T>
T DB_Get(const char* keyPath);

DataNode* DB_FindNode(const char* path, Symbol* key);
StringTable* KS_GetStringTable();
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
		return false;

	return KS_GetKeyBool(node->ks, KS_Root(node->ks), key);
}

template<>
ValueRef
DB_Get(const char* keyPath)
{
	Symbol    key;
	DataNode* node = DB_FindNode(keyPath, &key);
	if (!node)
		return NilValue;

	return KS_ObjectGetValue(node->ks, KS_Root(node->ks), key);
}

DataNode*
DB_FindOrCreateChild_r(const char* path, const char* curPart, DataNode* root, Symbol* pKey)
{
	char pathPart[kMaxDataNodeNameLength];
	size_t      partLen;
	const char* dotPos = strchr(curPart, '.');
	if (dotPos != nullptr)
	{
		partLen = dotPos - path;
		memcpy(pathPart, path, partLen);
		pathPart[partLen] = 0;
		path += partLen + 1;
	}
	else
	{
		partLen = strlen(path);
		strcpy(pathPart, path);
		path += partLen + 1;
		if (pKey)
			*pKey = ST_Intern(KS_GetStringTable(), pathPart);
	}
	// Check for existing node with that name
	if (root)
	{
		DataNode* children = DN_GetChildren(root);
		for (int i = 0; i < root->usedChildren; i++)
		{
			if (strcmp(children[i].name, pathPart) == 0)
			{
				return DB_FindOrCreateChild_r(path, pathPart, root, pKey);
			}
		}
		// Node not found; Try to find and load a corresponding qed file

	}
	return nullptr;
}

DataNode*
DB_FindNode(const char* path, Symbol* key)
{
	return DB_FindOrCreateChild_r(path, nullptr, gdb->root, key);
}

#if 0
KeyStore *QED_LoadDataStore(const char *dsName)
{
	const char *   fname = VS("qed/%s.qed", dsName);
	Symbol         name  = ST_Intern(gks->symbolTable, dsName);
	GameDataStore *ds    = &gks->dataStores[gks->numDataStores++];
	memset(ds, 0, sizeof(GameDataStore));
	ds->name               = name;
	const char *loadResult = QED_LoadFile(&ds->keyStore, dsName, fname);
	if (loadResult != nullptr)
	{
		gks->numDataStores--;
		fprintf(stderr, "Failed to load game data store %s", fname);
		return nullptr;
	}
	return ds->keyStore;
}

KeyStore *QED_GetDataStore(const char *dsName)
{
	Symbol name = ST_Intern(gks->symbolTable, dsName);
	for (u32 i = 0; i < gks->numDataStores; i++)
	{
		if (gks->dataStores[i].name == name)
			return gks->dataStores[i].keyStore;
	}
	return nullptr;
}
#endif

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
