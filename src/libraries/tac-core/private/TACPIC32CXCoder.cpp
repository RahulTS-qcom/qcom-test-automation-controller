// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#include "TACPIC32CXCoder.h"
#include "TACCommandHashes.h"
#include "TACCommands.h"
#include <sstream>
#include <variant>

TACPIC32CXCoder::TACPIC32CXCoder() = default;
TACPIC32CXCoder::~TACPIC32CXCoder() = default;

void TACPIC32CXCoder::reset() { FrameCoder::reset(); _recieveBuffer.clear(); }

void TACPIC32CXCoder::decode(const std::string& decodeMe)
{
    static const std::string kDelimiter("\r\n");

    _recieveBuffer += decodeMe;

    // Check for error response
    if (_recieveBuffer.find(kPIC32CXCommandError) == 0 ||
        _recieveBuffer.find(kPIC32CXCommandNotRecognized) != std::string::npos)
    {
        if (_frameFunction)
            _frameFunction(std::string(), _protocolInterface);  // empty = end of response
        _recieveBuffer.clear();
    }
    // Check for valid response (size > threshold means we got real data)
    else if (_recieveBuffer.size() > kValidPIC32CXResponseSize)
    {
        // Split by \r\n, deliver the second frame (response data)
        std::vector<std::string> frames;
        size_t pos = 0;
        while (pos < _recieveBuffer.size())
        {
            size_t delimPos = _recieveBuffer.find(kDelimiter, pos);
            if (delimPos == std::string::npos)
            {
                std::string remaining = _recieveBuffer.substr(pos);
                if (!remaining.empty())
                    frames.push_back(remaining);
                break;
            }
            std::string line = _recieveBuffer.substr(pos, delimPos - pos);
            if (!line.empty())
                frames.push_back(line);
            pos = delimPos + kDelimiter.size();
        }

        if (_frameFunction)
        {
            // Deliver the response data (second frame if available)
            if (frames.size() > 1)
                _frameFunction(frames[1], _protocolInterface);
            // Empty frame signals end-of-response
            _frameFunction(std::string(), _protocolInterface);
        }

        _recieveBuffer.clear();
    }
    else
    {
        // Response too short — might be incomplete, or just a prompt
        // Deliver empty frame to signal completion
        if (_frameFunction)
            _frameFunction(std::string(), _protocolInterface);
        _recieveBuffer.clear();
    }
}

std::string TACPIC32CXCoder::encode(const std::string& encodeMe, const Arguments& arguments)
{
    std::string result;

    switch (CommandStringToHash(encodeMe))
    {
    case kPIC32CXSetPinCommandHash:
        // PIC32CX SetPin: "CONF:DIG:ON <state> (@<pin>)\n"
        // Pin must be zero-padded to 3 chars for port 0 pins
        if (arguments.size() >= 2)
        {
            std::string stateStr = "0";
            std::string pinStr = "0";

            if (std::holds_alternative<bool>(arguments.at(0)))
                stateStr = std::get<bool>(arguments.at(0)) ? "1" : "0";

            if (std::holds_alternative<uint32_t>(arguments.at(1)))
                pinStr = std::to_string(std::get<uint32_t>(arguments.at(1)));

            // Prepend '0' to pin to comply with PIC32CX firmware if port 0 pins
            if (pinStr.size() < 3)
                pinStr = "0" + pinStr;

            result = std::string(kPIC32CXSetPinCommand) + " " + stateStr + " (@" + pinStr + ")";
        }
        break;

    default:
        result = encodeMe;
        break;
    }

    // PIC32CX uses \n as line terminator (not \r)
    if (result.empty())
        result = encodeMe;
    if (!result.empty() && result.back() != '\n')
        result += "\n";

    return result;
}
