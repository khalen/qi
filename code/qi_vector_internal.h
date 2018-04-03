#ifndef __QI_VECTOR_INTERNAL_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Sub vector templates, implementing operations for a swizzleable vector of size N.
//

#include <initializer_list>
#include <array>
#include <vector>
#include <utility>

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

template <typename Traits, typename Idxs> struct SVector;
template<typename Traits, typename IdxsA, typename IdxsB> auto operator+ (SVector<Traits, IdxsA> const& a, SVector<Traits, IdxsB> const& b);
template<typename Traits, typename IdxsA, typename IdxsB> auto operator- (SVector<Traits, IdxsA> const& a, SVector<Traits, IdxsB> const& b);
template<typename Traits, typename IdxsA, typename IdxsB> auto operator* (SVector<Traits, IdxsA> const& a, SVector<Traits, IdxsB> const& b);
template<typename Traits, typename IdxsA, typename IdxsB> auto operator/ (SVector<Traits, IdxsA> const& a, SVector<Traits, IdxsB> const& b);

template <typename Traits, typename Idxs>
struct SVector
{
    using Traits = Traits;
    using ScalarType = typename Traits::Type;
    using IndicesType = Idxs;
    constexpr static const size_t Rank = IndicesType::Rank;
    constexpr static const size_t Indices = IndicesType::Indices;

    ScalarType data[Rank];

    SVector(std::initializer_list<ScalarType> Inits)
        {
            size_t i = 0;
            for (auto a : Inits)
            {
                at(i++) = a;
            }
        }

    template<typename = std::enable_if<Rank == 1, void>>
    SVector(ScalarType const& Init)
	{
		data[0] = Init;
	}

	template<typename IdxsOther>
    SVector(SVector<Traits, IdxsOther> const& other)
	{
        const size_t RankOther = IdxsOther::Rank;
		if constexpr (RankOther >= Rank)
		{
			for (int i = 0; i < Rank; i++)
			{
				at(i) = other.at(i);
			}
		}
		else
		{
			for (int i = 0; i < RankOther; i++)
			{
                at(i) = other.at(i);
			}
			for (int i = RankOther; i < RankOther; i++)
			{
                at(i) = Traits::Zero;
			}
		}
	}

    template<typename IdxsOther>
    SVector& operator=( SVector<Traits, IdxsOther>const &other )
	{
        const size_t RankOther = IdxsOther::Rank;

		if constexpr (RankOther >= Rank)
		{
			for (int i = 0; i < Rank; i++)
			{
                at(i) = other.at(i);
			}
		}
		else
		{
			for (int i = 0; i < RankOther; i++)
			{
                at(i) = other.at(i);
			}
			for (int i = RankOther; i < Rank; i++)
			{
                at(i) = Traits::Zero;
			}
		}
        return *this;
	}

    template<typename IdxsOther>
    SVector const& operator=( SVector<Traits, IdxsOther>const &other ) const
	{
        const size_t RankOther = IdxsOther::Rank;

		if constexpr (RankOther >= Rank)
		{
			for (int i = 0; i < Rank; i++)
			{
                at(i) = other.at(i);
			}
		}
		else
		{
			for (int i = 0; i < RankOther; i++)
			{
                at(i) = other.at(i);
			}
		}
		return *this;
	}

	inline ScalarType& at(const size_t idx) { return data[IndicesType::Indices[idx]]; }
	inline ScalarType const& at(const size_t idx) const { return data[IndicesType::Indices[idx]]; }
	inline ScalarType& operator[](const size_t idx) { return at(idx); }
	inline ScalarType const& operator[](const size_t idx) const { return at(idx); }

    template<typename IdxsOther, typename = std::enable_if<(IdxsOther::Rank > 1), void>>
    operator SVector<Traits, IdxsOther> const () const
	{
		return SVector<Traits, IdxsOther>(*this);
	}

	template<typename = std::enable_if<Rank == 1, void>>
	operator ScalarType const() const
	{
		return data[0];
	}

    // Rank specific operations
    template <typename = std::enable_if< Rank == 2, void >>
    inline auto perp() const
	{
        return SVector<Traits, MakeIdxList<2>::Type>(at(1), -at(0));
	}

    template <typename IdxsOther, typename = std::enable_if< Rank == 3, void >>
    inline auto cross(SVector<Traits, IdxsOther>const& b) const
	{
		return SVector<Traits, MakeIdxList<3>::Type>(
            at(1) * b.at(2) - at(2) * b.at(1),
            at(2) * b.at(0) - at(0) * b.at(2),
            at(0) * b.at(1) - at(1) * b.at(0)
        );
	}

	// Math
    template<typename IdxsOther>
	friend inline SVector& operator+=(SVector& a, SVector<Traits, IdxsOther> const& b)
	{
        const size_t RankOther = IdxsOther::Rank;

		for (size_t i = 0; i < (Rank <= RankOther ? Rank : RankOther); i++)
			a.at(i) += b.at(i);

        return a;
	}

    template<typename IdxsOther>
	friend inline SVector& operator+(SVector& a, SVector<Traits, IdxsOther> const& b)
    {
        SVector rval(a);
        return a += b;
    }

	friend inline SVector& operator+=(SVector& a, ScalarType const& b)
	{
		for (size_t i = 0; i < Rank; i++)
			a.at(i) += b;

		return a;
	}

    template<typename IdxsOther>
	friend inline SVector& operator-=(SVector& a, SVector<Traits, IdxsOther> const& b)
	{
        const size_t RankOther = IdxsOther::Rank;
		for (size_t i = 0; i < (Rank <= RankOther ? Rank : RankOther); i++)
			a.at(i) -= b.at(i);

		return a;
	}

	friend inline SVector& operator-=(SVector& a, ScalarType const& b)
	{
		for (size_t i = 0; i < Rank; i++)
			a.at(i) -= b;

		return a;
	}

    template<typename IdxsOther>
	friend inline SVector& operator*=(SVector& a, SVector<Traits, IdxsOther> const& b)
	{
        const size_t RankOther = IdxsOther::Rank;
		for (size_t i = 0; i < (Rank <= RankOther ? Rank : RankOther); i++)
			a.at(i) *= b.at(i);

		return a;
	}

	friend inline SVector& operator*=(SVector& a, ScalarType const& b)
	{
		for (size_t i = 0; i < Rank; i++)
			a.at(i) *= b;

		return a;
	}

    template<typename IdxsOther>
	friend inline SVector& operator/=(SVector& a, SVector<Traits, IdxsOther> const& b)
	{
        const size_t RankOther = IdxsOther::Rank;
		for (size_t i = 0; i < (Rank <= RankOther ? Rank : RankOther); i++)
			a.at(i) /= b.at(i);

		return a;
	}

	friend inline SVector& operator/=(SVector& a, ScalarType const& b)
	{
		for (size_t i = 0; i < Rank; i++)
			a.at(i) /= b;

		return a;
	}

	inline ScalarType const& dot(SVector const& b)
	{
		ScalarType rval = Traits::Zero;
		for (size_t i = 0; i < Rank; i++)
		{
			rval += at(i) * b.at(i);
		}
		return rval;
	}

	inline ScalarType const& squaredLength() { return this->dot(*this); }

	inline ScalarType const& length() { return Traits::Sqrt(this->squaredLength()); }

	inline SVector& slowNormalize()
	{
		ScalarType recip = Traits::One / this->length();
		this *= recip;
		return *this;
	}

    inline SVector& normalize()
	{
		ScalarType recip = Traits::FastInvSqrt(this->squaredLength);
		this *= recip;
		return *this;
	}

    inline SVector const& normal() const
	{
        SVector rval(*this);
		return rval.normalize();
	}
};

template<typename Traits, typename IdxsA, typename IdxsB>
auto operator+ (SVector<Traits, IdxsA> const& a, SVector<Traits, IdxsB> const& b)
{
    const size_t RankA = IdxsA::Rank;
    const size_t RankB = IdxsB::Rank;
    if constexpr(RankA >= RankB)
	{
        SVector<Traits, typename MakeIdxList<RankA>::Type> rval(a);
        rval += b;
        return rval;
	}
    else
    {
        SVector<Traits, typename MakeIdxList<RankB>::Type> rval(a);
        rval += b;
        return rval;
    }
}

template<typename Traits, typename Idxs>
auto operator+ (SVector<Traits, Idxs> const& a, typename Traits::Type const& b)
{
    const size_t RankA = Idxs::Rank;
    SVector<Traits, typename MakeIdxList<RankA>::Type> rval(a);

    return rval += b;
}

template<typename Traits, typename Idxs>
auto operator+ (typename Traits::Type const& b, SVector<Traits, Idxs> const& a)
{
    const size_t RankA = Idxs::Rank;
    SVector<Traits, typename MakeIdxList<RankA>::Type> rval(a);

    return rval += b;
}

template<typename Traits, typename IdxsA, typename IdxsB>
auto operator- (SVector<Traits, IdxsA> const& a, SVector<Traits, IdxsB> const& b)
{
    const size_t RankA = IdxsA::Rank;
    const size_t RankB = IdxsB::Rank;
    if constexpr(RankA >= RankB)
	{
		SVector<Traits, typename MakeIdxList<RankA>::Type> rval(a);
		rval -= b;
		return rval;
	}
	else
    {
        SVector<Traits, typename MakeIdxList<RankB>::Type> rval(a);
        rval -= b;
        return rval;
    }
}

template<typename Traits, typename Idxs>
auto operator- (SVector<Traits, Idxs> const& a, typename Traits::Type const& b)
{
    const size_t RankA = Idxs::Rank;
    SVector<Traits, typename MakeIdxList<RankA>::Type> rval(a);

    return rval -= b;
}

template<typename Traits, typename Idxs>
auto operator- (typename Traits::Type const& b, SVector<Traits, Idxs> const& a)
{
    const size_t RankA = Idxs::Rank;
    SVector<Traits, typename MakeIdxList<RankA>::Type> rval(a);

    return rval -= b;
}

template<typename Traits, typename IdxsA, typename IdxsB>
auto operator* (SVector<Traits, IdxsA> const& a, SVector<Traits, IdxsB> const& b)
{
    const size_t RankA = IdxsA::Rank;
    const size_t RankB = IdxsB::Rank;
    if constexpr(RankA >= RankB)
	{
		SVector<Traits, typename MakeIdxList<RankA>::Type> rval(a);
		rval *= b;
		return rval;
	}
	else
    {
        SVector<Traits, typename MakeIdxList<RankB>::Type> rval(a);
        rval *= b;
        return rval;
    }
}

template<typename Traits, typename Idxs>
auto operator* (SVector<Traits, Idxs> const& a, typename Traits::Type const& b)
{
    const size_t RankA = Idxs::Rank;
    SVector<Traits, typename MakeIdxList<RankA>::Type> rval(a);

    return rval *= b;
}

template<typename Traits, typename Idxs>
auto operator* (typename Traits::Type const& b, SVector<Traits, Idxs> const& a)
{
    const size_t RankA = Idxs::Rank;
    SVector<Traits, typename MakeIdxList<RankA>::Type> rval(a);

    return rval *= b;
}

template<typename Traits, typename IdxsA, typename IdxsB>
auto operator/ (SVector<Traits, IdxsA> const& a, SVector<Traits, IdxsB> const& b)
{
    const size_t RankA = IdxsA::Rank;
    const size_t RankB = IdxsB::Rank;
    if constexpr(RankA >= RankB)
	{
		SVector<Traits, typename MakeIdxList<RankA>::Type> rval(a);
		rval /= b;
		return rval;
	}
	else
    {
        SVector<Traits, typename MakeIdxList<RankB>::Type> rval(a);
        rval /= b;
        return rval;
    }
}

template<typename Traits, typename Idxs>
auto operator/ (SVector<Traits, Idxs> const& a, typename Traits::Type const& b)
{
    const size_t RankA = Idxs::Rank;
    SVector<Traits, typename MakeIdxList<RankA>::Type> rval(a);

    return rval /= b;
}

template<typename Traits, typename Idxs>
auto operator/ (typename Traits::Type const& b, SVector<Traits, Idxs> const& a)
{
    const size_t RankA = Idxs::Rank;
    SVector<Traits, typename MakeIdxList<RankA>::Type> rval(a);

    return rval /= b;
}

template<typename Traits, typename Idxs>
auto dot(SVector<Traits, Idxs> const& a, SVector<Traits, Idxs> const& b)
{
    return a.dot(b);
}

template<typename Traits, typename Idxs, typename = std::enable_if<Idxs::Rank == 3, void>>
auto cross(SVector<Traits, Idxs> const& a, SVector<Traits, Idxs> const& b)
{
    SVector<Traits, MakeIdxList<3>::Type> rval(a);
    return a.cross(b);
}

template <size_t Idx, typename Traits, typename Idxs>
auto get(SVector<Traits, Idxs> const& v)
{
    return v.data[Idxs::Indices[Idx]];
}

#define __QI_VECTOR_INTERNAL_H
#endif // #ifndef __QI_VECTOR_INTERNAL_H
