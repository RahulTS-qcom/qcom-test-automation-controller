// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#include "TACLiteCoder.h"
#include "TACCommandHashes.h"
#include "TACCommands.h"
#include <algorithm>
#include <sstream>
#include <variant>

TACLiteCoder::TACLiteCoder() = default;
TACLiteCoder::~TACLiteCoder() = default;

void TACLiteCoder::reset() { FrameCoder::reset(); _recieveBuffer.clear(); }

void TACLiteCoder::decode(const std::string& decodeMe)
{
    _recieveBuffer += decodeMe;
    // TODO Week 3: implement TACLiteCoder frame parsing with real hardware
    if (_frameFunction && !_recieveBuffer.empty())
    {
        _frameFunction(_recieveBuffer, _protocolInterface);
        _recieveBuffer.clear();
    }
}

std::string TACLiteCoder::encode(const std::string& encodeMe, const Arguments& arguments)
{
    // For SetPin commands, the coded request must be the pin number as a string.
    // TACLiteDriveThread::run() does std::stoi(_codedRequest) to get the pin.
    // Arguments for SetPin: [0]=state (bool), [1]=pin (uint32_t)
    uint32_t hash = CommandStringToHash(encodeMe);
    if (hash == kSetPinCommandHash && arguments.size() >= 2)
    {
        if (std::holds_alternative<uint32_t>(arguments[1]))
            return std::to_string(std::get<uint32_t>(arguments[1]));
    }

    return encodeMe;
}
