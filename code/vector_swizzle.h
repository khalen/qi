#ifndef __QI_VECTOR_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Sub vector templates, implementing operations for a swizzleable vector of size N.
//

#include "qi_vector_internal.h"
#include <cmath>
#include <xmmintrin.h>

struct R32ScalarTraits
{
    using Type = r32;
    constexpr static const Type Zero = 0.0f;
    constexpr static const Type One = 1.0f;

	inline Type const Sqrt(const Type val) { return std::sqrt(val); }
	inline Type const FastInvSqrt(const Type val)
	{
        return _mm_cvtss_f32( _mm_rsqrt_ss( _mm_set_ps1(val) ) );
    }
};

struct I32ScalarTraits
{
    using Type = i32;
    constexpr static const Type Zero = 0;
    constexpr static const Type One = 1;

	inline Type const Sqrt(const Type val) { return static_cast<Type>(std::round(std::sqrt(val))); }
};

struct I64ScalarTraits
{
    using Type = i64;
    constexpr static const Type Zero = 0ll;
    constexpr static const Type One = 1ll;;

	inline Type const Sqrt(const Type val) { return static_cast<Type>(std::round(std::sqrt(val))); }
};

// Actual VectorN types, implementing swizzle unions
template <typename Traits>
struct GVec2
{
    using ScalarType = typename Traits::Type;

    template<size_t... Idxs>
    using SubVec = SVector<Traits, IdxList<Idxs...>>;

    GVec2(GVec2 const& Init)
	{
		this->x = Init.x;
		this->y = Init.y;
	}

	GVec2(ScalarType const& i0, ScalarType const& i1)
	{
		this->x = i0;
		this->y = i1;
	}

    template<size_t... Idxs, typename = std::enable_if<SubVec<Idxs...>::Rank >= 2, void>>
    GVec2(SubVec<Idxs...> const& Init)
	{
		this->x = Init[0];
		this->y = Init[1];
	}

    ScalarType& operator[](const size_t Idx) { return data[Idx]; };
    ScalarType const & operator[](const size_t Idx) const { return data[Idx]; };

    operator SubVec<0, 1>&() { return this->xy; }
    operator SubVec<0, 1> const& () const { return this->xy; }

	union
    {
        ScalarType data[2];

        struct { r32 x, y; };
        struct { r32 r, g; };

        SubVec<0, 1> xy, rg;
        SubVec<1, 0> yx, gr;
    };
};

template <typename Traits>
struct GVec3
{
    using ScalarType = typename Traits::Type;

    template<size_t... Idxs>
    using SubVec = SVector<Traits, IdxList<Idxs...>>;

    GVec3(GVec3 const& Init)
	{
		this->x = Init.x;
		this->y = Init.y;
		this->z = Init.z;
	}

	GVec3(ScalarType const& i0, ScalarType const& i1, ScalarType const& i2)
	{
		this->x = i0;
		this->y = i1;
		this->z = i2;
	}

	template<size_t... Idxs, typename = std::enable_if<SubVec<Idxs...>::Rank >= 3, void>>
    GVec3(SubVec<Idxs...> const& Init)
	{
		this->x = Init[0];
		this->y = Init[1];
		this->z = Init[2];
	}

    ScalarType& operator[](const size_t Idx) { return data[Idx]; }
    ScalarType const & operator[](const size_t Idx) const { return data[Idx]; }

    operator SubVec<0, 1, 2>() { return this->xyz; }
    operator SubVec<0, 1, 2> const () const { return this->xyz; }

    union
    {
        ScalarType data[3];
		struct
		{
			r32 x, y, z;
		};
		struct
        {
			r32 r, g, b;
		};

        SubVec<0, 1> xy, rg;
        SubVec<0, 2> xz, rb;
        SubVec<1, 0> yx, gr;
        SubVec<1, 2> yz, gb;
        SubVec<2, 0> zx, br;
        SubVec<2, 1> zy, bg;

        SubVec<0, 1, 2> xyz, rgb;
        SubVec<0, 2, 1> xzy, rbg;
        SubVec<1, 0, 2> yxz, grb;
        SubVec<1, 2, 0> yzx, gbr;
        SubVec<2, 0, 1> zxy, brg;
        SubVec<2, 1, 0> zyx, bgr;
    };
};

template <typename Traits>
struct GVec4
{
    using ScalarType = typename Traits::Type;

    template<size_t... Idxs>
    using SubVec = SVector<Traits, IdxList<Idxs...>>;

    GVec4(GVec4 const& Init)
        {
            this->x = Init.x;
            this->y = Init.y;
            this->z = Init.z;
        }

	GVec4(ScalarType const& i0, ScalarType const& i1, ScalarType const& i2, ScalarType const& i3)
	{
		this->x = i0;
		this->y = i1;
		this->z = i2;
		this->w = i3;
	}

	template<size_t... Idxs, typename = std::enable_if<SubVec<Idxs...>::Rank >= 4, void>>
    GVec4(SubVec<Idxs...> const& Init)
	{
		this->x = Init[0];
		this->y = Init[1];
		this->z = Init[2];
		this->w = Init[3];
	}

    ScalarType& operator[](const size_t Idx) { return data[Idx]; };
    ScalarType const & operator[](const size_t Idx) const { return data[Idx]; };

    operator SubVec<0, 1, 2, 3>& () { return this->xyzw; }
    operator SubVec<0, 1, 2, 3> const& () const { return this->xyzw; }

    union
    {
        ScalarType data[4];
        struct {
			r32 x, y, z, w;
		};
		struct
		{
			r32 r, g, b, a;
		};

        SubVec<0, 1> xy, rg;
        SubVec<0, 2> xz, rb;
        SubVec<0, 3> xw, ra;
        SubVec<1, 0> yx, gr;
        SubVec<1, 2> yz, gb;
        SubVec<1, 3> yw, ga;
        SubVec<2, 0> zx, br;
        SubVec<2, 1> zy, bg;
        SubVec<2, 3> zw, ba;

        SubVec<0, 1, 2> xyz, rgb;
        SubVec<0, 1, 3> xyw, rga;
        SubVec<0, 2, 1> xzy, rbg;
        SubVec<0, 2, 3> xzw, rba;
        SubVec<0, 3, 1> xwy, rag;
        SubVec<0, 3, 2> xwz, rab;

		SubVec<1, 0, 2> yxz, grb;
		SubVec<1, 0, 3> yxw, gra;
        SubVec<1, 2, 0> yzx, gbr;
        SubVec<1, 2, 3> yzw, gba;
        SubVec<1, 3, 0> ywx, gar;
        SubVec<1, 3, 2> ywz, gab;

		SubVec<2, 0, 1> zxy, brg;
		SubVec<2, 0, 3> zxw, bra;
        SubVec<2, 1, 0> zyx, bgr;
        SubVec<2, 1, 3> zyw, bga;
        SubVec<2, 3, 0> zwx, bar;
        SubVec<2, 3, 1> zwy, bag;

		SubVec<3, 0, 1> wxy, arg;
		SubVec<3, 0, 2> wxz, arb;
		SubVec<3, 1, 0> wyx, agr;
		SubVec<3, 1, 2> wyz, agb;
		SubVec<3, 2, 0> wzx, abr;
		SubVec<3, 2, 1> wzy, abg;

		SubVec<0, 1, 2, 3> xyzw, rgba;
		SubVec<0, 1, 3, 2> xywz, rgab;
		SubVec<0, 2, 1, 3> xzyw, rbga;
		SubVec<0, 2, 3, 1> xzwy, rbag;
		SubVec<0, 3, 1, 2> xwyz, ragb;
		SubVec<0, 3, 2, 1> xwzy, rabg;

		SubVec<1, 0, 2, 3> yxzw, grba;
		SubVec<1, 0, 3, 2> yxwz, grab;
		SubVec<1, 2, 0, 3> yzxw, gbra;
		SubVec<1, 2, 3, 0> yzwx, gbar;
		SubVec<1, 3, 0, 2> ywxz, garb;
		SubVec<1, 3, 2, 0> ywzx, gabr;

		SubVec<2, 0, 1, 3> zxyw, brga;
		SubVec<2, 0, 3, 1> zxwy, brag;
		SubVec<2, 1, 0, 3> zyxw, bgra;
		SubVec<2, 1, 3, 0> zywx, bgar;
		SubVec<2, 3, 0, 1> zwxy, barg;
		SubVec<2, 3, 1, 0> zwyx, bagr;

		SubVec<3, 0, 1, 2> wxyz, argb;
		SubVec<3, 0, 2, 1> wxzy, arbg;
		SubVec<3, 1, 0, 2> wyxz, agrb;
		SubVec<3, 1, 2, 0> wyzx, agbr;
		SubVec<3, 2, 0, 1> wzxy, abrg;
		SubVec<3, 2, 1, 0> wzyx, abgr;
    };
};

using Vector2 = GVec2<R32ScalarTraits>;
using Vector3 = GVec3<R32ScalarTraits>;
using Vector4 = GVec4<R32ScalarTraits>;

using IVector2 = GVec2<I32ScalarTraits>;
using IVector3 = GVec3<I32ScalarTraits>;
using IVector4 = GVec4<I32ScalarTraits>;

using LVector2 = GVec2<I64ScalarTraits>;
using LVector3 = GVec3<I64ScalarTraits>;
using LVector4 = GVec4<I64ScalarTraits>;

#define __QI_VECTOR_H
#endif // #ifndef __QI_VECTOR_H
