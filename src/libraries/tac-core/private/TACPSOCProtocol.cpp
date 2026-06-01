// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#include "TACPSOCProtocol.h"
#include "TACPSOCCoder.h"
#include "TACDriveThread.h"
#include "TACCommands.h"
#include "TACCommandHashes.h"
#include "TickCount.h"
#include <chrono>
#include <string>
#include <thread>
#include <vector>

TACPSOCProtocol::TACPSOCProtocol() : _tacDriveTrain(nullptr), _tickCount(0)
{
    setFrameCoder(new TACPSOCCoder);
}
TACPSOCProtocol::~TACPSOCProtocol()
{
    // _frameCoder is deleted by ProtocolInterface destructor
}

void TACPSOCProtocol::setTACDriveTrain(TACDriveThread* t) { _tacDriveTrain = t; }

uint32_t TACPSOCProtocol::sendCommand(const std::string& command, const Arguments& arguments,
    bool console, ReceiveInterface* receiveInterface, bool shouldStore)
{
    uint32_t result = kBadQueueValue;

    if (!command.empty())
    {
        result = getNextSendID();

        auto fp = std::make_shared<_FramePackage>();
        fp->_packetID        = result;
        fp->_request         = command;
        fp->_arguments       = arguments;
        fp->_requestHash     = CommandStringToHash(command);
        fp->_console         = console;
        fp->_shouldStore     = shouldStore;
        fp->_tickcount       = tickCount();
        fp->_recieveInterface = receiveInterface;

        if (_frameCoder)
            fp->_codedRequest = _frameCoder->encode(command, arguments);
        else
            fp->_codedRequest = command;

        ProtocolInterface::queueCommand(fp);
    }

    return result;
}

void TACPSOCProtocol::endTransaction(ReceiveInterface* receiveInterface)
{
    queueEndTransaction(receiveInterface);
}

void TACPSOCProtocol::sendHelpCommand() {}

void TACPSOCProtocol::receive(FramePackage& /*framePackage*/) {}

void TACPSOCProtocol::idle()
{
    // TODO Week 3: implement timing logic
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void TACPSOCProtocol::frameComplete(const std::string& completedFrame)
{
    // PSOC protocol: responses arrive as multiple frames. An empty frame
    // signals end-of-response. Accumulate frames until the delimiter,
    // then deliver the complete response to the pending command.

    if (!completedFrame.empty())
    {
        // Skip echo lines that start with "CMD >> "
        if (completedFrame.find(kCommand) == std::string::npos)
            _responseFrames.push_back(completedFrame);
    }
    else
    {
        // Empty frame = end of response. Deliver to pending command.
        FramePackage& framePackage = pendingFramePackage();
        if (framePackage != nullptr)
        {
            framePackage->_responses = _responseFrames;

            // Check if last response line is "ok" (case insensitive)
            if (!_responseFrames.empty())
            {
                std::string last = _responseFrames.back();
                // Trim whitespace
                while (!last.empty() && (last.back() == '\r' || last.back() == '\n' || last.back() == ' '))
                    last.pop_back();
                framePackage->_valid = (last == "ok" || last == "OK" || last == "Ok");
            }
            else
            {
                framePackage->_valid = false;
            }

            if (framePackage->_recieveInterface != nullptr)
                framePackage->_recieveInterface->receive(framePackage);
            else if (_tacDriveTrain)
                _tacDriveTrain->receive(framePackage);
        }

        clearPendingFrame();
        _responseFrames.clear();
    }
}

void TACPSOCProtocol::badFrame(const std::string& /*completedFrame*/)
{
    clearPendingFrame();
}

void TACPSOCProtocol::triggerElapsed() {}
