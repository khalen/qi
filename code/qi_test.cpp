//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Test stuff
//

#include "basictypes.h"

#include "qi_vector.h"

#include <stdio.h>

static_assert(sizeof(SVector<R32ScalarTraits, IdxList<0, 1, 2, 3>>) == sizeof(r32) * 4, "skjrhg");
static_assert(sizeof(Vector4) == sizeof(r32) * 4, "Bad size");

int main(int, char**)
{
	Vector4 ta = {1.0f, 2.0f, 3.0f, 4.0f};
    Vector4 tb(ta.wzyx);

    r32 v[4];
    r32 r[4];

    for (int i = 0; i < 4; i++)
    {
        v[i] = ta[i];
        r[i] = tb[i];
    }

    Vector4 td = ta + tb;
    Vector3 tc = td.xyz;

    printf("ta: %f %f %f %f  tb: %f %f %f %f\n", v[0], v[1], v[2], v[3], r[0], r[1], r[2], r[3]);
    printf("tc: %f %f %f  td: %f %f %f %f\n", tc[0], tc[1], tc[2], td[0], td[1], td[2], td[3]);
}
