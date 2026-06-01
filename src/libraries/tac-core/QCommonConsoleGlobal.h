#ifndef QCOMMONGLOBAL_H
#define QCOMMONGLOBAL_H
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

// Compatibility shim — forwards to TACCoreExport.h.
// Existing code uses QCOMMONCONSOLE_EXPORT; this maps it to TACCORE_EXPORT.
// New code should #include "TACCoreExport.h" and use TACCORE_EXPORT directly.

#include "TACCoreExport.h"

#define QCOMMONCONSOLE_EXPORT TACCORE_EXPORT

#endif // QCOMMONGLOBAL_H
