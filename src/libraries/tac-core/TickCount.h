#ifndef TICKCOUNT_H
#define TICKCOUNT_H
/*
	Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
	SPDX-License-Identifier: BSD-3-Clause
*/

// Qt-free replacement for qcommon-console/TickCount.h
// tac-core include path puts this file first, shadowing the Qt version.

#include "QCommonConsoleGlobal.h"

#include <cstdint>
#include <functional>

uint64_t QCOMMONCONSOLE_EXPORT tickCount();

bool QCOMMONCONSOLE_EXPORT sleepUntil(std::function<bool()> test, uint64_t milliseconds);

#endif // TICKCOUNT_H
