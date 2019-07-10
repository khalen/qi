#ifndef __QI_KEYVALUES_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Key / value store
//

#include "basictypes.h"
#include "qi_memory.h"

enum KVType : u8
{
	NIL    = 0,
	STRING = 0,
	INT,
	REAL,
	ARRAY,
	TRUE,
	FALSE,
	KEYVALUES,
};

typedef u32 KVRef;
typedef i64 IntValue;
typedef r64 RealValue;

#define KVR_SHIFT 3
#define KVR_MASK  7

inline constexpr KVRef MakeKVRef(const u32 offset, const KVType type)
{
    return (offset << KVR_SHIFT) | type;
}

inline constexpr KVType KVRType(const KVRef ref)
{
    return (KVType) (ref & KVR_MASK);
}

inline constexpr size_t KVROffset(const KVRef ref)
{
    return (size_t) ref >> KVR_SHIFT;
}

union Value
{
	KVRef     refv;
	IntValue  intv;
	RealValue realv;
};

struct KeyValue
{
	KVRef key;
	Value value;
};

struct ValueArray
{
	IntValue len;
	IntValue allocatedLen;
	Value    values[];
};

struct KeyValues
{
    KVRef root;
    :w
}

Value KV_GetValue(KeyValue* kv, u32 idx);
Value KV_GetKey(KeyValue* kv, u32 idx);
Value KV_GetValueForKey(KeyValue* kv, const char* key);
Value KV_GetValueForKey(KeyValue* kv, i32 key);
Value KV_GetValueForKey(KeyValues* kv, const Value key);
i32 KV_CompareValues(const KeyValues* kva, const Value a, const KeyValues* kvb, const Value b);
ValueType KV_GetValueType(const Value v);
const char* KV_GetValueAsString(const KeyValues* kva, const Value v);

#define __QI_KEYVALUES_H
#endif // #ifndef __QI_KEYVALUES_H
