#ifndef __QI_KEYVALUES_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Key / value store
//

#include "basictypes.h"
#include "memory.h"

enum KVType : u8
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

#define KVR_SHIFT 3
#define KVR_MASK  7

inline constexpr ValueRef
MakeValueRef(const u32 offset, const KVType type)
{
	return (offset << KVR_SHIFT) | type;
}

inline constexpr KVType
ValueRefType(const ValueRef ref)
{
	return (KVType)(ref & KVR_MASK);
}

inline constexpr size_t
ValueRefOffset(const ValueRef ref)
{
	return (size_t)ref >> KVR_SHIFT;
}

const ValueRef TrueValue = MakeValueRef(0, KVType::TRUE);
const ValueRef FalseValue = MakeValueRef(0, KVType::FALSE);

// Header for a memory block representing a top level config data store. Root will refer to a KV object or an array.
struct KeyStore
{
	u32 sizeBytes;
    u32 usedBytes;
	ValueRef root;
};
void KV_Init(KeyStore* cd, void* buffer, size_t bufSize);

struct DataBlock
{
	u32 usedElems;
	u32 sizeElems;
	u32 nextBlock;
};
u32 DB_ElemCount(const DataBlock* db);

struct KeyValue
{
	ValueRef key;   // Must be symbol
	ValueRef value;
};

#define __QI_KEYVALUES_H
#endif // #ifndef __QI_KEYVALUES_H
