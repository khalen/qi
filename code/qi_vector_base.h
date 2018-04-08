#ifndef __QI_VECTOR_BASE_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Another attempt at swizzle vectors
//

#include "qi_template_utils.h"

template<size_t Rank, template<typename> class SubVectorGen, typename DataType>
struct VectorBase;

template<template<typename> class SubVectorGen, typename DataType>
struct VectorBase<1, SubVectorGen, DataType>
{
	template<size_t... Idxs>
	using SubVector = typename SubVectorGen<IdxList<Idxs...>>::Type;

	union {
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

template<template<typename> class SubVectorGen, typename DataType>
struct VectorBase<2, SubVectorGen, DataType>
{
	template<size_t... Idxs>
	using SubVector = typename SubVectorGen<IdxList<Idxs...>>::Type;

	union {
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

		SubVector<0, 0> xx, rr, ss;
		SubVector<0, 1> xy, rg, st;
		SubVector<1, 0> yx, gr, ts;
		SubVector<1, 1> yy, gg, tt;

		SubVector<0, 0, 0> xxx, rrr, sss;
		SubVector<0, 0, 1> xxy, rrg, sst;
		SubVector<0, 1, 0> xyx, rgr, sts;
		SubVector<0, 1, 1> xyy, rgg, stt;
		SubVector<1, 0, 0> yxx, grr, tss;
		SubVector<1, 0, 1> yxy, grg, tst;
		SubVector<1, 1, 0> yyx, ggr, tts;
		SubVector<1, 1, 1> yyy, ggg, ttt;

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

template<template<typename> class SubVectorGen, typename DataType>
struct VectorBase<3, SubVectorGen, DataType>
{
	template<size_t... Idxs>
	using SubVector = typename SubVectorGen<IdxList<Idxs...>>::Type;

	union {
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

		SubVector<0, 0> xx, rr, ss;
		SubVector<0, 1> xy, rg, st;
		SubVector<0, 2> xz, rb, sp;
		SubVector<1, 0> yx, gr, ts;
		SubVector<1, 1> yy, gg, tt;
		SubVector<1, 2> yz, gb, tp;
		SubVector<2, 0> zx, br, ps;
		SubVector<2, 1> zy, bg, pt;
		SubVector<2, 2> zz, bb, pp;

		SubVector<0, 0, 0> xxx, rrr, sss;
		SubVector<0, 0, 1> xxy, rrg, sst;
		SubVector<0, 0, 2> xxz, rrb, ssp;
		SubVector<0, 1, 0> xyx, rgr, sts;
		SubVector<0, 1, 1> xyy, rgg, stt;
		SubVector<0, 1, 2> xyz, rgb, stp;
		SubVector<0, 2, 0> xzx, rbr, sps;
		SubVector<0, 2, 1> xzy, rbg, spt;
		SubVector<0, 2, 2> xzz, rbb, spp;
		SubVector<1, 0, 0> yxx, grr, tss;
		SubVector<1, 0, 1> yxy, grg, tst;
		SubVector<1, 0, 2> yxz, grb, tsp;
		SubVector<1, 1, 0> yyx, ggr, tts;
		SubVector<1, 1, 1> yyy, ggg, ttt;
		SubVector<1, 1, 2> yyz, ggb, ttp;
		SubVector<1, 2, 0> yzx, gbr, tps;
		SubVector<1, 2, 1> yzy, gbg, tpt;
		SubVector<1, 2, 2> yzz, gbb, tpp;
		SubVector<2, 0, 0> zxx, brr, pss;
		SubVector<2, 0, 1> zxy, brg, pst;
		SubVector<2, 0, 2> zxz, brb, psp;
		SubVector<2, 1, 0> zyx, bgr, pts;
		SubVector<2, 1, 1> zyy, bgg, ptt;
		SubVector<2, 1, 2> zyz, bgb, ptp;
		SubVector<2, 2, 0> zzx, bbr, pps;
		SubVector<2, 2, 1> zzy, bbg, ppt;
		SubVector<2, 2, 2> zzz, bbb, ppp;

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

template<template<typename> class SubVectorGen, typename DataType>
struct VectorBase<4, SubVectorGen, DataType>
{
	template<size_t... Idxs>
	using SubVector = typename SubVectorGen<IdxList<Idxs...>>::Type;

	union {
		DataType data;
		struct
		{
			SubVector<0> x;
			SubVector<1> y;
			SubVector<2> z;
			SubVector<3> w;
		};
		struct
		{
			SubVector<0> r;
			SubVector<1> g;
			SubVector<2> b;
			SubVector<3> a;
		};
		struct
		{
			SubVector<0> s;
			SubVector<1> t;
			SubVector<2> p;
			SubVector<3> q;
		};

		SubVector<0, 0> xx, rr, ss;
		SubVector<0, 1> xy, rg, st;
		SubVector<0, 2> xz, rb, sp;
		SubVector<0, 3> xw, ra, sq;
		SubVector<1, 0> yx, gr, ts;
		SubVector<1, 1> yy, gg, tt;
		SubVector<1, 2> yz, gb, tp;
		SubVector<1, 3> yw, ga, tq;
		SubVector<2, 0> zx, br, ps;
		SubVector<2, 1> zy, bg, pt;
		SubVector<2, 2> zz, bb, pp;
		SubVector<2, 3> zw, ba, pq;
		SubVector<3, 0> wx, ar, qs;
		SubVector<3, 1> wy, ag, qt;
		SubVector<3, 2> wz, ab, qp;
		SubVector<3, 3> ww, aa, qq;

		SubVector<0, 0, 0> xxx, rrr, sss;
		SubVector<0, 0, 1> xxy, rrg, sst;
		SubVector<0, 0, 2> xxz, rrb, ssp;
		SubVector<0, 0, 3> xxw, rra, ssq;
		SubVector<0, 1, 0> xyx, rgr, sts;
		SubVector<0, 1, 1> xyy, rgg, stt;
		SubVector<0, 1, 2> xyz, rgb, stp;
		SubVector<0, 1, 3> xyw, rga, stq;
		SubVector<0, 2, 0> xzx, rbr, sps;
		SubVector<0, 2, 1> xzy, rbg, spt;
		SubVector<0, 2, 2> xzz, rbb, spp;
		SubVector<0, 2, 3> xzw, rba, spq;
		SubVector<0, 3, 0> xwx, rar, sqs;
		SubVector<0, 3, 1> xwy, rag, sqt;
		SubVector<0, 3, 2> xwz, rab, sqp;
		SubVector<0, 3, 3> xww, raa, sqq;
		SubVector<1, 0, 0> yxx, grr, tss;
		SubVector<1, 0, 1> yxy, grg, tst;
		SubVector<1, 0, 2> yxz, grb, tsp;
		SubVector<1, 0, 3> yxw, gra, tsq;
		SubVector<1, 1, 0> yyx, ggr, tts;
		SubVector<1, 1, 1> yyy, ggg, ttt;
		SubVector<1, 1, 2> yyz, ggb, ttp;
		SubVector<1, 1, 3> yyw, gga, ttq;
		SubVector<1, 2, 0> yzx, gbr, tps;
		SubVector<1, 2, 1> yzy, gbg, tpt;
		SubVector<1, 2, 2> yzz, gbb, tpp;
		SubVector<1, 2, 3> yzw, gba, tpq;
		SubVector<1, 3, 0> ywx, gar, tqs;
		SubVector<1, 3, 1> ywy, gag, tqt;
		SubVector<1, 3, 2> ywz, gab, tqp;
		SubVector<1, 3, 3> yww, gaa, tqq;
		SubVector<2, 0, 0> zxx, brr, pss;
		SubVector<2, 0, 1> zxy, brg, pst;
		SubVector<2, 0, 2> zxz, brb, psp;
		SubVector<2, 0, 3> zxw, bra, psq;
		SubVector<2, 1, 0> zyx, bgr, pts;
		SubVector<2, 1, 1> zyy, bgg, ptt;
		SubVector<2, 1, 2> zyz, bgb, ptp;
		SubVector<2, 1, 3> zyw, bga, ptq;
		SubVector<2, 2, 0> zzx, bbr, pps;
		SubVector<2, 2, 1> zzy, bbg, ppt;
		SubVector<2, 2, 2> zzz, bbb, ppp;
		SubVector<2, 2, 3> zzw, bba, ppq;
		SubVector<2, 3, 0> zwx, bar, pqs;
		SubVector<2, 3, 1> zwy, bag, pqt;
		SubVector<2, 3, 2> zwz, bab, pqp;
		SubVector<2, 3, 3> zww, baa, pqq;
		SubVector<3, 0, 0> wxx, arr, qss;
		SubVector<3, 0, 1> wxy, arg, qst;
		SubVector<3, 0, 2> wxz, arb, qsp;
		SubVector<3, 0, 3> wxw, ara, qsq;
		SubVector<3, 1, 0> wyx, agr, qts;
		SubVector<3, 1, 1> wyy, agg, qtt;
		SubVector<3, 1, 2> wyz, agb, qtp;
		SubVector<3, 1, 3> wyw, aga, qtq;
		SubVector<3, 2, 0> wzx, abr, qps;
		SubVector<3, 2, 1> wzy, abg, qpt;
		SubVector<3, 2, 2> wzz, abb, qpp;
		SubVector<3, 2, 3> wzw, aba, qpq;
		SubVector<3, 3, 0> wwx, aar, qqs;
		SubVector<3, 3, 1> wwy, aag, qqt;
		SubVector<3, 3, 2> wwz, aab, qqp;
		SubVector<3, 3, 3> www, aaa, qqq;

		SubVector<0, 0, 0, 0> xxxx, rrrr, ssss;
		SubVector<0, 0, 0, 1> xxxy, rrrg, ssst;
		SubVector<0, 0, 0, 2> xxxz, rrrb, sssp;
		SubVector<0, 0, 0, 3> xxxw, rrra, sssq;
		SubVector<0, 0, 1, 0> xxyx, rrgr, ssts;
		SubVector<0, 0, 1, 1> xxyy, rrgg, sstt;
		SubVector<0, 0, 1, 2> xxyz, rrgb, sstp;
		SubVector<0, 0, 1, 3> xxyw, rrga, sstq;
		SubVector<0, 0, 2, 0> xxzx, rrbr, ssps;
		SubVector<0, 0, 2, 1> xxzy, rrbg, sspt;
		SubVector<0, 0, 2, 2> xxzz, rrbb, sspp;
		SubVector<0, 0, 2, 3> xxzw, rrba, sspq;
		SubVector<0, 0, 3, 0> xxwx, rrar, ssqs;
		SubVector<0, 0, 3, 1> xxwy, rrag, ssqt;
		SubVector<0, 0, 3, 2> xxwz, rrab, ssqp;
		SubVector<0, 0, 3, 3> xxww, rraa, ssqq;
		SubVector<0, 1, 0, 0> xyxx, rgrr, stss;
		SubVector<0, 1, 0, 1> xyxy, rgrg, stst;
		SubVector<0, 1, 0, 2> xyxz, rgrb, stsp;
		SubVector<0, 1, 0, 3> xyxw, rgra, stsq;
		SubVector<0, 1, 1, 0> xyyx, rggr, stts;
		SubVector<0, 1, 1, 1> xyyy, rggg, sttt;
		SubVector<0, 1, 1, 2> xyyz, rggb, sttp;
		SubVector<0, 1, 1, 3> xyyw, rgga, sttq;
		SubVector<0, 1, 2, 0> xyzx, rgbr, stps;
		SubVector<0, 1, 2, 1> xyzy, rgbg, stpt;
		SubVector<0, 1, 2, 2> xyzz, rgbb, stpp;
		SubVector<0, 1, 2, 3> xyzw, rgba, stpq;
		SubVector<0, 1, 3, 0> xywx, rgar, stqs;
		SubVector<0, 1, 3, 1> xywy, rgag, stqt;
		SubVector<0, 1, 3, 2> xywz, rgab, stqp;
		SubVector<0, 1, 3, 3> xyww, rgaa, stqq;
		SubVector<0, 2, 0, 0> xzxx, rbrr, spss;
		SubVector<0, 2, 0, 1> xzxy, rbrg, spst;
		SubVector<0, 2, 0, 2> xzxz, rbrb, spsp;
		SubVector<0, 2, 0, 3> xzxw, rbra, spsq;
		SubVector<0, 2, 1, 0> xzyx, rbgr, spts;
		SubVector<0, 2, 1, 1> xzyy, rbgg, sptt;
		SubVector<0, 2, 1, 2> xzyz, rbgb, sptp;
		SubVector<0, 2, 1, 3> xzyw, rbga, sptq;
		SubVector<0, 2, 2, 0> xzzx, rbbr, spps;
		SubVector<0, 2, 2, 1> xzzy, rbbg, sppt;
		SubVector<0, 2, 2, 2> xzzz, rbbb, sppp;
		SubVector<0, 2, 2, 3> xzzw, rbba, sppq;
		SubVector<0, 2, 3, 0> xzwx, rbar, spqs;
		SubVector<0, 2, 3, 1> xzwy, rbag, spqt;
		SubVector<0, 2, 3, 2> xzwz, rbab, spqp;
		SubVector<0, 2, 3, 3> xzww, rbaa, spqq;
		SubVector<0, 3, 0, 0> xwxx, rarr, sqss;
		SubVector<0, 3, 0, 1> xwxy, rarg, sqst;
		SubVector<0, 3, 0, 2> xwxz, rarb, sqsp;
		SubVector<0, 3, 0, 3> xwxw, rara, sqsq;
		SubVector<0, 3, 1, 0> xwyx, ragr, sqts;
		SubVector<0, 3, 1, 1> xwyy, ragg, sqtt;
		SubVector<0, 3, 1, 2> xwyz, ragb, sqtp;
		SubVector<0, 3, 1, 3> xwyw, raga, sqtq;
		SubVector<0, 3, 2, 0> xwzx, rabr, sqps;
		SubVector<0, 3, 2, 1> xwzy, rabg, sqpt;
		SubVector<0, 3, 2, 2> xwzz, rabb, sqpp;
		SubVector<0, 3, 2, 3> xwzw, raba, sqpq;
		SubVector<0, 3, 3, 0> xwwx, raar, sqqs;
		SubVector<0, 3, 3, 1> xwwy, raag, sqqt;
		SubVector<0, 3, 3, 2> xwwz, raab, sqqp;
		SubVector<0, 3, 3, 3> xwww, raaa, sqqq;
		SubVector<1, 0, 0, 0> yxxx, grrr, tsss;
		SubVector<1, 0, 0, 1> yxxy, grrg, tsst;
		SubVector<1, 0, 0, 2> yxxz, grrb, tssp;
		SubVector<1, 0, 0, 3> yxxw, grra, tssq;
		SubVector<1, 0, 1, 0> yxyx, grgr, tsts;
		SubVector<1, 0, 1, 1> yxyy, grgg, tstt;
		SubVector<1, 0, 1, 2> yxyz, grgb, tstp;
		SubVector<1, 0, 1, 3> yxyw, grga, tstq;
		SubVector<1, 0, 2, 0> yxzx, grbr, tsps;
		SubVector<1, 0, 2, 1> yxzy, grbg, tspt;
		SubVector<1, 0, 2, 2> yxzz, grbb, tspp;
		SubVector<1, 0, 2, 3> yxzw, grba, tspq;
		SubVector<1, 0, 3, 0> yxwx, grar, tsqs;
		SubVector<1, 0, 3, 1> yxwy, grag, tsqt;
		SubVector<1, 0, 3, 2> yxwz, grab, tsqp;
		SubVector<1, 0, 3, 3> yxww, graa, tsqq;
		SubVector<1, 1, 0, 0> yyxx, ggrr, ttss;
		SubVector<1, 1, 0, 1> yyxy, ggrg, ttst;
		SubVector<1, 1, 0, 2> yyxz, ggrb, ttsp;
		SubVector<1, 1, 0, 3> yyxw, ggra, ttsq;
		SubVector<1, 1, 1, 0> yyyx, gggr, ttts;
		SubVector<1, 1, 1, 1> yyyy, gggg, tttt;
		SubVector<1, 1, 1, 2> yyyz, gggb, tttp;
		SubVector<1, 1, 1, 3> yyyw, ggga, tttq;
		SubVector<1, 1, 2, 0> yyzx, ggbr, ttps;
		SubVector<1, 1, 2, 1> yyzy, ggbg, ttpt;
		SubVector<1, 1, 2, 2> yyzz, ggbb, ttpp;
		SubVector<1, 1, 2, 3> yyzw, ggba, ttpq;
		SubVector<1, 1, 3, 0> yywx, ggar, ttqs;
		SubVector<1, 1, 3, 1> yywy, ggag, ttqt;
		SubVector<1, 1, 3, 2> yywz, ggab, ttqp;
		SubVector<1, 1, 3, 3> yyww, ggaa, ttqq;
		SubVector<1, 2, 0, 0> yzxx, gbrr, tpss;
		SubVector<1, 2, 0, 1> yzxy, gbrg, tpst;
		SubVector<1, 2, 0, 2> yzxz, gbrb, tpsp;
		SubVector<1, 2, 0, 3> yzxw, gbra, tpsq;
		SubVector<1, 2, 1, 0> yzyx, gbgr, tpts;
		SubVector<1, 2, 1, 1> yzyy, gbgg, tptt;
		SubVector<1, 2, 1, 2> yzyz, gbgb, tptp;
		SubVector<1, 2, 1, 3> yzyw, gbga, tptq;
		SubVector<1, 2, 2, 0> yzzx, gbbr, tpps;
		SubVector<1, 2, 2, 1> yzzy, gbbg, tppt;
		SubVector<1, 2, 2, 2> yzzz, gbbb, tppp;
		SubVector<1, 2, 2, 3> yzzw, gbba, tppq;
		SubVector<1, 2, 3, 0> yzwx, gbar, tpqs;
		SubVector<1, 2, 3, 1> yzwy, gbag, tpqt;
		SubVector<1, 2, 3, 2> yzwz, gbab, tpqp;
		SubVector<1, 2, 3, 3> yzww, gbaa, tpqq;
		SubVector<1, 3, 0, 0> ywxx, garr, tqss;
		SubVector<1, 3, 0, 1> ywxy, garg, tqst;
		SubVector<1, 3, 0, 2> ywxz, garb, tqsp;
		SubVector<1, 3, 0, 3> ywxw, gara, tqsq;
		SubVector<1, 3, 1, 0> ywyx, gagr, tqts;
		SubVector<1, 3, 1, 1> ywyy, gagg, tqtt;
		SubVector<1, 3, 1, 2> ywyz, gagb, tqtp;
		SubVector<1, 3, 1, 3> ywyw, gaga, tqtq;
		SubVector<1, 3, 2, 0> ywzx, gabr, tqps;
		SubVector<1, 3, 2, 1> ywzy, gabg, tqpt;
		SubVector<1, 3, 2, 2> ywzz, gabb, tqpp;
		SubVector<1, 3, 2, 3> ywzw, gaba, tqpq;
		SubVector<1, 3, 3, 0> ywwx, gaar, tqqs;
		SubVector<1, 3, 3, 1> ywwy, gaag, tqqt;
		SubVector<1, 3, 3, 2> ywwz, gaab, tqqp;
		SubVector<1, 3, 3, 3> ywww, gaaa, tqqq;
		SubVector<2, 0, 0, 0> zxxx, brrr, psss;
		SubVector<2, 0, 0, 1> zxxy, brrg, psst;
		SubVector<2, 0, 0, 2> zxxz, brrb, pssp;
		SubVector<2, 0, 0, 3> zxxw, brra, pssq;
		SubVector<2, 0, 1, 0> zxyx, brgr, psts;
		SubVector<2, 0, 1, 1> zxyy, brgg, pstt;
		SubVector<2, 0, 1, 2> zxyz, brgb, pstp;
		SubVector<2, 0, 1, 3> zxyw, brga, pstq;
		SubVector<2, 0, 2, 0> zxzx, brbr, psps;
		SubVector<2, 0, 2, 1> zxzy, brbg, pspt;
		SubVector<2, 0, 2, 2> zxzz, brbb, pspp;
		SubVector<2, 0, 2, 3> zxzw, brba, pspq;
		SubVector<2, 0, 3, 0> zxwx, brar, psqs;
		SubVector<2, 0, 3, 1> zxwy, brag, psqt;
		SubVector<2, 0, 3, 2> zxwz, brab, psqp;
		SubVector<2, 0, 3, 3> zxww, braa, psqq;
		SubVector<2, 1, 0, 0> zyxx, bgrr, ptss;
		SubVector<2, 1, 0, 1> zyxy, bgrg, ptst;
		SubVector<2, 1, 0, 2> zyxz, bgrb, ptsp;
		SubVector<2, 1, 0, 3> zyxw, bgra, ptsq;
		SubVector<2, 1, 1, 0> zyyx, bggr, ptts;
		SubVector<2, 1, 1, 1> zyyy, bggg, pttt;
		SubVector<2, 1, 1, 2> zyyz, bggb, pttp;
		SubVector<2, 1, 1, 3> zyyw, bgga, pttq;
		SubVector<2, 1, 2, 0> zyzx, bgbr, ptps;
		SubVector<2, 1, 2, 1> zyzy, bgbg, ptpt;
		SubVector<2, 1, 2, 2> zyzz, bgbb, ptpp;
		SubVector<2, 1, 2, 3> zyzw, bgba, ptpq;
		SubVector<2, 1, 3, 0> zywx, bgar, ptqs;
		SubVector<2, 1, 3, 1> zywy, bgag, ptqt;
		SubVector<2, 1, 3, 2> zywz, bgab, ptqp;
		SubVector<2, 1, 3, 3> zyww, bgaa, ptqq;
		SubVector<2, 2, 0, 0> zzxx, bbrr, ppss;
		SubVector<2, 2, 0, 1> zzxy, bbrg, ppst;
		SubVector<2, 2, 0, 2> zzxz, bbrb, ppsp;
		SubVector<2, 2, 0, 3> zzxw, bbra, ppsq;
		SubVector<2, 2, 1, 0> zzyx, bbgr, ppts;
		SubVector<2, 2, 1, 1> zzyy, bbgg, pptt;
		SubVector<2, 2, 1, 2> zzyz, bbgb, pptp;
		SubVector<2, 2, 1, 3> zzyw, bbga, pptq;
		SubVector<2, 2, 2, 0> zzzx, bbbr, ppps;
		SubVector<2, 2, 2, 1> zzzy, bbbg, pppt;
		SubVector<2, 2, 2, 2> zzzz, bbbb, pppp;
		SubVector<2, 2, 2, 3> zzzw, bbba, pppq;
		SubVector<2, 2, 3, 0> zzwx, bbar, ppqs;
		SubVector<2, 2, 3, 1> zzwy, bbag, ppqt;
		SubVector<2, 2, 3, 2> zzwz, bbab, ppqp;
		SubVector<2, 2, 3, 3> zzww, bbaa, ppqq;
		SubVector<2, 3, 0, 0> zwxx, barr, pqss;
		SubVector<2, 3, 0, 1> zwxy, barg, pqst;
		SubVector<2, 3, 0, 2> zwxz, barb, pqsp;
		SubVector<2, 3, 0, 3> zwxw, bara, pqsq;
		SubVector<2, 3, 1, 0> zwyx, bagr, pqts;
		SubVector<2, 3, 1, 1> zwyy, bagg, pqtt;
		SubVector<2, 3, 1, 2> zwyz, bagb, pqtp;
		SubVector<2, 3, 1, 3> zwyw, baga, pqtq;
		SubVector<2, 3, 2, 0> zwzx, babr, pqps;
		SubVector<2, 3, 2, 1> zwzy, babg, pqpt;
		SubVector<2, 3, 2, 2> zwzz, babb, pqpp;
		SubVector<2, 3, 2, 3> zwzw, baba, pqpq;
		SubVector<2, 3, 3, 0> zwwx, baar, pqqs;
		SubVector<2, 3, 3, 1> zwwy, baag, pqqt;
		SubVector<2, 3, 3, 2> zwwz, baab, pqqp;
		SubVector<2, 3, 3, 3> zwww, baaa, pqqq;
		SubVector<3, 0, 0, 0> wxxx, arrr, qsss;
		SubVector<3, 0, 0, 1> wxxy, arrg, qsst;
		SubVector<3, 0, 0, 2> wxxz, arrb, qssp;
		SubVector<3, 0, 0, 3> wxxw, arra, qssq;
		SubVector<3, 0, 1, 0> wxyx, argr, qsts;
		SubVector<3, 0, 1, 1> wxyy, argg, qstt;
		SubVector<3, 0, 1, 2> wxyz, argb, qstp;
		SubVector<3, 0, 1, 3> wxyw, arga, qstq;
		SubVector<3, 0, 2, 0> wxzx, arbr, qsps;
		SubVector<3, 0, 2, 1> wxzy, arbg, qspt;
		SubVector<3, 0, 2, 2> wxzz, arbb, qspp;
		SubVector<3, 0, 2, 3> wxzw, arba, qspq;
		SubVector<3, 0, 3, 0> wxwx, arar, qsqs;
		SubVector<3, 0, 3, 1> wxwy, arag, qsqt;
		SubVector<3, 0, 3, 2> wxwz, arab, qsqp;
		SubVector<3, 0, 3, 3> wxww, araa, qsqq;
		SubVector<3, 1, 0, 0> wyxx, agrr, qtss;
		SubVector<3, 1, 0, 1> wyxy, agrg, qtst;
		SubVector<3, 1, 0, 2> wyxz, agrb, qtsp;
		SubVector<3, 1, 0, 3> wyxw, agra, qtsq;
		SubVector<3, 1, 1, 0> wyyx, aggr, qtts;
		SubVector<3, 1, 1, 1> wyyy, aggg, qttt;
		SubVector<3, 1, 1, 2> wyyz, aggb, qttp;
		SubVector<3, 1, 1, 3> wyyw, agga, qttq;
		SubVector<3, 1, 2, 0> wyzx, agbr, qtps;
		SubVector<3, 1, 2, 1> wyzy, agbg, qtpt;
		SubVector<3, 1, 2, 2> wyzz, agbb, qtpp;
		SubVector<3, 1, 2, 3> wyzw, agba, qtpq;
		SubVector<3, 1, 3, 0> wywx, agar, qtqs;
		SubVector<3, 1, 3, 1> wywy, agag, qtqt;
		SubVector<3, 1, 3, 2> wywz, agab, qtqp;
		SubVector<3, 1, 3, 3> wyww, agaa, qtqq;
		SubVector<3, 2, 0, 0> wzxx, abrr, qpss;
		SubVector<3, 2, 0, 1> wzxy, abrg, qpst;
		SubVector<3, 2, 0, 2> wzxz, abrb, qpsp;
		SubVector<3, 2, 0, 3> wzxw, abra, qpsq;
		SubVector<3, 2, 1, 0> wzyx, abgr, qpts;
		SubVector<3, 2, 1, 1> wzyy, abgg, qptt;
		SubVector<3, 2, 1, 2> wzyz, abgb, qptp;
		SubVector<3, 2, 1, 3> wzyw, abga, qptq;
		SubVector<3, 2, 2, 0> wzzx, abbr, qpps;
		SubVector<3, 2, 2, 1> wzzy, abbg, qppt;
		SubVector<3, 2, 2, 2> wzzz, abbb, qppp;
		SubVector<3, 2, 2, 3> wzzw, abba, qppq;
		SubVector<3, 2, 3, 0> wzwx, abar, qpqs;
		SubVector<3, 2, 3, 1> wzwy, abag, qpqt;
		SubVector<3, 2, 3, 2> wzwz, abab, qpqp;
		SubVector<3, 2, 3, 3> wzww, abaa, qpqq;
		SubVector<3, 3, 0, 0> wwxx, aarr, qqss;
		SubVector<3, 3, 0, 1> wwxy, aarg, qqst;
		SubVector<3, 3, 0, 2> wwxz, aarb, qqsp;
		SubVector<3, 3, 0, 3> wwxw, aara, qqsq;
		SubVector<3, 3, 1, 0> wwyx, aagr, qqts;
		SubVector<3, 3, 1, 1> wwyy, aagg, qqtt;
		SubVector<3, 3, 1, 2> wwyz, aagb, qqtp;
		SubVector<3, 3, 1, 3> wwyw, aaga, qqtq;
		SubVector<3, 3, 2, 0> wwzx, aabr, qqps;
		SubVector<3, 3, 2, 1> wwzy, aabg, qqpt;
		SubVector<3, 3, 2, 2> wwzz, aabb, qqpp;
		SubVector<3, 3, 2, 3> wwzw, aaba, qqpq;
		SubVector<3, 3, 3, 0> wwwx, aaar, qqqs;
		SubVector<3, 3, 3, 1> wwwy, aaag, qqqt;
		SubVector<3, 3, 3, 2> wwwz, aaab, qqqp;
		SubVector<3, 3, 3, 3> wwww, aaaa, qqqq;
	};
};

#define __QI_VECTOR_BASE_H
#endif // #ifndef __QI_VECTOR_BASE_H
