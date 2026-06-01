#ifndef DEBUGBOARDTYPE_H
#define DEBUGBOARDTYPE_H
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

// Author:	Michael Simpson <msimpson@qti.qualcomm.com>
//			Biswajit Roy <biswroy@qti.qualcomm.com>

#include "QCommonConsoleGlobal.h"

#include <string>

enum DebugBoardType
{
	eUnknownDebugBoard,
	eSpiderBoard,
	ePSOC,
	eFTDI,
	ePIC32CXAuto
};

std::string QCOMMONCONSOLE_EXPORT debugBoardTypeToString(DebugBoardType);
DebugBoardType QCOMMONCONSOLE_EXPORT debugBoardTypeFromString(const std::string& boardString);

#endif // DEBUGBOARDTYPE_H
