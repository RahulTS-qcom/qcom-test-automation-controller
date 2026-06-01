#ifndef TACCOREEXPORT_H
#define TACCOREEXPORT_H
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

// DLL export/import macros for tac-core shared library.
//
// When building tac-core as a shared library (DLL/SO):
//   - Define TACCORE_LIBRARY during compilation of tac-core itself (exports symbols)
//   - Consumers that link against tac-core do NOT define it (imports symbols)
//
// When building tac-core as a static library:
//   - Define TACCORE_STATIC to make TACCORE_EXPORT empty

#if defined(TACCORE_STATIC)
    #define TACCORE_EXPORT
#elif defined(TACCORE_LIBRARY)
    #ifdef _WIN32
        #define TACCORE_EXPORT __declspec(dllexport)
    #else
        #define TACCORE_EXPORT __attribute__((visibility("default")))
    #endif
#else
    #ifdef _WIN32
        #define TACCORE_EXPORT __declspec(dllimport)
    #else
        #define TACCORE_EXPORT __attribute__((visibility("default")))
    #endif
#endif

#endif // TACCOREEXPORT_H
