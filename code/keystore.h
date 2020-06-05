#ifndef __KEYVALUES_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Key / value store
//

#include "basictypes.h"
#include "memory.h"
#include "stringtable.h"

const u32 kKSDefaultInitialElems = 5;
const u32 kKSDefaultInitialSize  = 1024;

enum ValueType : u8
{
	NIL      = 0,
	SMALLINT = 1,
	FALSE    = 2,
	TRUE     = 4,
	INT      = 6,
	REAL     = 8,
	SYMBOL   = 10,
	STRING   = 12,
	ARRAY    = 14,
	OBJECT   = 16,
};

typedef u32 ValueRef;
typedef i32 SmallIntValue;
typedef i64 IntValue;
typedef r64 RealValue;

// Small integers are encoded directly into the ValueRef, to save space and overhead in the keystores.
// TODO: Encode small Reals into ValueRef like small int?
#define LARGEST_SMALLINT  (SmallIntValue)(0x3FFFFFFF)
#define SMALLEST_SMALLINT (SmallIntValue)(-LARGEST_SMALLINT)
#define SMALLINT_SHIFT    1

#define VALUE_SHIFT 5
#define VALUE_MASK  31

inline constexpr ValueRef MakeValueRef(const i32 offset, const ValueType type)
{
	return (ValueRef)((type == SMALLINT ? (offset << SMALLINT_SHIFT) : (offset << VALUE_SHIFT)) | type);
}

inline constexpr ValueType ValueRefType(const ValueRef ref)
{
	return (ValueType)((ref & SMALLINT) ? SMALLINT : (ref & VALUE_MASK));
}

inline constexpr size_t ValueRefOffset(const ValueRef ref)
{
	return (size_t)((ref & SMALLINT) != 0 ? 0 : (ref >> VALUE_SHIFT));
}

static_assert((i32)(MakeValueRef(LARGEST_SMALLINT, SMALLINT)) >> 1 == LARGEST_SMALLINT, "Small int encoding issue");
static_assert((i32)(MakeValueRef(SMALLEST_SMALLINT, SMALLINT)) >> 1 == SMALLEST_SMALLINT, "Small int encoding issue");

const ValueRef TrueValue  = MakeValueRef(0, ValueType::TRUE);
const ValueRef FalseValue = MakeValueRef(0, ValueType::FALSE);
const ValueRef NilValue   = MakeValueRef(0, ValueType::NIL);

struct KeyValue
{
	ValueRef key;
	ValueRef value;
};

// Header for a memory block representing a top level config data store. Root will refer to a KV object or an array.
struct KeyStore;
KeyStore *KS_Create(const char *name, u32 initialElems = kKSDefaultInitialElems, size_t initialSize = kKSDefaultInitialSize);
void      KS_Free(KeyStore **ksp);

ValueRef KS_Root(const KeyStore *ks);
void     KS_SetRoot(KeyStore *ks, ValueRef root);

StringTable *KS_GetStringTable();
u32          KS_ValueToString(const KeyStore *ks, ValueRef value, const char *buffer, size_t bufSize, bool pretty);
// Allocates and returns a new keystore based on the parameter. Will result in an optimally sized copy (no extra
// space in objects/arrays, no unreferenced int/string/real values, etc.
KeyStore *KS_CompactCopy(const KeyStore *ks);

inline constexpr ValueRef KS_AddSmallInt(KeyStore **, SmallIntValue val)
{
	return MakeValueRef(val, SMALLINT);
}
ValueRef KS_AddInt(KeyStore **ksp, IntValue val, ValueRef prevRef = NilValue);
ValueRef KS_AddReal(KeyStore **ksp, RealValue val, ValueRef prevRef = NilValue);
ValueRef KS_AddString(KeyStore **ksp, const char *str, ValueRef prevRef = NilValue);

ValueRef KS_AddSymbol(KeyStore **ksp, const char *str);
ValueRef KS_AddSymbol(KeyStore **ksp, Symbol sym);

ValueRef KS_AddArray(KeyStore **ksp, u32 count);
ValueRef KS_AddArray(KeyStore **ksp, ValueRef *initialValues, u32 initialCount, u32 extraCount);
u32      KS_ArrayCount(const KeyStore *ks, ValueRef array);
ValueRef KS_ArrayElem(const KeyStore *ks, ValueRef array, u32 elem);
void     KS_ArraySet(KeyStore *ks, ValueRef array, u32 elem, ValueRef value);
void     KS_ArrayPush(KeyStore **ksp, ValueRef array, ValueRef value);

ValueRef KS_AddObject(KeyStore **ksp, u32 count);
ValueRef KS_AddObject(KeyStore **ksp, KeyValue *initialKeyValues, u32 initialCount, u32 extraCount);
u32      KS_ObjectCount(const KeyStore *ks, ValueRef object);

// Simple helpers for common cases
SmallIntValue KS_GetKeySmallInt(const KeyStore *ks, ValueRef object, const char *key);
SmallIntValue KS_GetKeySmallInt(const KeyStore *ks, ValueRef object, Symbol key);
IntValue      KS_GetKeyInt(const KeyStore *ks, ValueRef object, const char *key);
IntValue      KS_GetKeyInt(const KeyStore *ks, ValueRef object, Symbol key);
RealValue     KS_GetKeyReal(const KeyStore *ks, ValueRef object, const char *key);
RealValue     KS_GetKeyReal(const KeyStore *ks, ValueRef object, Symbol key);
const char *  KS_GetKeyString(const KeyStore *ks, ValueRef object, const char *key);
const char *  KS_GetKeyString(const KeyStore *ks, ValueRef object, Symbol key);
bool          KS_GetKeyBool(const KeyStore *ks, ValueRef object, const char *key);
bool          KS_GetKeyBool(const KeyStore *ks, ValueRef object, Symbol key);
const char *  KS_GetKeyAsString(const KeyStore *ks, ValueRef object, const char *key, ValueType *typePtr = nullptr);
const char *  KS_GetKeyAsString(const KeyStore *ks, ValueRef object, Symbol key, ValueType *typePtr = nullptr);

void KS_SetKeySmallInt(KeyStore **ksp, ValueRef object, const char *key, IntValue val);
void KS_SetKeySmallInt(KeyStore **ksp, ValueRef object, Symbol key);
void KS_SetKeyInt(KeyStore **ksp, ValueRef object, const char *key, IntValue val);
void KS_SetKeyInt(KeyStore **ksp, ValueRef object, Symbol key, IntValue val);
void KS_SetKeyReal(KeyStore **ksp, ValueRef object, const char *key, RealValue val);
void KS_SetKeyReal(KeyStore **ksp, ValueRef object, Symbol key, RealValue val);
void KS_SetKeyString(KeyStore **ksp, ValueRef object, const char *key, const char *val);
void KS_SetKeyString(KeyStore **ksp, ValueRef object, Symbol key, const char *val);
void KS_SetKeyBool(KeyStore **ksp, ValueRef object, const char *key, bool val);
void KS_SetKeyBool(KeyStore **ksp, ValueRef object, Symbol key, bool val);

// These both try to "DWIM" based on the key type, ie. object will parse QED syntax using P_ParseValue()
const char *KS_SetKeyAsString(KeyStore **ksp, ValueRef object, const char *key, ValueType type, const char *val, ssize_t len = -1);
const char *KS_SetKeyAsString(KeyStore **ksp, ValueRef object, Symbol key, ValueType type, const char *val, ssize_t len = -1);

KeyValue KS_ObjectElemKeyValue(const KeyStore *ks, ValueRef object, u32 elem);
ValueRef KS_ObjectElemKey(const KeyStore *ks, ValueRef object, u32 elem);
ValueRef KS_ObjectElemValue(const KeyStore *ks, ValueRef object, u32 elem);
void     KS_ObjectSetValue(KeyStore **ks, ValueRef object, const char *key, ValueRef value);
void     KS_ObjectSetValue(KeyStore **ks, ValueRef object, ValueRef keyVal, ValueRef value);
ValueRef KS_ObjectGetValue(const KeyStore *ks, ValueRef object, const char *key);
ValueRef KS_ObjectGetValue(const KeyStore *ks, ValueRef object, ValueRef keyVal);

// QED Parser

// Returns nullptr on success, error message on failure; *ksp will be created if initially null
const char *QED_LoadFile(KeyStore **ksp, const char *ksName, const char *fileName);
const char *QED_LoadBuffer(KeyStore **ksp, const char *ksName, const char *buf, size_t bufSize);

// Global data store interface
KeyStore *QED_LoadDataStore(const char *dsName);
KeyStore *QED_GetDataStore(const char *dsName);

#define __KEYVALUES_H
#endif // #ifndef __KEYVALUES_H
