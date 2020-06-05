//
// Copyright 2020, Quantum Immortality LTD
//

#include "basictypes.h"
#include "game.h"
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>
#include "stringtable.h"
#include "debug.h"
#include "memory.h"
#include "keystore.h"
#include "game.h"
#include "util.h"
#include "qed_parse.h"

#if HAS(IS_CLANG)
#pragma clang diagnostic push
#pragma ide diagnostic   ignored "bugprone-reserved-identifier"
#endif

// Repository for game configuration data, using a global symbol table for key values.
static const size_t kGlobalSymbolTableSize = 64 * 1024;
static const size_t kConfigDataHeapSize    = 4 * 1024 * 1024;
static const size_t kMaxDataStoreCount     = 64;

struct GameDataStore
{
	Symbol    name;
	KeyStore *keyStore;
};

// Module
struct KeyStoreGlobals
{
	BuddyAllocator *allocator;
	StringTable *   symbolTable;
	GameDataStore   dataStores[kMaxDataStoreCount];
	u32             numDataStores;
};
static KeyStoreGlobals *gks = nullptr;

void KS_InitSubsystem(const SubSystem *sys, bool isReinit);

SubSystem KeyStoreSubsystem = {"KeyStore", KS_InitSubsystem, sizeof(KeyStoreGlobals) + kGlobalSymbolTableSize + kConfigDataHeapSize, nullptr};

void KS_InitSubsystem(const SubSystem *sys, bool isReinit)
{
	Assert(gks == nullptr);

	gks = (KeyStoreGlobals *)sys->globalPtr;
	if (!isReinit)
	{
		u8 *stringTableBasePtr = (u8 *)(gks + 1);
		gks->symbolTable       = (StringTable *)stringTableBasePtr;
		ST_Init(gks->symbolTable, kGlobalSymbolTableSize, 15);

		u8 *dataStoreBasePtr = stringTableBasePtr + kGlobalSymbolTableSize;
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
	Symbol   name;
	ValueRef root;
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

static u8 *KS__ValuePtr(const KeyStore *ks, const ValueRef ref)
{
	return (u8 *)ks + ValueRefOffset(ref);
}

static DataBlock *KS__GetBlock(const KeyStore *ks, const ValueRef ref)
{
	return (DataBlock *)KS__ValuePtr(ks, ref);
}

static ValueRef *KS__GetBlockDataPtr(const KeyStore *, DataBlock *db)
{
	return (ValueRef *)(db + 1);
}

static ValueRef KS__Write(KeyStore **ksp, ValueType type, const void *data, size_t sizeBytes, size_t extraSizeBytes)
{
	KeyStore *   ks          = *ksp;
	const size_t neededSpace = sizeBytes + extraSizeBytes;
	while (ks->sizeBytes - ks->usedBytes < neededSpace)
	{
		const size_t newBufSize = (ks->sizeBytes - ks->usedBytes) * 2;
		ks                      = (KeyStore *)BA_Realloc(gks->allocator, ks, newBufSize);
		ks->sizeBytes           = newBufSize;
		*ksp                    = ks;
	}

	ValueRef ref = MakeValueRef(ks->usedBytes, type);

	memcpy((u8 *)ks + ks->usedBytes, data, sizeBytes);
	ks->usedBytes += sizeBytes;

	memset((u8 *)ks + ks->usedBytes, 0, extraSizeBytes);
	ks->usedBytes += extraSizeBytes;

	return ref;
}

ValueRef KS_Root(const KeyStore *ks)
{
	return ks->root;
}

void KS_SetRoot(KeyStore *ks, const ValueRef root)
{
	ks->root = root;
}

KeyStore *KS_Create(const char *name, u32 initialElems, size_t initialSize)
{
	const size_t actualSize = sizeof(KeyStore) + sizeof(DataBlock) + initialElems * sizeof(KeyValue) + initialSize;
	KeyStore *   ks         = (KeyStore *)BA_Alloc(gks->allocator, actualSize);

	ks->name      = ST_Intern(gks->symbolTable, name);
	ks->sizeBytes = (u32)actualSize;
	ks->usedBytes = (u32)sizeof(KeyStore);
	KS_SetRoot(ks, KS_AddObject(&ks, initialElems));
	Assert(ks->usedBytes == sizeof(KeyStore) + sizeof(DataBlock) + initialElems * sizeof(KeyValue));
	return ks;
}

void KS_Free(KeyStore **ksp)
{
	BA_Free(gks->allocator, *ksp);
	*ksp = nullptr;
}

StringTable *KS_GetStringTable()
{
	return gks->symbolTable;
}

SmallIntValue KS_ValueSmallInt(const KeyStore *ks, ValueRef value)
{
	Assert(ValueRefType(value) == ValueType::SMALLINT);
	return (SmallIntValue)(ValueRefOffset(value));
}

IntValue KS_ValueInt(const KeyStore *ks, ValueRef value)
{
	Assert(ValueRefType(value) == ValueType::INT);
	return *(IntValue *)(KS__ValuePtr(ks, value));
}

RealValue KS_ValueReal(const KeyStore *ks, ValueRef value)
{
	Assert(ValueRefType(value) == ValueType::REAL);
	return *(RealValue *)(KS__ValuePtr(ks, value));
}

const char *KS_ValueString(const KeyStore *ks, ValueRef value)
{
	Assert(ValueRefType(value) == ValueType::STRING);
	return (const char *)(KS__ValuePtr(ks, value));
}

Symbol KS_ValueSymbol(const KeyStore *ks, ValueRef value)
{
	Assert(ValueRefType(value) == ValueType::SYMBOL);
	return (Symbol)(ValueRefOffset(value));
}

const char *KS_ValueSymbolAsStr(const KeyStore *ks, ValueRef value)
{
	Assert(ValueRefType(value) == ValueType::SYMBOL);
	return ST_ToString(gks->symbolTable, KS_ValueSymbol(ks, value));
}

void KS__EmitFixedStr(char **bufPtr, const char *bufEnd, const char *str, size_t len)
{
	Assert(bufEnd - *bufPtr > len);
	strcpy(*bufPtr, str);
	*bufPtr += len;
}

void KS__EmitStr(char **bufPtr, const char *bufEnd, const char *str)
{
	size_t len = strlen(str);
	KS__EmitFixedStr(bufPtr, bufEnd, str, len);
}

void KS__EmitNewlineAndIndent(char **bufPtr, const char *bufEnd, bool pretty, int indent)
{
	static const char *indentStr   = "  ";
	static const int   kIndentSize = 2;

	if (!pretty)
	{
		return;
	}

	KS__EmitFixedStr(bufPtr, bufEnd, "\n", 1);
	for (int i = 0; i < indent; i++)
	{
		KS__EmitFixedStr(bufPtr, bufEnd, indentStr, kIndentSize);
	}
}

bool P_CanStartSymbol(char c);
bool P_CanContinueSymbol(char c);
bool P_IsNonASCII(char c);

u32 KS__ValueToString_r(const KeyStore *ks, ValueRef value, char **bufPtr, const char *bufEnd, bool pretty, int indent)
{
	const char *start = *bufPtr;
	size_t      room  = bufEnd - *bufPtr;
	i32         putLen;
	void *      valuePtr = nullptr;

	switch (ValueRefType(value))
	{
	case NIL:
		KS__EmitFixedStr(bufPtr, bufEnd, "nil", 3);
		break;

	case FALSE:
		KS__EmitFixedStr(bufPtr, bufEnd, "false", 5);
		break;

	case TRUE:
		KS__EmitFixedStr(bufPtr, bufEnd, "true", 4);
		break;

	case SMALLINT:
		putLen = snprintf(*bufPtr, room, "%d", KS_ValueSmallInt(ks, value));
		Assert(putLen > 0 && putLen < room);
		*bufPtr += putLen;
		break;

	case INT:
		putLen = snprintf(*bufPtr, room, "%lld", KS_ValueInt(ks, value));
		Assert(putLen > 0 && putLen < room);
		*bufPtr += putLen;
		break;

	case REAL:
		putLen = snprintf(*bufPtr, room, "%.8g", KS_ValueReal(ks, value));
		Assert(putLen > 0 && putLen < room);
		*bufPtr += putLen;
		break;

	case SYMBOL:
	{
		const char *sym    = KS_ValueSymbolAsStr(ks, value);
		size_t      symLen = strlen(sym);
		if (symLen == 0)
		{
			KS__EmitFixedStr(bufPtr, bufEnd, "``", 2);
		}
		else
		{
			bool needsQuote = !P_CanStartSymbol(*sym);
			for (size_t i = 1; i < symLen; i++)
				needsQuote |= !P_CanContinueSymbol(sym[i]);
			if (needsQuote)
				KS__EmitFixedStr(bufPtr, bufEnd, "`", 1);
			KS__EmitStr(bufPtr, bufEnd, sym);
			if (needsQuote)
				KS__EmitFixedStr(bufPtr, bufEnd, "`", 1);
		}
		break;
	}

	case STRING:
	{
		const char *str = (const char *)(KS__ValuePtr(ks, value));
		size_t      len = strlen(str);
		if (len == 0)
		{
			KS__EmitFixedStr(bufPtr, bufEnd, "\"\"", 2);
		}
		else
		{
			bool needsQuote = P_IsNonASCII(str[0]);
			for (size_t i = 1; i < len; i++)
				needsQuote |= P_IsNonASCII(str[i]);
			if (needsQuote)
				KS__EmitFixedStr(bufPtr, bufEnd, "\"\"\"", 3);
			else
				KS__EmitFixedStr(bufPtr, bufEnd, "\"", 1);
			KS__EmitStr(bufPtr, bufEnd, str);
			if (needsQuote)
				KS__EmitFixedStr(bufPtr, bufEnd, "\"\"\"", 3);
			else
				KS__EmitFixedStr(bufPtr, bufEnd, "\"", 1);
		}
		break;
	}

	case ARRAY:
	{
		u32 numElems = KS_ArrayCount(ks, value);
		KS__EmitStr(bufPtr, bufEnd, "[");
		for (u32 i = 0; i < numElems; i++)
		{
			KS__EmitNewlineAndIndent(bufPtr, bufEnd, pretty, indent);
			KS__ValueToString_r(ks, KS_ArrayElem(ks, value, i), bufPtr, bufEnd, pretty, indent + 1);
			if (!pretty)
				KS__EmitStr(bufPtr, bufEnd, ", ");
		}
		KS__EmitNewlineAndIndent(bufPtr, bufEnd, pretty, indent - 1);
		KS__EmitStr(bufPtr, bufEnd, "]");
		break;
	}

	case OBJECT:
		u32 numElems = KS_ObjectCount(ks, value);
		if (indent > 0)
			KS__EmitStr(bufPtr, bufEnd, "{");
		for (u32 i = 0; i < numElems; i++)
		{
			KeyValue kv = KS_ObjectElemKeyValue(ks, value, i);
			KS__EmitNewlineAndIndent(bufPtr, bufEnd, pretty, indent);
			KS__ValueToString_r(ks, kv.key, bufPtr, bufEnd, pretty, indent);
			KS__EmitFixedStr(bufPtr, bufEnd, " = ", 3);
			KS__ValueToString_r(ks, kv.value, bufPtr, bufEnd, pretty, indent + 1);
			if (!pretty)
				KS__EmitStr(bufPtr, bufEnd, ", ");
		}
		if (indent > 0)
		{
			KS__EmitNewlineAndIndent(bufPtr, bufEnd, pretty, indent - 1);
			KS__EmitStr(bufPtr, bufEnd, "}");
		}
		break;
		break;
	}

	return (u32)(*bufPtr - start);
}

u32 KS_ValueToString(const KeyStore *ks, ValueRef value, const char *buffer, size_t bufSize, bool pretty)
{
	char *      curBufPos = (char *)buffer;
	const char *endBufPos = buffer + bufSize;
	return KS__ValueToString_r(ks, value, &curBufPos, endBufPos, pretty, 0);
}

void KS_DumpToFile(const KeyStore *ks, const char *fileName) {}

ValueRef KS_AddInt(KeyStore **ksp, IntValue val, ValueRef curRef)
{
	if (curRef == NilValue)
	{
		return KS__Write(ksp, ValueType::INT, &val, sizeof(IntValue), 0);
	}
	else
	{
		Assert(ValueRefType(curRef) == ValueType::INT);
		*(IntValue *)(KS__ValuePtr(*ksp, curRef)) = val;
		return curRef;
	}
}

ValueRef KS_AddReal(KeyStore **ksp, RealValue val, ValueRef curRef)
{
	if (curRef == NilValue)
	{
		return KS__Write(ksp, ValueType::REAL, &val, sizeof(RealValue), 0);
	}
	else
	{
		Assert(ValueRefType(curRef) == ValueType::REAL);
		*(RealValue *)(KS__ValuePtr(*ksp, curRef)) = val;
		return curRef;
	}
}

ValueRef KS_AddString(KeyStore **ksp, const char *str, ValueRef curRef)
{
	size_t len = strlen(str) + 1; // null terminated
	if (curRef != NilValue)
	{
		Assert(ValueRefType(curRef) == ValueType::STRING);
		char * oldStr = (char *)KS__ValuePtr(*ksp, curRef);
		size_t oldLen = strlen(oldStr) + 1;
		if (oldLen >= len)
		{
			memcpy(oldStr, str, len);
			return curRef;
		}
	}

	return KS__Write(ksp, ValueType::STRING, str, len, 0);
}

ValueRef KS_AddSymbol(KeyStore **ksp, Symbol s)
{
	return MakeValueRef(s, ValueType::SYMBOL);
}

ValueRef KS_AddSymbol(KeyStore **ksp, const char *str)
{
	const Symbol sym = ST_Intern(gks->symbolTable, str);
	Assert(sym != QI_ST_FULL);
	return MakeValueRef(sym, ValueType::SYMBOL);
}

ValueRef KS_AddArray(KeyStore **ksp, u32 count)
{
	if (count < 4)
		count = 4;
	DataBlock tmpBlock = {
#if HAS(DEV_BUILD)
		ValueType::ARRAY,
#endif
		0,
		count,
		0
	};
	return KS__Write(ksp, ValueType::ARRAY, &tmpBlock, sizeof(tmpBlock), count * sizeof(ValueRef));
}

ValueRef KS_AddArray(KeyStore **ksp, ValueRef *initial, u32 count, u32 extra)
{
	ValueRef   arr     = KS_AddArray(ksp, count + extra);
	DataBlock *db      = KS__GetBlock(*ksp, arr);
	u8 *       dataPtr = (u8 *)(KS__GetBlockDataPtr(*ksp, db));
	memcpy(dataPtr, initial, count * sizeof(ValueRef));
	db->usedElems = count;
	return arr;
}

u32 KS_ArrayCount(const KeyStore *ks, ValueRef array)
{
	u32        total = 0;
	DataBlock *db    = KS__GetBlock(ks, array);
#if HAS(DEV_BUILD)
	Assert(db->type == ValueType::ARRAY);
#endif

	total += db->usedElems;
	while (db->nextBlock != 0)
	{
		db = KS__GetBlock(ks, db->nextBlock);
		total += db->usedElems;
	}

	return total;
}

ValueRef *KS__ArrayElemPtr(KeyStore *ks, ValueRef array, u32 elem)
{
	u32        curElem = elem;
	DataBlock *db      = KS__GetBlock(ks, array);
#if HAS(DEV_BUILD)
	Assert(db->type == ValueType::ARRAY);
#endif

	while (curElem > db->usedElems && db->nextBlock != 0)
	{
		curElem -= db->usedElems;
		db = KS__GetBlock(ks, db->nextBlock);
	}

	// Unsure what the semantics should be here - maybe it would be better to assert if OOB rather than return nil.
	// There's currently no distinction between an out of bounds access and a nil value in the array, which is
	// legitimate.
	if (curElem > db->usedElems)
	{
		return nullptr;
	}

	ValueRef *blockPtr = KS__GetBlockDataPtr(ks, db);
	return &blockPtr[curElem];
}

ValueRef KS_ArrayElem(const KeyStore *ks, ValueRef array, u32 elem)
{
	ValueRef *refPtr = KS__ArrayElemPtr((KeyStore *)ks, array, elem);
	return refPtr == nullptr ? NilValue : *refPtr;
}

void KS_ArraySet(KeyStore *ks, ValueRef array, u32 elem, ValueRef value)
{
	ValueRef *refPtr = KS__ArrayElemPtr(ks, array, elem);
	Assert(refPtr != nullptr);
	if (refPtr != nullptr)
	{
		*refPtr = value;
	}
}

void KS_ArrayPush(KeyStore **ksp, ValueRef array, ValueRef value)
{
	DataBlock *db = KS__GetBlock(*ksp, array);
#if HAS(DEV_BUILD)
	Assert(db->type == ValueType::ARRAY);
#endif
	while (db->usedElems == db->sizeElems)
	{
		if (db->nextBlock == 0)
		{
			ValueRef newBlock = KS_AddArray(ksp, db->sizeElems * 2);
			db->nextBlock     = newBlock;
			db                = KS__GetBlock(*ksp, newBlock);
			break;
		}
		db = KS__GetBlock(*ksp, db->nextBlock);
	}
	ValueRef *blockPtr      = KS__GetBlockDataPtr(*ksp, db);
	blockPtr[db->usedElems] = value;
	db->usedElems++;
}

ValueRef KS_AddObject(KeyStore **ksp, u32 count)
{
	if (count < 4)
		count = 4;
	DataBlock tmpBlock = {
#if HAS(DEV_BUILD)
		ValueType::OBJECT,
#endif
		0,
		count,
		0
	};
	return KS__Write(ksp, ValueType::OBJECT, &tmpBlock, sizeof(tmpBlock), count * sizeof(KeyValue));
}

ValueRef KS_AddObject(KeyStore **ksp, KeyValue *initial, u32 count, u32 extra)
{
	ValueRef   obj     = KS_AddObject(ksp, count + extra);
	DataBlock *db      = KS__GetBlock(*ksp, obj);
	u8 *       dataPtr = (u8 *)(KS__GetBlockDataPtr(*ksp, db));
	memcpy(dataPtr, initial, count * sizeof(KeyValue));
	db->usedElems = count;
	return obj;
}

u32 KS_ObjectCount(const KeyStore *ks, ValueRef object)
{
	u32        total = 0;
	DataBlock *db    = KS__GetBlock(ks, object);
#if HAS(DEV_BUILD)
	Assert(db->type == ValueType::OBJECT);
#endif

	total += db->usedElems;
	while (db->nextBlock != 0)
	{
		db = KS__GetBlock(ks, db->nextBlock);
		total += db->usedElems;
	}

	return total;
}

KeyValue *KS__ObjectElemPtr(KeyStore *ks, ValueRef object, u32 elem)
{
	u32        curElem = elem;
	DataBlock *db      = KS__GetBlock(ks, object);
#if HAS(DEV_BUILD)
	Assert(db->type == ValueType::OBJECT);
#endif

	while (curElem > db->usedElems && db->nextBlock != 0)
	{
		curElem -= db->usedElems;
		db = KS__GetBlock(ks, db->nextBlock);
	}

	// Unsure what the semantics should be here - maybe it would be better to assert if OOB rather than return nil.
	// There's currently no distinction between an out of bounds access and a nil value in the array, which is
	// legitimate.
	if (curElem > db->usedElems)
	{
		return nullptr;
	}

	KeyValue *blockPtr = (KeyValue *)KS__GetBlockDataPtr(ks, db);
	return &blockPtr[curElem];
}

KeyValue KS_ObjectElemKeyValue(const KeyStore *ks, ValueRef object, u32 elem)
{
	KeyValue *refPtr = KS__ObjectElemPtr((KeyStore *)ks, object, elem);
	return refPtr == nullptr ? KeyValue() : *refPtr;
}

ValueRef KS_ObjectElemKey(KeyStore *ks, ValueRef object, u32 elem)
{
	KeyValue *refPtr = KS__ObjectElemPtr((KeyStore *)ks, object, elem);
	return refPtr == nullptr ? NilValue : refPtr->key;
}

ValueRef KS_ObjectElemValue(KeyStore *ks, ValueRef object, u32 elem)
{
	KeyValue *refPtr = KS__ObjectElemPtr((KeyStore *)ks, object, elem);
	return refPtr == nullptr ? NilValue : refPtr->value;
}

KeyValue *KS__ObjectFindKey(const KeyStore *ks, const ValueRef object, const ValueRef key)
{
	ValueRef nextBlock = object;
	do
	{
		DataBlock *db = KS__GetBlock(ks, nextBlock);
		KeyValue * kv = (KeyValue *)KS__GetBlockDataPtr(ks, db);
		for (u32 i = 0; i < db->usedElems; i++)
		{
			if (kv->key == key)
			{
				return kv;
			}
		}
		nextBlock = db->nextBlock;
	} while (nextBlock != NilValue);

	return nullptr;
}

void KS_ObjectSetValue(KeyStore **ksp, ValueRef object, const char *key, ValueRef value)
{
	const Symbol sym = ST_Intern(gks->symbolTable, key);
	Assert(sym != QI_ST_FULL);
	const ValueRef keyVal = MakeValueRef(sym, ValueType::SYMBOL);
	KS_ObjectSetValue(ksp, object, keyVal, value);
}

void KS_ObjectSetValue(KeyStore **ksp, ValueRef object, ValueRef key, ValueRef value)
{
	KeyValue *kv = KS__ObjectFindKey(*ksp, object, key);
	if (kv != nullptr)
	{
		kv->value = value;
		return;
	}

	DataBlock *db = KS__GetBlock(*ksp, object);
	while (db->usedElems == db->sizeElems)
	{
		if (db->nextBlock == NilValue)
		{
			u32 newSize = db->sizeElems * 2;
			if (newSize < 10)
				newSize = 10;
			ValueRef newBlock = KS_AddObject(ksp, newSize);
			db->nextBlock     = newBlock;
			db                = KS__GetBlock(*ksp, newBlock);
			break;
		}
		db = KS__GetBlock(*ksp, db->nextBlock);
	}

	kv                = (KeyValue *)KS__GetBlockDataPtr(*ksp, db);
	kv[db->usedElems] = {key, value};
	db->usedElems++;
}

ValueRef KS_ObjectGetValue(const KeyStore *ks, ValueRef object, const char *key)
{
	const Symbol sym = ST_Intern(gks->symbolTable, key);
	Assert(sym != QI_ST_FULL);
	const ValueRef keyVal = MakeValueRef(sym, ValueType::SYMBOL);
	return KS_ObjectGetValue(ks, object, keyVal);
}

ValueRef KS_ObjectGetValue(const KeyStore *ks, ValueRef object, ValueRef keyVal)
{
	const KeyValue *kv = KS__ObjectFindKey(ks, object, keyVal);
	return kv == nullptr ? NilValue : kv->value;
}

static ValueRef KS__CopyValue(KeyStore **destp, const KeyStore *src, ValueRef srcVal)
{
	switch (ValueRefType(srcVal))
	{
	case SMALLINT:
		return KS_AddSmallInt(destp, KS_ValueSmallInt(src, srcVal));
	case INT:
		return KS_AddInt(destp, KS_ValueInt(src, srcVal));
	case REAL:
		return KS_AddReal(destp, KS_ValueReal(src, srcVal));
	case STRING:
		return KS_AddString(destp, KS_ValueString(src, srcVal));
	case ARRAY:
	{
		u32      arrayCount = KS_ArrayCount(src, srcVal);
		ValueRef newArr     = KS_AddArray(destp, arrayCount);

		ValueRef *newVals = KS__ArrayElemPtr(*destp, newArr, 0);
		for (u32 i = 0; i < arrayCount; i++)
		{
			newVals[i] = KS__CopyValue(destp, src, KS_ArrayElem(src, srcVal, i));
		}

		return newArr;
	}
	case OBJECT:
	{
		u32      objCount = KS_ObjectCount(src, srcVal);
		ValueRef newObj   = KS_AddObject(destp, objCount);

		KeyValue *newKVs = KS__ObjectElemPtr(*destp, newObj, 0);
		for (u32 i = 0; i < objCount; i++)
		{
			KeyValue kv     = KS_ObjectElemKeyValue(src, srcVal, i);
			newKVs[i].key   = KS__CopyValue(destp, src, kv.key);
			newKVs[i].value = KS__CopyValue(destp, src, kv.value);
		}

		return newObj;
	}
	default:
		return srcVal;
	}

	return NilValue;
}

KeyStore *KS_CompactCopy(const KeyStore *ks)
{
	KeyStore *newKS = KS_Create(ST_ToString(gks->symbolTable, ks->name), 0, 0);
	KS_SetRoot(newKS, KS__CopyValue(&newKS, ks, KS_Root(ks)));
	return newKS;
}

SmallIntValue KS_GetKeySmallInt(const KeyStore *ks, ValueRef object, const char *key)
{
	return KS_GetKeySmallInt(ks, object, ST_Intern(gks->symbolTable, key));
}

SmallIntValue KS_GetKeySmallInt(const KeyStore *ks, ValueRef object, Symbol key)
{
	ValueRef  keySym = MakeValueRef(key, ValueType::SYMBOL);
	KeyValue *kv     = KS__ObjectFindKey(ks, object, keySym);
	if (kv)
	{
		Assert(ValueRefType(kv->value) == ValueType::SMALLINT);
		return KS_ValueSmallInt(ks, kv->value);
	}
	return (SmallIntValue)0;
}

void KS_SetKeySmallInt(KeyStore **ksp, ValueRef object, const char *key, IntValue val) {}
void KS_SetKeySmallInt(KeyStore **ksp, ValueRef object, Symbol key) {}

IntValue KS_GetKeyInt(const KeyStore *ks, ValueRef object, const char *key)
{
	return KS_GetKeyInt(ks, object, ST_Intern(gks->symbolTable, key));
}

IntValue KS_GetKeyInt(const KeyStore *ks, ValueRef object, Symbol key)
{
	ValueRef  keySym = MakeValueRef(key, ValueType::SYMBOL);
	KeyValue *kv     = KS__ObjectFindKey(ks, object, keySym);
	if (kv)
	{
		Assert(ValueRefType(kv->value) == ValueType::INT);
		return KS_ValueInt(ks, kv->value);
	}
	return (IntValue)0;
}

RealValue KS_GetKeyReal(const KeyStore *ks, ValueRef object, const char *key)
{
	return KS_GetKeyReal(ks, object, ST_Intern(gks->symbolTable, key));
}

RealValue KS_GetKeyReal(const KeyStore *ks, ValueRef object, Symbol key)
{
	ValueRef  keySym = MakeValueRef(key, ValueType::SYMBOL);
	KeyValue *kv     = KS__ObjectFindKey(ks, object, keySym);
	if (kv)
	{
		Assert(ValueRefType(kv->value) == ValueType::REAL);
		return KS_ValueReal(ks, kv->value);
	}
	return (RealValue)0.0;
}

const char *KS_GetKeyString(const KeyStore *ks, ValueRef object, const char *key)
{
	return KS_GetKeyString(ks, object, ST_Intern(gks->symbolTable, key));
}

const char *KS_GetKeyString(const KeyStore *ks, ValueRef object, Symbol key)
{
	ValueRef  keySym = MakeValueRef(key, ValueType::SYMBOL);
	KeyValue *kv     = KS__ObjectFindKey(ks, object, keySym);
	if (kv)
	{
		Assert(ValueRefType(kv->value) == ValueType::STRING);
		return KS_ValueString(ks, kv->value);
	}
	return nullptr;
}

bool KS_GetKeyBool(const KeyStore *ks, ValueRef object, const char *key)
{
	return KS_GetKeyBool(ks, object, ST_Intern(gks->symbolTable, key));
}

bool KS_GetKeyBool(const KeyStore *ks, ValueRef object, Symbol key)
{
	ValueRef  keySym = MakeValueRef(key, ValueType::SYMBOL);
	KeyValue *kv     = KS__ObjectFindKey(ks, object, keySym);
	if (kv)
	{
		Assert(kv->value == TrueValue || kv->value == FalseValue);
		return kv->value == TrueValue;
	}
	return FalseValue;
}

const char *KS_GetKeyAsString(const KeyStore *ks, ValueRef object, const char *key, ValueType *typePtr)
{
	return KS_GetKeyAsString(ks, object, ST_Intern(gks->symbolTable, key), typePtr);
}

const char *KS_GetKeyAsString(const KeyStore *ks, ValueRef object, Symbol key, ValueType *typePtr)
{
	ValueRef    keySym = MakeValueRef(key, ValueType::SYMBOL);
	static char stupidBuffer[16384];
	KeyValue *  kv = KS__ObjectFindKey(ks, object, keySym);
	if (kv)
	{
		if (typePtr)
			*typePtr = ValueRefType(kv->value);
		KS_ValueToString(ks, kv->value, stupidBuffer, sizeof(stupidBuffer), false);
		return stupidBuffer;
	}
	return nullptr;
}

void KS_SetKeyInt(KeyStore **ksp, ValueRef object, const char *key, IntValue val)
{
	KS_SetKeyInt(ksp, object, ST_Intern(gks->symbolTable, key), val);
}

void KS_SetKeyInt(KeyStore **ksp, ValueRef object, Symbol key, IntValue val)
{
	ValueRef  keySym = MakeValueRef(key, ValueType::SYMBOL);
	KeyValue *kv     = KS__ObjectFindKey(*ksp, object, keySym);
	if (kv)
	{
		kv->value = KS_AddInt(ksp, val, kv->value);
	}
	else
	{
		KS_ObjectSetValue(ksp, object, key, KS_AddInt(ksp, val));
	}
}

void KS_SetKeyReal(KeyStore **ksp, ValueRef object, const char *key, RealValue val)
{
	KS_SetKeyReal(ksp, object, ST_Intern(gks->symbolTable, key), val);
}

void KS_SetKeyReal(KeyStore **ksp, ValueRef object, Symbol key, RealValue val)
{
	ValueRef  keySym = MakeValueRef(key, ValueType::SYMBOL);
	KeyValue *kv     = KS__ObjectFindKey(*ksp, object, keySym);
	if (kv)
	{
		kv->value = KS_AddInt(ksp, val, kv->value);
	}
	else
	{
		KS_ObjectSetValue(ksp, object, key, KS_AddReal(ksp, val));
	}
}

void KS_SetKeyString(KeyStore **ksp, ValueRef object, const char *key, const char *val)
{
	KS_SetKeyString(ksp, object, ST_Intern(gks->symbolTable, key), val);
}

void KS_SetKeyString(KeyStore **ksp, ValueRef object, Symbol key, const char *val)
{
	ValueRef  keySym = MakeValueRef(key, ValueType::SYMBOL);
	KeyValue *kv     = KS__ObjectFindKey(*ksp, object, keySym);
	if (kv)
	{
		kv->value = KS_AddString(ksp, val, kv->value);
	}
	else
	{
		KS_ObjectSetValue(ksp, object, key, KS_AddString(ksp, val));
	}
}

void KS_SetKeyBool(KeyStore **ksp, ValueRef object, const char *key, bool val)
{
	KS_SetKeyBool(ksp, object, ST_Intern(gks->symbolTable, key), val);
}

void KS_SetKeyBool(KeyStore **ksp, ValueRef object, Symbol key, bool val)
{
	ValueRef  keySym = MakeValueRef(key, ValueType::SYMBOL);
	KeyValue *kv     = KS__ObjectFindKey(*ksp, object, keySym);
	ValueRef  refVal = val ? TrueValue : FalseValue;
	if (kv)
	{
		kv->value = refVal;
	}
	else
	{
		KS_ObjectSetValue(ksp, object, key, refVal);
	}
}

const char *KS_SetKeyAsString(KeyStore **ksp, ValueRef object, const char *key, ValueType type, const char *val, ssize_t len)
{
	return KS_SetKeyAsString(ksp, object, ST_Intern(gks->symbolTable, key), type, val, len);
}

const char *P_ParseBuffer(KeyStore **ksp, const char *ksName, const char *buffer, size_t bufSize, ValueRef *refOut);
const char *KS_SetKeyAsString(KeyStore **ksp, ValueRef object, Symbol key, ValueType type, const char *val, ssize_t len)
{
	ValueRef result = NilValue;
	ValueRef keySym = MakeValueRef(key, ValueType::SYMBOL);

	if (len < 0)
		len = strlen(val);

	const char *errorMsg = P_ParseBuffer(ksp, nullptr, val, len, &result);
	if (errorMsg != nullptr)
		return errorMsg;

	KeyValue *kv = KS__ObjectFindKey(*ksp, object, keySym);
	if (kv)
	{
		kv->value = result;
	}
	else
	{
		KS_ObjectSetValue(ksp, object, key, result);
	}
	return nullptr;
}

BuddyAllocator *KS_GetKeyStoreAllocator()
{
	return gks->allocator;
}

#if HAS(IS_CLANG)
#pragma clang diagnostic pop
#endif

