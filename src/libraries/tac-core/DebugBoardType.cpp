// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

// Author:	Michael Simpson <msimpson@qti.qualcomm.com>
//			Biswajit Roy <biswroy@qti.qualcomm.com>

#include "DebugBoardType.h"

#include <algorithm>
#include <cctype>

static bool iequals(const std::string& a, const std::string& b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    return true;
}

std::string debugBoardTypeToString(DebugBoardType debugBoardType)
{
    std::string result("Unknown");

    switch (debugBoardType)
    {
        case ePSOC:        result = "PSOC";        break;
        case eSpiderBoard: result = "Spider Board"; break;
        case eFTDI:        result = "FTDI";        break;
        case ePIC32CXAuto: result = "PIC32CXAuto"; break;
        default: break;
    }

    return result;
}

DebugBoardType debugBoardTypeFromString(const std::string& boardString)
{
    DebugBoardType result{eUnknownDebugBoard};

    if      (iequals(boardString, "PSOC"))         result = ePSOC;
    else if (iequals(boardString, "FTDI"))         result = eFTDI;
    else if (iequals(boardString, "PIC32CXAuto"))  result = ePIC32CXAuto;
    else if (iequals(boardString, "Spider Board")) result = eSpiderBoard;

    return result;
}
