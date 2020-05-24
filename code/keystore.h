#ifndef __QI_KEYVALUES_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Key / value store
//

#include "basictypes.h"
#include "memory.h"

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

#define VALUE_SHIFT 3
#define VALUE_MASK  7

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

// Header for a memory block representing a top level config data store. Root will refer to a KV object or an array.
struct KeyStore;
KeyStore* KS_Create(const size_t initialSize);
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

ValueRef KS_AddInt(KeyStore** ksp, const IntValue val);
ValueRef KS_AddReal(KeyStore** ksp, const RealValue val);
ValueRef KS_AddString(KeyStore** ksp, const char* str);

ValueRef KS_AddArray(KeyStore** ksp, const u32 count);
u32      KS_ArrayCount(const KeyStore* ks, const ValueRef array);
ValueRef KS_ArrayElem(const KeyStore* ks, const ValueRef array, const u32 elem);
ValueRef KS_ArraySet(const KeyStore* ks, const ValueRef array, const u32 elem, const ValueRef value);
ValueRef KS_ArrayPush(KeyStore** ksp, const ValueRef array, const ValueRef value);

ValueRef KS_AddObject(KeyStore** ksp, const u32 count);
u32      KS_ObjectCount(const KeyStore* ks, const ValueRef object);
Symbol   KS_ObjectElemKey(const KeyStore* ks, const ValueRef object, const u32 elem);
ValueRef KS_ObjectElemValue(const KeyStore* ks, const ValueRef object, const u32 elem);
ValueRef KS_ObjectSetValue(const KeyStore* ks, const ValueRef object, const char* key, const ValueRef value);
ValueRef KS_ObjectSetValue(const KeyStore* ks, const ValueRef object, Symbol key, const ValueRef value);
ValueRef KS_ObjectGetValue(const KeyStore* ks, const ValueRef object, const char* key, const ValueRef value);
ValueRef KS_ObjectGetValue(const KeyStore* ks, const ValueRef object, Symbol key, const ValueRef value);

#define __QI_KEYVALUES_H
#endif // #ifndef __QI_KEYVALUES_H
