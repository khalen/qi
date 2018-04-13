#ifndef __QI_VECTOR_HELPER_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Simplifies creation of vector base types and indexed proxies
//

#include "qi_template_utils.h"
#include "qi_vector_proxy.h"
#include "qi_vector_base.h"

template <typename ScalarType, size_t Size> class Vector;

template <typename ScalarType, size_t Size>
struct VectorHelper
{
    using DataType = ScalarType[Size];

    template <size_t... Indices>
    struct ProxyGenerator
    {
        using Type = IndexedVectorProxy< Vector< ScalarType, sizeof...(Indices) >, DataType, IdxList<Indices...> >;
    };

    template<size_t X>
    struct ProxyGenerator<X>
    {
        using Type = ScalarType;
    };

    using BaseType = VectorBase<Size, ProxyGenerator, DataType>;
};


#define __QI_VECTOR_HELPER_H
#endif // #ifndef __QI_VECTOR_HELPER_H
