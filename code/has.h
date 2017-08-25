#ifndef __HAS_H

//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// HAS macro system and basic defines
//
// We want HAS(FOO) to:
// a) Evaluate to 1 / true if FOO is defined as HAS_X,
// b) Evaluate to 0 / false if FOO is defined as HAS__,
// c) Error if FOO is undefined or defined as 0 or some other invalid token.
//

#define HAS__ 2
#define HAS_X 4

#define HAS(test)  (1 % ((test) / 2))
#define WHEN(test) (2 + (test) * 2)

#if defined(QI_DEV_BUILD)
#define DEV_BUILD HAS_X
#else
#define DEV_BUILD HAS__
#endif

#if defined(QI_FAST_BUILD)
#define FAST_BUILD HAS_X
#else
#define FAST_BUILD HAS__
#endif

#if defined(QI_PROFILE_BUILD)
#define PROF_BUILD HAS_X
#else
#define PROF_BUILD HAS__
#endif

#define RELEASE_BUILD WHEN(!HAS(DEV_BUILD) && HAS(FAST_BUILD))

#if HAS(RELEASE_BUILD) && !defined(__clang__)
#pragma message "RELEASE_BUILD enabled"
#else

#if HAS(DEV_BUILD) && !defined(__clang__)
#pragma message("DEV_BUILD enabled")
#endif

#if HAS(FAST_BUILD) && !defined(__clang__)
#pragma message("FAST_BUILD enabled")
#endif
#endif

// TODO(plat): Find a better place for this
#define PLAT_EXPORT __declspec(dllexport)
#define PLAT_IMPORT __declspec(dllimport)

#define __HAS_H
#endif // #ifndef __HAS_H
