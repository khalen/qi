#ifndef __QI_KEYVALUES_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Key / value store
//

enum ValueType : u8
{
    STRING = 0,
    KEYVALUES,
};

struct KeyValues;

struct KeyValues
{
    ValueHeader name;
	u32 len;       // Number of key value pairs
	u32 totalSize; // Byte size of entire KV set including the initial len and header
    // Pair buffer immediately follows
};

union Value
{
    ValueHeader h;
    IntValue i;
    RealValue r;
};

Value KV_GetValue(KeyValues* kv, u32 idx);
Value KV_GetKey(KeyValues* kv, u32 idx);
Value KV_GetValueForKey(KeyValues* kv, const char* key);
Value KV_GetValueForKey(KeyValues* kv, i32 key);
Value KV_GetValueForKey(KeyValues* kv, const Value key);
i32 KV_CompareValues(const KeyValues* kva, const Value a, const KeyValues* kvb, const Value b);
ValueType KV_GetValueType(const Value v);
const char* KV_GetValueAsString(const KeyValues* kva, const Value v);

#define __QI_KEYVALUES_H
#endif // #ifndef __QI_KEYVALUES_H
