#ifndef __QI_VECTOR_FUNCTIONS_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Common vector / matrix functions
//

#include "qi_vector_traits.h"
#include <type_traits>
#include <cmath>

template<typename Base,
         typename VectorType,
         typename ScalarType,
         size_t Rank,
         typename VectorArgType = VectorType const&,
         typename ScalarArgType = ScalarType const&>
struct VectorFunctions : public Base
{
	friend ScalarType dot(VectorArgType a, VectorArgType b)
	{
		ScalarType result(0);

		for (size_t i = 0; i < VectorType::Rank; i++)
			result += a[i] * b[i];

		return result;
	}

	template<size_t R = Rank>
	friend typename std::enable_if<(R == 2), VectorType>::type perp(VectorArgType v)
	{
		return VectorType(v[1], -v[0]);
	}

	template<size_t R = Rank>
	friend typename std::enable_if<(R == 3), VectorType>::type cross(VectorArgType a, VectorArgType b)
	{
		return a.yzx * b.zxy - a.zxy * b.yzx;
	}

	friend ScalarType squared_length(VectorArgType v) { return dot(v, v); }
	friend ScalarType length(VectorArgType v) { return sqrt(squared_length(v)); }
	friend VectorType normalize(VectorArgType v)
	{
		const ScalarType invLength = rsqrt(squared_length(v));
		return v * invLength;
	}

	friend ScalarType distance(VectorArgType v0, VectorArgType v1) { return length(v1 - v0); }

	friend VectorType min(VectorArgType v, ScalarArgType s)
	{
		VectorType result{};
		for (size_t i = 0; i < Rank; i++)
			result[i] = min(v[i], s);
		return result;
	}
	friend VectorType min(VectorArgType v, VectorArgType s)
	{
		VectorType result{};
		for (size_t i = 0; i < Rank; i++)
			result[i] = min(v[i], s[i]);
		return result;
	}

	friend VectorType clamp(VectorArgType v, ScalarArgType a, ScalarArgType b)
	{
		VectorType result{};
		for (size_t i = 0; i < Rank; i++)
			result[i] = max(min(v[i], b), a);
		return result;
	}
	friend VectorType clamp(VectorArgType v, VectorArgType a, VectorArgType b)
	{
		VectorType result{};
		for (size_t i = 0; i < Rank; i++)
			result[i] = max(min(v[i], b[i]), a[i]);
		return result;
	}

	friend VectorType floor(VectorArgType v)
	{
		VectorType result{};
		for (size_t i = 0; i < Rank; i++)
			result[i] = floor(v[i]);
		return result;
	}

	friend VectorType ceil(VectorArgType v)
	{
		VectorType result{};
		for (size_t i = 0; i < Rank; i++)
			result[i] = ceil(v[i]);
		return result;
	}

	friend VectorType fract(VectorArgType v)
	{
		VectorType result{};
		for (size_t i = 0; i < Rank; i++)
			result[i] = fract(v[i]);
		return result;
	}

	friend VectorType saturate(VectorArgType v) { return clamp(v, 0, 1); }

	friend VectorType step(ScalarArgType edge, VectorArgType v)
	{
		VectorType result{};
		for (size_t i = 0; i < Rank; i++)
		{
			result[i] = step(edge, v[i]);
		}
		return result;
	}
	friend VectorType step(VectorArgType edge, VectorArgType v)
	{
		VectorType result{};
		for (size_t i = 0; i < Rank; i++)
		{
			result[i] = step(edge[i], v[i]);
		}
		return result;
	}

	friend VectorType smoothstep(ScalarArgType edge0, ScalarArgType edge1, VectorArgType v)
	{
		VectorType result{};
		for (size_t i = 0; i < Rank; i++)
		{
			auto t    = saturate((v[i] - edge0) / (edge1 - edge0));
			result[i] = t * t * (3 - 2 * t);
		}
		return result;
	}
	friend VectorType smoothstep(VectorArgType edge0, VectorArgType edge1, VectorArgType v)
	{
		VectorType result{};
		for (size_t i = 0; i < Rank; i++)
		{
			result[i] = smoothstep(edge0[i], edge1[i], v[i]);
		}
		return result;
	}
};

#define __QI_VECTOR_FUNCTIONS_H
#endif // #ifndef __QI_VECTOR_FUNCTIONS_H
