#ifndef __QI_VECTOR_DETAILS_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Another attempt at swizzle vectors
//

#include "qi_template_utils.h"

#if 0
#include <utility>

template <size_t Rank, template <typename> class SubVectorGen, typename DataType>
struct VectorBase;

template <template <typename> class SubVectorGen, typename DataType>
struct VectorBase<1, SubVectorGen, DataType>
{
    template <size_t... Idxs>
    using SubVector = typename SubVectorGen<IdxList<Idxs...>>::Type;

    union
    {
        DataType data;
        struct
        {
            SubVector<0> x;
        };
        struct
        {
            SubVector<0> r;
        };
        struct
        {
            SubVector<0> s;
        };

		SubVector<0, 0>       xx, rr, ss;
		SubVector<0, 0, 0>    xxx, rrr, sss;
		SubVector<0, 0, 0, 0> xxxx, rrrr, ssss;
    };
};

template <template <typename> class SubVectorGen, typename DataType>
struct VectorBase<2, SubVectorGen, DataType>
{
    template <size_t... Idxs>
    using SubVector = typename SubVectorGen<IdxList<Idxs...>>::Type;

    union
    {
        DataType data;
        struct
        {
            SubVector<0> x;
            SubVector<1> y;
        };
        struct
        {
            SubVector<0> r;
            SubVector<1> g;
        };
        struct
        {
            SubVector<0> s;
            SubVector<1> t;
        };

		SubVector<0, 0>       xx, rr, ss;
		SubVector<0, 1>       xy, rg, st;
		SubVector<1, 0>       yx, gr, ts;
		SubVector<1, 1>       yy, gg, tt;

		SubVector<0, 0, 0>    xxx, rrr, sss;
		SubVector<0, 0, 1>    xxy, rrg, sst;
		SubVector<0, 1, 0>    xyx, rgr, sts;
		SubVector<0, 1, 1>    xyy, rgg, stt;
		SubVector<1, 0, 0>    yxx, grr, tss;
		SubVector<1, 0, 1>    yxy, grg, tst;
		SubVector<1, 1, 0>    yyx, ggr, tts;
		SubVector<1, 1, 1>    yyy, ggg, ttt;

		SubVector<0, 0, 0, 0> xxxx, rrrr, ssss;
		SubVector<0, 0, 0, 1> xxxy, rrrg, ssst;
		SubVector<0, 0, 1, 0> xxyx, rrgr, ssts;
		SubVector<0, 0, 1, 1> xxyy, rrgg, sstt;
		SubVector<0, 1, 0, 0> xyxx, rgrr, stss;
		SubVector<0, 1, 0, 1> xyxy, rgrg, stst;
		SubVector<0, 1, 1, 0> xyyx, rggr, stts;
		SubVector<0, 1, 1, 1> xyyy, rggg, sttt;
		SubVector<1, 0, 0, 0> yxxx, grrr, tsss;
		SubVector<1, 0, 0, 1> yxxy, grrg, tsst;
		SubVector<1, 0, 1, 0> yxyx, grgr, tsts;
		SubVector<1, 0, 1, 1> yxyy, grgg, tstt;
		SubVector<1, 1, 0, 0> yyxx, ggrr, ttss;
		SubVector<1, 1, 0, 1> yyxy, ggrg, ttst;
		SubVector<1, 1, 1, 0> yyyx, gggr, ttts;
		SubVector<1, 1, 1, 1> yyyy, gggg, tttt;
    };
};

template <template <typename> class SubVectorGen, typename DataType>
struct VectorBase<3, SubVectorGen, DataType>
{
    template <size_t... Idxs>
    using SubVector = typename SubVectorGen<IdxList<Idxs...>>::Type;

    union
    {
        DataType data;
        struct
        {
            SubVector<0> x;
            SubVector<1> y;
            SubVector<2> z;
        };
        struct
        {
            SubVector<0> r;
            SubVector<1> g;
            SubVector<2> b;
        };
        struct
        {
            SubVector<0> s;
            SubVector<1> t;
            SubVector<2> p;
        };

		SubVector<0, 0>       xx, rr, ss;
		SubVector<0, 1>       xy, rg, st;
		SubVector<0, 2>       xz, rb, sp;
		SubVector<1, 0>       yx, gr, ts;
		SubVector<1, 1>       yy, gg, tt;
		SubVector<1, 2>       yz, gb, tp;
		SubVector<2, 0>       zx, br, ps;
		SubVector<2, 1>       zy, bg, pt;
		SubVector<2, 2>       zz, bb, pp;

        SubVector<0, 0, 0>    xxx, rrr, sss;
        SubVector<0, 0, 1>    xxy, rrg, sst;
        SubVector<0, 0, 2>    xxz, rrb, ssp;
        SubVector<0, 1, 0>    xyx, rgr, sts;
        SubVector<0, 1, 1>    xyy, rgg, stt;
        SubVector<0, 1, 2>    xyz, rgb, stp;
        SubVector<0, 2, 0>    xzx, rbr, sps;
        SubVector<0, 2, 1>    xzy, rbg, spt;
        SubVector<0, 2, 2>    xzz, rbb, spp;
        SubVector<1, 0, 0>    yxx, grr, tss;
        SubVector<1, 0, 1>    yxy, grg, tst;
        SubVector<1, 0, 2>    yxz, grb, tsp;
        SubVector<1, 1, 0>    yyx, ggr, tts;
        SubVector<1, 1, 1>    yyy, ggg, ttt;
        SubVector<1, 1, 2>    yyz, ggb, ttp;
        SubVector<1, 2, 0>    yzx, gbr, tps;
        SubVector<1, 2, 1>    yzy, gbg, tpt;
        SubVector<1, 2, 2>    yzz, gbb, tpp;
        SubVector<2, 0, 0>    zxx, brr, pss;
        SubVector<2, 0, 1>    zxy, brg, pst;
        SubVector<2, 0, 2>    zxz, brb, psp;
        SubVector<2, 1, 0>    zyx, bgr, pts;
        SubVector<2, 1, 1>    zyy, bgg, ptt;
        SubVector<2, 1, 2>    zyz, bgb, ptp;
        SubVector<2, 2, 0>    zzx, bbr, pps;
        SubVector<2, 2, 1>    zzy, bbg, ppt;
        SubVector<2, 2, 2>    zzz, bbb, ppp;

        SubVector<0, 0, 0, 0> xxxx, rrrr, ssss;
        SubVector<0, 0, 0, 1> xxxy, rrrg, ssst;
        SubVector<0, 0, 0, 2> xxxz, rrrb, sssp;
        SubVector<0, 0, 1, 0> xxyx, rrgr, ssts;
        SubVector<0, 0, 1, 1> xxyy, rrgg, sstt;
        SubVector<0, 0, 1, 2> xxyz, rrgb, sstp;
        SubVector<0, 0, 2, 0> xxzx, rrbr, ssps;
        SubVector<0, 0, 2, 1> xxzy, rrbg, sspt;
        SubVector<0, 0, 2, 2> xxzz, rrbb, sspp;
        SubVector<0, 1, 0, 0> xyxx, rgrr, stss;
        SubVector<0, 1, 0, 1> xyxy, rgrg, stst;
        SubVector<0, 1, 0, 2> xyxz, rgrb, stsp;
        SubVector<0, 1, 1, 0> xyyx, rggr, stts;
        SubVector<0, 1, 1, 1> xyyy, rggg, sttt;
        SubVector<0, 1, 1, 2> xyyz, rggb, sttp;
        SubVector<0, 1, 2, 0> xyzx, rgbr, stps;
        SubVector<0, 1, 2, 1> xyzy, rgbg, stpt;
        SubVector<0, 1, 2, 2> xyzz, rgbb, stpp;
        SubVector<0, 2, 0, 0> xzxx, rbrr, spss;
        SubVector<0, 2, 0, 1> xzxy, rbrg, spst;
        SubVector<0, 2, 0, 2> xzxz, rbrb, spsp;
        SubVector<0, 2, 1, 0> xzyx, rbgr, spts;
        SubVector<0, 2, 1, 1> xzyy, rbgg, sptt;
        SubVector<0, 2, 1, 2> xzyz, rbgb, sptp;
        SubVector<0, 2, 2, 0> xzzx, rbbr, spps;
        SubVector<0, 2, 2, 1> xzzy, rbbg, sppt;
        SubVector<0, 2, 2, 2> xzzz, rbbb, sppp;
        SubVector<1, 0, 0, 0> yxxx, grrr, tsss;
        SubVector<1, 0, 0, 1> yxxy, grrg, tsst;
        SubVector<1, 0, 0, 2> yxxz, grrb, tssp;
        SubVector<1, 0, 1, 0> yxyx, grgr, tsts;
        SubVector<1, 0, 1, 1> yxyy, grgg, tstt;
        SubVector<1, 0, 1, 2> yxyz, grgb, tstp;
        SubVector<1, 0, 2, 0> yxzx, grbr, tsps;
        SubVector<1, 0, 2, 1> yxzy, grbg, tspt;
        SubVector<1, 0, 2, 2> yxzz, grbb, tspp;
        SubVector<1, 1, 0, 0> yyxx, ggrr, ttss;
        SubVector<1, 1, 0, 1> yyxy, ggrg, ttst;
        SubVector<1, 1, 0, 2> yyxz, ggrb, ttsp;
        SubVector<1, 1, 1, 0> yyyx, gggr, ttts;
        SubVector<1, 1, 1, 1> yyyy, gggg, tttt;
        SubVector<1, 1, 1, 2> yyyz, gggb, tttp;
        SubVector<1, 1, 2, 0> yyzx, ggbr, ttps;
        SubVector<1, 1, 2, 1> yyzy, ggbg, ttpt;
        SubVector<1, 1, 2, 2> yyzz, ggbb, ttpp;
        SubVector<1, 2, 0, 0> yzxx, gbrr, tpss;
        SubVector<1, 2, 0, 1> yzxy, gbrg, tpst;
        SubVector<1, 2, 0, 2> yzxz, gbrb, tpsp;
        SubVector<1, 2, 1, 0> yzyx, gbgr, tpts;
        SubVector<1, 2, 1, 1> yzyy, gbgg, tptt;
        SubVector<1, 2, 1, 2> yzyz, gbgb, tptp;
        SubVector<1, 2, 2, 0> yzzx, gbbr, tpps;
        SubVector<1, 2, 2, 1> yzzy, gbbg, tppt;
        SubVector<1, 2, 2, 2> yzzz, gbbb, tppp;
        SubVector<2, 0, 0, 0> zxxx, brrr, psss;
        SubVector<2, 0, 0, 1> zxxy, brrg, psst;
        SubVector<2, 0, 0, 2> zxxz, brrb, pssp;
        SubVector<2, 0, 1, 0> zxyx, brgr, psts;
        SubVector<2, 0, 1, 1> zxyy, brgg, pstt;
        SubVector<2, 0, 1, 2> zxyz, brgb, pstp;
        SubVector<2, 0, 2, 0> zxzx, brbr, psps;
        SubVector<2, 0, 2, 1> zxzy, brbg, pspt;
        SubVector<2, 0, 2, 2> zxzz, brbb, pspp;
        SubVector<2, 1, 0, 0> zyxx, bgrr, ptss;
        SubVector<2, 1, 0, 1> zyxy, bgrg, ptst;
        SubVector<2, 1, 0, 2> zyxz, bgrb, ptsp;
        SubVector<2, 1, 1, 0> zyyx, bggr, ptts;
        SubVector<2, 1, 1, 1> zyyy, bggg, pttt;
        SubVector<2, 1, 1, 2> zyyz, bggb, pttp;
        SubVector<2, 1, 2, 0> zyzx, bgbr, ptps;
        SubVector<2, 1, 2, 1> zyzy, bgbg, ptpt;
        SubVector<2, 1, 2, 2> zyzz, bgbb, ptpp;
        SubVector<2, 2, 0, 0> zzxx, bbrr, ppss;
        SubVector<2, 2, 0, 1> zzxy, bbrg, ppst;
        SubVector<2, 2, 0, 2> zzxz, bbrb, ppsp;
        SubVector<2, 2, 1, 0> zzyx, bbgr, ppts;
        SubVector<2, 2, 1, 1> zzyy, bbgg, pptt;
        SubVector<2, 2, 1, 2> zzyz, bbgb, pptp;
        SubVector<2, 2, 2, 0> zzzx, bbbr, ppps;
        SubVector<2, 2, 2, 1> zzzy, bbbg, pppt;
        SubVector<2, 2, 2, 2> zzzz, bbbb, pppp;
    };
};
#endif
#define __QI_VECTOR_DETAILS_H
#endif // #ifndef __QI_VECTOR_DETAILS_H
