#ifndef __QI_VECTOR_TRAITS_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Helper classes to deduce common types between two vector implementations
//

#include "template_utils.h"
#include <type_traits>

template<typename T>
struct GetVectorTypeImpl
{
};

template<typename T>
struct HasVectorTypeImpl : BoolConstant<HasType<GetVectorTypeImpl<typename RemoveReferenceCV<T>::Type>>::Value>
{
};

// Defines a type if:
// 1) Scalars have a common type and must be either one or the other of them
// 2) The sizes are equal -or- either size is 1
template<typename VectorType1,
         typename ScalarType1,
         size_t Size1,
         typename VectorType2,
         typename ScalarType2,
         size_t Size2>
struct CommonVectorTypeFallback
{
  private:
    using CommonScalarType = typename std::common_type_t<ScalarType1, ScalarType2>;

    static_assert(std::is_same<CommonScalarType, ScalarType1>::value || std::is_same<CommonScalarType, ScalarType2>::value,
                  "Error: Common scalar type between two vectors must be the scalar type of one or the other vector");

public:
  using Type = typename std::conditional_t<
	  Size1 == Size2,
      std::conditional_t<std::is_same<CommonScalarType, ScalarType1>::value, VectorType1, VectorType2>,
	  std::conditional_t<Size2 == 1, VectorType1, VectorType2>>;
};

template<typename VectorType1, typename VectorType2>
struct CommonVectorType : CommonVectorTypeFallback<VectorType1,
                                                   typename VectorType1::ScalarType,
                                                   VectorType1::Rank,
                                                   VectorType2,
                                                   typename VectorType2::ScalarType,
                                                   VectorType2::Rank>
{};

// Find the common vector type for a given combination of input types. Scalars are treated as a 1 component vector.
template<typename Head, typename... Tail>
struct GetVectorType
    : std::conditional_t<HasVectorTypeImpl<Head>::Value,
                         CommonVectorType<typename GetVectorType<Head>::Type, typename GetVectorType<Tail...>::Type>,
                         Nothing>
{};

// Terminator
template<typename Head>
struct GetVectorType<Head> : std::conditional_t<HasVectorTypeImpl<Head>::Value,
                                                GetVectorTypeImpl<typename RemoveReferenceCV<Head>::Type>,
                                                Nothing>
{};

template<typename Vector, typename = void>
struct GetVectorRank : SizeConstant<1>
{};

template<typename Vector>
struct GetVectorRank<Vector, decltype(Vector::Rank, void())> : SizeConstant<Vector::Rank>
{};

template<typename Vector>
constexpr size_t GetVectorRankFn()
{
    return GetVectorRank<Vector>::Value;
}

template<bool AllHaveRanks, typename... VectorTypes>
struct GetTotalRankHelper {};

template<typename... VectorTypes>
struct GetTotalRankHelper<true, VectorTypes...> : SizeConstant< SumSizes<GetVectorRank<VectorTypes>::Value...>::Value > {};

template<typename... VectorTypes>
struct GetTotalRank : GetTotalRankHelper< Every< HasVectorTypeImpl, VectorTypes... >::Value, VectorTypes... > {};

#define __QI_VECTOR_TRAITS_H
#endif // #ifndef __QI_VECTOR_TRAITS_H
