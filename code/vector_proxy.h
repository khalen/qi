#ifndef __QI_VECTOR_PROXY_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Indexed proxy for a vector type, convertible to vector
//

#include "template_utils.h"
#include <utility>

// Idxs is the IdxList containing our indices to use, with possible repeats.
template <typename VecType, typename IDataType, typename Idxs>
class IndexedVectorProxy
{
    IDataType data;

  public:
    constexpr static const size_t Rank = Idxs::Rank;

    // Proxy is assignable if all indices are unique
    constexpr static bool IsAssignable = HasAllUniqueIndices<Idxs>();

	using VectorType = VecType;
	using DataType   = IDataType;
    using IndicesType = Idxs;

	VectorType Decay() const
	{
		VectorType result;
		for (size_t i = 0; i < Rank; i++)
			result[i] = data[Idxs::Indices[i]];
		return result;
	}

    // Implicit conversions to vector
	operator VectorType() { return Decay(); }
	operator VectorType() const { return Decay(); }

    // Assignment only if possible
    IndexedVectorProxy& operator=(std::conditional_t<IsAssignable, VectorType, OperationNotAvailable>const& vec)
	{
		for (size_t i = 0; i < Rank; i++)
		{
			data[Idxs::Indices[i]] = vec.at(i);
		}
        return *this;
	}

    template<typename T>
    IndexedVectorProxy& operator +=(T&& b)
	{
		return operator=(Decay() + std::forward<T>(b));
	}

    template<typename T>
    IndexedVectorProxy& operator -=(T&& b)
	{
		return operator=(Decay() - std::forward<T>(b));
	}

	template<typename T>
    IndexedVectorProxy& operator *=(T&& b)
	{
		return operator=(Decay() * std::forward<T>(b));
	}

	template<typename T>
    IndexedVectorProxy& operator /=(T&& b)
	{
		return operator=(Decay() / std::forward<T>(b));
	}
};

template<typename VecType, typename DataType, typename Idxs>
struct GetVectorTypeImpl<IndexedVectorProxy<VecType, DataType, Idxs>>
{
    using Type = VecType;
};

#define __QI_VECTOR_PROXY_H
#endif // #ifndef __QI_VECTOR_PROXY_H
