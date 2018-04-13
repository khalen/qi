#ifndef __QI_VECTOR_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Sub vector templates, implementing operations for a swizzleable vector of size N.
//

#include "qi_vector_traits.h"
#include "qi_vector_proxy.h"
#include "qi_vector_helper.h"
#include "qi_vector_scalar_support.h"
#include "qi_common_binops.h"
#include "qi_vector_functions.h"
#include "qi_template_utils.h"

struct Empty {};

template<typename ScalarType, size_t Size>
class Vector : public VectorHelper<ScalarType, Size>::BaseType,
               public VectorFunctions<
                   std::conditional_t<Size == 1, Nothing, CommonBinaryOps<Vector<ScalarType, Size>, ScalarType>>,
                   Vector<ScalarType, Size>,
                   ScalarType,
                   Size,
                   Vector<ScalarType, Size> const&,
                   ScalarType const&>
{
	static_assert(Size >= 1, "Size must be >= 1");

  public:
	using BaseType = typename VectorHelper<ScalarType, Size>::BaseType;
	using BaseType::data;

	using InternalScalarType     = std::remove_reference_t<decltype(std::declval<BaseType>().data[0])>;
	const static size_t Rank = Size;

	using VectorType    = Vector;
	using VectorArgType = Vector const&;
	using ScalarType    = ScalarType;
	using ScalarArgType = ScalarType const&;

	using AreScalarTypesSame = IsSame<ScalarType, InternalScalarType>;

	// Type static functions return; for Size 1 == ScalarType, else VectorType
	using DecayType = std::conditional_t<Size == 1, ScalarType, Vector>;

	// Sanity
	static_assert(sizeof(BaseType) == sizeof(ScalarType) * Size, "Unexpected size of vector base class");

	inline Vector() = default;

	// Construct from scalar implicit only if size == 1
	Vector(std::conditional_t<Size == 1, ScalarArgType, OperationNotAvailable> Init) { data[0] = Init; }

	explicit Vector(std::conditional_t<Size != 1, ScalarArgType, OperationNotAvailable> Init)
	{
		for (size_t i = 0; i < Rank; i++)
			data[i] = Init;
	}

	// I hope this gets optimized.. All of this shenanigance is to allow constructors of the form
	// Vector( va, sa, vb ) etc. where va, vb are vector types and sa is a scalar type, in any order,
	// as long as the sizes of all the arguments <= our size.
	template<typename Head,
	         typename... Tail,
	         typename = std::enable_if<Size <= GetTotalRank<Head, Tail...>::Value, void>>
	explicit Vector(Head&& InitHead, Tail&&... InitTail)
	{
		Construct<0>(std::forward<Head>(InitHead), std::forward<Tail>(InitTail)..., Nothing{});
	}

	// Construction mechanics
  private:
	template<size_t Offset, typename Head, typename... Tail>
	inline void Construct(Head&& InitHead, Tail&&... InitTail)
	{
		Compose<Offset>(::Decay(std::forward<Head>(InitHead)));
		Construct<Offset + GetTotalRank<Head>::Value>(std::forward<Tail>(InitTail)...);
	}

	template<size_t>
	inline void Construct(Nothing)
	{
	}

	template<size_t Offset>
	inline void Compose(ScalarArgType Value)
	{
		data[Offset] = Value;
	}

	template<size_t Offset, typename OtherScalarType, size_t OtherSize>
	inline void Compose(Vector<OtherScalarType, OtherSize> const& Value)
	{
		constexpr const size_t Limit = Offset + OtherSize > Size ? Size : Offset + OtherSize;
		for (size_t i = Offset; i < Limit; i++)
			data[i] = Value.at(i - Offset);
	}

  public:
	inline ScalarType&               operator[](const size_t Idx) { return at(Idx); }
	inline ScalarType const&         operator[](const size_t Idx) const { return at(Idx); }
	inline InternalScalarType&       at(const size_t Idx, TrueConstant) { return data[Idx]; }
	inline InternalScalarType const& at(const size_t Idx, TrueConstant) const { return data[Idx]; }
	inline InternalScalarType&       at(const size_t Idx, FalseConstant)
	{
		static_assert(sizeof(InternalScalarType) == sizeof(ScalarType),
		              "Size of internal scalar type must match size of external scalar type");
		return *reinterpret_cast<ScalarType*>(&data[Idx]);
	}
	inline InternalScalarType const& at(const size_t Idx, FalseConstant) const
	{
		static_assert(sizeof(InternalScalarType) == sizeof(ScalarType),
		              "Size of internal scalar type must match size of external scalar type");
		return *reinterpret_cast<ScalarType*>(&data[Idx]);
	}
	inline ScalarType&       at(const size_t Idx) { return at(Idx, AreScalarTypesSame()); }
	inline ScalarType const& at(const size_t Idx) const { return at(Idx, AreScalarTypesSame()); }
	inline DecayType         Decay() { return static_cast<DecayType const&>(*this); }

  public:
	inline Vector& operator=(VectorArgType b)
	{
		for (size_t i = 0; i < Rank; i++)
			data[i] = b.at(i);
		return *this;
	}

	// Math ops
	inline Vector& operator+=(VectorArgType b)
	{
		for (size_t i = 0; i < Size; i++)
			data[i] += b.at(i);
		return *this;
	}
	inline Vector& operator+=(ScalarArgType b)
	{
		for (size_t i = 0; i < Size; i++)
			data[i] += b;
		return *this;
	}
	inline Vector& operator-=(VectorArgType b)
	{
		for (size_t i = 0; i < Size; i++)
			data[i] -= b.at(i);
		return *this;
	}
	inline Vector& operator-=(ScalarArgType b)
	{
		for (size_t i = 0; i < Size; i++)
			data[i] -= b;
		return *this;
	}
	inline Vector& operator*=(VectorArgType b)
	{
		for (size_t i = 0; i < Size; i++)
			data[i] *= b.at(i);
		return *this;
	}
	inline Vector& operator*=(ScalarArgType b)
	{
		for (size_t i = 0; i < Size; i++)
			data[i] *= b;
		return *this;
	}
	inline Vector& operator/=(VectorArgType b)
	{
		for (size_t i = 0; i < Size; i++)
			data[i] /= b.at(i);
		return *this;
	}
	inline Vector& operator/=(ScalarArgType b)
	{
		for (size_t i = 0; i < Size; i++)
			data[i] /= b;
		return *this;
	}
};

template<typename ScalarType, size_t Size>
struct GetVectorTypeImpl<Vector<ScalarType, Size>>
{
	using Type = Vector<ScalarType, Size>;
};

using Vector2 = Vector<r32, 2>;
using Vector3 = Vector<r32, 3>;
using Vector4 = Vector<r32, 4>;

using DVector2 = Vector<r64, 2>;
using DVector3 = Vector<r64, 3>;
using DVector4 = Vector<r64, 4>;

using IVector2 = Vector<i32, 2>;
using IVector3 = Vector<i32, 3>;
using IVector4 = Vector<i32, 4>;

using LVector2 = Vector<i64, 2>;
using LVector3 = Vector<i64, 3>;
using LVector4 = Vector<i64, 4>;

#define __QI_VECTOR_H
#endif // #ifndef __QI_VECTOR_H
