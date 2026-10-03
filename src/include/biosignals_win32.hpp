#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(BIOSIGNALS_WIN32_EXPORT)
#define BIOSIGNALS_EXPORT __declspec(dllexport)
#else
#define BIOSIGNALS_EXPORT __declspec(dllimport)
#endif
#else
#define BIOSIGNALS_EXPORT
#endif
