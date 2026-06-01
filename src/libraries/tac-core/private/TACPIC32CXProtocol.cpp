// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#include "TACPIC32CXProtocol.h"
#include "TACPIC32CXCoder.h"
#include "TACDriveThread.h"
#include "TACCommands.h"
#include "TACCommandHashes.h"
#include "TickCount.h"
#include <chrono>
#include <string>
#include <thread>
#include <vector>

TACPIC32CXProtocol::TACPIC32CXProtocol() : _tacDriveTrain(nullptr), _tickCount(0)
{
    setFrameCoder(new TACPIC32CXCoder);
}
TACPIC32CXProtocol::~TACPIC32CXProtocol()
{
    // _frameCoder is deleted by ProtocolInterface destructor
}

void TACPIC32CXProtocol::setTACDriveTrain(TACDriveThread* t) { _tacDriveTrain = t; }

uint32_t TACPIC32CXProtocol::sendCommand(const std::string& command, const Arguments& arguments,
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

void TACPIC32CXProtocol::endTransaction(ReceiveInterface* receiveInterface)
{
    queueEndTransaction(receiveInterface);
}

void TACPIC32CXProtocol::sendHelpCommand() {}

void TACPIC32CXProtocol::receive(FramePackage& /*framePackage*/) {}

void TACPIC32CXProtocol::idle()
{
    // TODO Week 3: implement timing logic
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void TACPIC32CXProtocol::frameComplete(const std::string& completedFrame)
{
    // PIC32CX protocol: responses arrive as multiple frames. An empty frame
    // signals end-of-response. All PIC32CX frames are considered valid.

    if (!completedFrame.empty())
    {
        _responseFrames.push_back(completedFrame);
    }
    else
    {
        // Empty frame = end of response
        FramePackage& framePackage = pendingFramePackage();
        if (framePackage != nullptr)
        {
            framePackage->_responses = _responseFrames;
            framePackage->_valid = true;  // PIC32CX: all frames considered valid

            if (framePackage->_recieveInterface != nullptr)
                framePackage->_recieveInterface->receive(framePackage);
            else if (_tacDriveTrain)
                _tacDriveTrain->receive(framePackage);
        }

        clearPendingFrame();
        _responseFrames.clear();
    }
}

void TACPIC32CXProtocol::badFrame(const std::string& /*completedFrame*/)
{
    clearPendingFrame();
}

void TACPIC32CXProtocol::triggerElapsed() {}
