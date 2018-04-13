#ifndef __QI_TEMPLATE_UTILS_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Template utilities
//

#include <type_traits>

struct Nothing {};

template <size_t Idx, size_t Head, size_t... Tail>
struct IdxElem
{
    inline static const size_t value = IdxElem<Idx - 1, Tail...>::value;
};

template <size_t Head, size_t... Tail>
struct IdxElem<0, Head, Tail...>
{
    inline static const size_t value = Head;
};

template <size_t... Idxs>
struct IdxList
{
    constexpr static size_t Rank = sizeof...(Idxs);
    constexpr static const size_t Indices[Rank] = { Idxs... };
};

template <typename Idxs>
inline constexpr bool HasAllUniqueIndices()
{
    for (size_t i = 0; i < Idxs::Rank; i++)
    {
        for (size_t j = i + 1; j < Idxs::Rank; j++)
        {
            if (Idxs::Indices[i] == Idxs::Indices[j])
                return false;
        }
    }

    return true;
}

template<typename IdxList>
struct NextIdxList;

template <size_t... Idxs>
struct NextIdxList<IdxList<Idxs...>>
{
    using Type = IdxList<Idxs..., sizeof...(Idxs)>;
};

template <size_t I, size_t N>
struct MakeIdxListImpl
{
    using Type = typename NextIdxList<typename MakeIdxListImpl<I + 1, N>::Type>::Type;
};

template <size_t N>
struct MakeIdxListImpl<N, N>
{
    using Type = IdxList<N>;
};

template <size_t N>
using MakeIdxList = MakeIdxListImpl<0, N>;

template <size_t Check, size_t Head, size_t... Tail>
struct Contains
{
    constexpr static bool Value = Check == Head || Contains<Head, Tail...>::Value;
};

template <size_t Check, size_t Head>
struct Contains<Check, Head>
{
    constexpr static bool Value = Check == Head;
};

template<size_t Head, size_t... Tail>
struct AreAllUnique
{
    constexpr static bool Value = !Contains<Head, Tail...>::Value && AreAllUnique<Tail...>::Value;
};

template<size_t Head, size_t Tail>
struct AreAllUnique<Head, Tail>
{
    constexpr static bool Value = Head != Tail;
};

template <typename T>
struct RemoveReferenceCV
{
    using Type = typename std::remove_cv< typename std::remove_reference<T>::type >::type;
};

// Allow conditional overloads based on whether or not a template argument has a Type parameter
template <typename T>
struct HasTypeHelper
{
    private:
    template<typename T1> static typename T1::Type test(int);
    template<typename> static void test(...);
    public:
    constexpr static bool Value = !std::is_void<decltype(test<T>(0))>::value;
};

template<typename T, T Constant>
struct ValueConstant
{
    constexpr static T Value = Constant;
};

template<size_t Constant>
using SizeConstant = ValueConstant<size_t, Constant>;

template<bool Constant>
using BoolConstant = ValueConstant<bool, Constant>;

struct TrueConstant : BoolConstant<true> {};
struct FalseConstant : BoolConstant<false> {};

template<typename T, typename U>
struct IsSame : FalseConstant
{};

template<typename T>
struct IsSame<T, T> : TrueConstant
{};

template<size_t S0, size_t S1>
struct IsGreater : BoolConstant<(S0 > S1)> {};

template <typename T>
struct HasType : BoolConstant<HasTypeHelper<T>::Value>
{};

template <size_t Head, size_t... Tail>
struct SumSizes
{
    constexpr static size_t Value = Head + SumSizes<Tail...>::Value;
};

template<size_t Head>
struct SumSizes<Head>
{
    constexpr static size_t Value = Head;
};

template<template<typename> class Predicate, typename Head, typename... Tail>
struct Every
{
    constexpr static bool Value = Predicate<Head>::Value && Every<Predicate, Tail...>::Value;
};

template<template<typename> class Predicate, typename Head>
struct Every<Predicate, Head>
{
    constexpr static bool Value = Predicate<Head>::Value;
};

struct OperationNotAvailable {};

template <typename T>
inline auto Decay(T&& t) -> decltype( t.Decay() ) { return t.Decay(); }

inline Nothing Decay(Nothing) { return Nothing(); }

template <typename T>
inline std::enable_if_t< std::is_scalar_v< std::remove_reference_t<T> >, T > Decay(T&& t)
{
    return t;
}

#define __QI_TEMPLATE_UTILS_H
#endif // #ifndef __QI_TEMPLATE_UTILS_H
