#ifndef __QI_KEYVALUES_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Key / value store
//

#include "basictypes.h"
#include "memory.h"
#include "stringtable.h"

const u32 kKSDefaultInitialElems = 5;
const u32 kKSDefaultInitialSize = 1024;

enum ValueType : u8
{
	NIL = 0,
	FALSE,
	TRUE,
	INT,
	REAL,
	SYMBOL,
	STRING,
	ARRAY,
	OBJECT,
};

typedef u32 ValueRef;
typedef i64 IntValue;
typedef r64 RealValue;

#define VALUE_SHIFT 4
#define VALUE_MASK  15

inline constexpr ValueRef
MakeValueRef(const u32 offset, const ValueType type)
{
	return (offset << VALUE_SHIFT) | type;
}

inline constexpr ValueType
ValueRefType(const ValueRef ref)
{
	return (ValueType)(ref & VALUE_MASK);
}

inline constexpr size_t
ValueRefOffset(const ValueRef ref)
{
	return (size_t)ref >> VALUE_SHIFT;
}

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
KeyStore* KS_Create(const char* name, const u32 initialElems = kKSDefaultInitialElems, const size_t initialSize = kKSDefaultInitialSize);
void      KS_Free(KeyStore** ksp);

ValueRef KS_Root(const KeyStore* ks);
void     KS_SetRoot(KeyStore* ks, ValueRef root);

inline constexpr ValueRef
KS_Nil()
{
	return NilValue;
}

inline constexpr ValueRef
KS_True()
{
	return TrueValue;
}

inline constexpr ValueRef
KS_False()
{
	return FalseValue;
}

u32 KS_ValueToString(const KeyStore *ks, ValueRef value, const char *buffer, size_t bufSize, bool pretty);

ValueRef KS_AddInt(KeyStore** ksp, IntValue val);
ValueRef KS_AddReal(KeyStore** ksp, RealValue val);
ValueRef KS_AddString(KeyStore** ksp, const char* str);

ValueRef KS_AddSymbol(KeyStore **ksp, const char *str);
ValueRef KS_AddSymbol(KeyStore **ksp, Symbol sym);

ValueRef KS_AddArray(KeyStore** ksp, u32 count);
ValueRef KS_AddArray(KeyStore **ksp, ValueRef* initialValues, u32 initialCount, u32 extraCount);
u32      KS_ArrayCount(const KeyStore* ks, ValueRef array);
ValueRef KS_ArrayElem(const KeyStore* ks, ValueRef array, u32 elem);
void     KS_ArraySet(const KeyStore* ks, ValueRef array, u32 elem, ValueRef value);
void     KS_ArrayPush(KeyStore** ksp, ValueRef array, ValueRef value);

ValueRef KS_AddObject(KeyStore** ksp, u32 count);
ValueRef KS_AddObject(KeyStore **ksp, KeyValue *initialKeyValues, u32 initialCount, u32 extraCount);
u32      KS_ObjectCount(const KeyStore* ks, ValueRef object);

KeyValue KS_ObjectElemKeyValue(const KeyStore *ks, ValueRef object, u32 elem);
ValueRef KS_ObjectElemKey(const KeyStore* ks, ValueRef object, u32 elem);
ValueRef KS_ObjectElemValue(const KeyStore* ks, ValueRef object, u32 elem);
void     KS_ObjectSetValue(KeyStore** ks, ValueRef object, const char* key, ValueRef value);
void     KS_ObjectSetValue(KeyStore** ks, ValueRef object, ValueRef keyVal, ValueRef value);
ValueRef KS_ObjectGetValue(const KeyStore* ks, ValueRef object, const char* key);
ValueRef KS_ObjectGetValue(const KeyStore* ks, ValueRef object, ValueRef keyVal);

// QED Parser

// Returns nullptr on success, error message on failure; *ksp will be created if initially null
const char* QED_LoadFile(KeyStore** ksp, const char* ksName, const char* fileName);
const char* QED_LoadBuffer(KeyStore** ksp, const char* ksName, const char* buf, size_t bufSize);

#define __QI_KEYVALUES_H
#endif // #ifndef __QI_KEYVALUES_H

