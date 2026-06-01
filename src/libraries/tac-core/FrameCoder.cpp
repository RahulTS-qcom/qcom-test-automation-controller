/*
	Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
	SPDX-License-Identifier: BSD-3-Clause
*/

// Author: msimpson

#include "FrameCoder.h"

#include <cassert>
#include <variant>

void FrameCoder::reset()
{
    _frame.clear();
}

void FrameCoder::decode(const std::string& decodeMe)
{
    assert(_frameFunction != nullptr);
    _frameFunction(decodeMe, _protocolInterface);
}

std::string FrameCoder::encode(const std::string& encodeMe, const Arguments& /*arguments*/)
{
    return encodeMe;
}

std::string FrameCoder::variantToBoolString(const std::variant<bool, uint32_t, std::string>& state) const
{
    if (std::holds_alternative<bool>(state))
        return std::get<bool>(state) ? "1" : "0";
    if (std::holds_alternative<uint32_t>(state))
        return std::get<uint32_t>(state) ? "1" : "0";
    return "0";
}

void FrameCoder::setupCallbackFunctions(
    ProtocolInterface* protocolInterface,
    FrameCompleteFunc frameFunc,
    BadFrameFunc badFrameFunc)
{
    _protocolInterface = protocolInterface;
    _frameFunction     = frameFunc;
    _badFrameFunction  = badFrameFunc;
}
