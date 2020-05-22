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

#if defined(QI_DEV_BUILD) && QI_DEV_BUILD == 1
#define DEV_BUILD HAS_X
#else
#define DEV_BUILD HAS__
#endif

#if defined(QI_PROFILE_BUILD) && QI_PROFILE_BUILD == 1
#define PROF_BUILD HAS_X
#else
#define PROF_BUILD HAS__
#endif

#if defined(QI_OSX_BUILD) && QI_OSX_BUILD == 1
#define OSX_BUILD HAS_X
#define WIN32_BUILD HAS__
#else // Assume WIN32
#define OSX_BUILD HAS__
#define WIN32_BUILD HAS_X
#endif

#if defined(QI_RELEASE_BUILD) && QI_RELEASE_BUILD == 1
#define RELEASE_BUILD HAS_X
#else
#define RELEASE_BUILD HAS__
#endif

#define OPTIMIZED_BUILD WHEN(HAS(PROF_BUILD) || HAS(RELEASE_BUILD))
#define DEBUG_BUILD WHEN(HAS(DEV_BUILD) && !HAS(OPTIMIZED_BUILD))

#if HAS(RELEASE_BUILD) && !defined(__clang__)
#pragma message "RELEASE_BUILD enabled"
#endif

#if HAS(DEV_BUILD) && !defined(__clang__)
#pragma message("DEV_BUILD enabled")
#endif

// TODO(plat): Find a better place for this
#if HAS(OSX_BUILD)
#define PLAT_EXPORT extern "C" __attribute__((visibility("default")))
#define PLAT_IMPORT
#elif HAS(WIN32_BUILD)
#define PLAT_EXPORT __declspec(dllexport)
#define PLAT_IMPORT __declspec(dllimport)
#else
#error Undefined platform! Need define for PLAT_EXPORT / PLAT_IMPORT
#endif

#define __HAS_H
#endif // #ifndef __HAS_H

