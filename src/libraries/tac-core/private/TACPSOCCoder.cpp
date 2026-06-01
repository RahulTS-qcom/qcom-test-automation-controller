// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#include "TACPSOCCoder.h"
#include "TACCommandHashes.h"
#include "TACCommands.h"
#include <algorithm>
#include <sstream>
#include <variant>

TACPSOCCoder::TACPSOCCoder() = default;
TACPSOCCoder::~TACPSOCCoder() = default;

void TACPSOCCoder::reset() { FrameCoder::reset(); _recieveBuffer.clear(); }

void TACPSOCCoder::decode(const std::string& decodeMe)
{
    static const std::string kDelimiter("\r\n");

    _recieveBuffer += decodeMe;

    std::string frame = _recieveBuffer;

    // Check if response is complete: contains "CMD >> " or "CMD: Command not recognized"
    bool hasPrompt = (frame.find(kCommand) != std::string::npos) ||
                     (frame.find(kCommandNotRecognized) != std::string::npos);

    if (hasPrompt && _frameFunction)
    {
        // Split by \r\n delimiter
        std::vector<std::string> frames;
        size_t pos = 0;
        while (pos < frame.size())
        {
            size_t delimPos = frame.find(kDelimiter, pos);
            if (delimPos == std::string::npos)
            {
                std::string remaining = frame.substr(pos);
                if (!remaining.empty())
                    frames.push_back(remaining);
                break;
            }
            std::string line = frame.substr(pos, delimPos - pos);
            if (!line.empty())
                frames.push_back(line);
            pos = delimPos + kDelimiter.size();
        }

        int count = 0;
        for (const auto& f : frames)
        {
            count++;
            _frameFunction(f, _protocolInterface);
        }

        // Pad to at least 3 frames (matches Qt behavior)
        while (count < 3)
        {
            _frameFunction(" ", _protocolInterface);
            count++;
        }

        // Empty frame signals end-of-response
        _frameFunction(std::string(), _protocolInterface);

        _recieveBuffer.clear();
    }
}

std::string TACPSOCCoder::encode(const std::string& encodeMe, const Arguments& arguments)
{
    std::string result;

    // Translate logical command names to PSOC firmware protocol commands
    HashType hash = CommandStringToHash(encodeMe);

    switch (hash)
    {
    case kVersionCommandHash:
        result = "version\r";
        break;

    case kGetNameCommandHash:
        result = "getname\r";
        break;

    case kSetNameCommandHash:
        if (!arguments.empty() && std::holds_alternative<std::string>(arguments.at(0)))
            result = "setname " + std::get<std::string>(arguments.at(0)) + "\r";
        else
            result = "setname\r";
        break;

    case kGetUUIDCommandHash:
        result = "sys getFSUUID\r";
        break;

    case kGetPlatformIDCommandHash:
        result = "getboardid\r";
        break;

    case kGetResetCountCommandHash:
        result = "getresetcount\r";
        break;

    case kClearResetCountCommandHash:
        result = "clearresetcount\r";
        break;

    case kI2CReadRegisterCommandHash:
        if (!arguments.empty() && std::holds_alternative<std::string>(arguments.at(0)))
            result = "i2c readRegisterBytes " + std::get<std::string>(arguments.at(0)) + " 1\r";
        else if (!arguments.empty() && std::holds_alternative<uint32_t>(arguments.at(0)))
            result = "i2c readRegisterBytes " + std::to_string(std::get<uint32_t>(arguments.at(0))) + " 1\r";
        else
            result = "i2c readRegisterBytes\r";
        break;

    case kI2CReadRegisterValueCommandHash:
        result = "i2c receive\r";
        break;

    case kI2CWriteRegisterCommandHash:
        if (!arguments.empty() && std::holds_alternative<std::string>(arguments.at(0)))
            result = "i2c writeByte " + std::get<std::string>(arguments.at(0)) + "\r";
        else if (!arguments.empty() && std::holds_alternative<uint32_t>(arguments.at(0)))
            result = "i2c writeByte " + std::to_string(std::get<uint32_t>(arguments.at(0))) + "\r";
        else
            result = "i2c writeByte\r";
        break;

    case kSetPinCommandHash:
        // SetPin arguments: [0]=state (bool), [1]=pin (uint32_t)
        if (arguments.size() >= 2)
        {
            std::string stateStr = "0";
            std::string pinStr = "0";

            if (std::holds_alternative<bool>(arguments.at(0)))
                stateStr = std::get<bool>(arguments.at(0)) ? "1" : "0";
            else if (std::holds_alternative<uint32_t>(arguments.at(0)))
                stateStr = std::to_string(std::get<uint32_t>(arguments.at(0)));

            if (std::holds_alternative<uint32_t>(arguments.at(1)))
                pinStr = std::to_string(std::get<uint32_t>(arguments.at(1)));

            result = "pin " + stateStr + " " + pinStr + "\r";
        }
        break;

    default:
        // Pass through unknown commands as-is
        result = encodeMe;
        if (!result.empty() && result.back() != '\r')
            result += "\r";
        break;
    }

    if (result.empty())
        result = encodeMe + "\r";

    return result;
}
