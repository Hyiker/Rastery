#pragma once

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#define RASTERY_WINDOWS 1
#elif defined(__APPLE__) || defined(__MACH__)
#define RASTERY_MACOSX 1
#else
#define RASTERY_LINUX 1
#endif

#ifdef __clang__
#define RASTERY_CLANG
#elif defined(__GNUC__) || defined(__GNUG__)
#define RASTERY_GCC
#elif defined(_MSC_VER)
#define RASTERY_MSVC
#endif

// dynamic library export/import

#if defined(RASTERY_WINDOWS)
#define RASTERY_API_EXPORT __declspec(dllexport)
#define RASTERY_API_IMPORT __declspec(dllimport)
#elif defined(RASTERY_LINUX) | defined(RASTERY_MACOSX)
#define RASTERY_API_EXPORT __attribute__((visibility("default")))
#define RASTERY_API_IMPORT
#else
#define RASTERY_API
#endif

#ifdef RASTERY_MODULE
#define RASTERY_API RASTERY_API_EXPORT
#else
#define RASTERY_API RASTERY_API_IMPORT
#endif

#if defined(RASTERY_MSVC) // MSVC
#define FORCE_INLINE __forceinline
#elif defined(RASTERY_GCC) || defined(RASTERY_CLANG)
#define FORCE_INLINE inline __attribute__((always_inline))
#else
#define FORCE_INLINE inline
#endif