// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#include "TACLiteProtocol.h"
#include "TACDriveThread.h"
#include "TACCommands.h"
#include "TACCommandHashes.h"
#include "TACLiteCoder.h"
#include "TickCount.h"
#include "DebugLog.h"

#include <chrono>
#include <thread>

#define PROTO_DBG(msg) \
    do { if (TACDebugLog::instance().enabled()) TACDebugLog::instance().write("Protocol", msg); } while(0)

TACLiteProtocol::TACLiteProtocol() : _tacDriveTrain(nullptr), _tickCount(0)
{
    setFrameCoder(new TACLiteCoder);
}
TACLiteProtocol::~TACLiteProtocol() = default;

void TACLiteProtocol::setTACDriveTrain(TACDriveThread* t) { _tacDriveTrain = t; }

uint32_t TACLiteProtocol::sendCommand(const std::string& command, const Arguments& arguments,
    bool console, ReceiveInterface* receiveInterface, bool shouldStore)
{
    uint32_t result = kBadQueueValue;

    if (!command.empty())
    {
        result = getNextSendID();

        auto framePackage = std::make_shared<_FramePackage>();

        framePackage->_packetID        = result;
        framePackage->_request         = command;
        framePackage->_arguments       = arguments;
        framePackage->_requestHash     = CommandStringToHash(command);
        framePackage->_console         = console;
        framePackage->_shouldStore     = shouldStore;
        framePackage->_tickcount       = tickCount();
        framePackage->_recieveInterface = receiveInterface;

        if (_frameCoder == nullptr)
        {
            framePackage->_codedRequest = command;
            PROTO_DBG("sendCommand: frameCoder=NULL, cmd='" + command + "' codedRequest='" + command + "'");
        }
        else
        {
            framePackage->_codedRequest = _frameCoder->encode(command, arguments);
            PROTO_DBG("sendCommand: cmd='" + command + "' hash=" + std::to_string(framePackage->_requestHash) +
                " args=" + std::to_string(arguments.size()) + " codedRequest='" + framePackage->_codedRequest + "'");
        }

        ProtocolInterface::queueCommand(framePackage);
        PROTO_DBG("sendCommand: queued, queueSize=" + std::to_string(queueSize()));
    }
    else
    {
        PROTO_DBG("sendCommand: EMPTY command — skipped");
    }

    return result;
}

void TACLiteProtocol::endTransaction(ReceiveInterface* receiveInterface)
{
    queueEndTransaction(receiveInterface);
}

void TACLiteProtocol::sendHelpCommand() {}

void TACLiteProtocol::receive(FramePackage& /*framePackage*/) {}

void TACLiteProtocol::idle()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void TACLiteProtocol::frameComplete(const std::string& completedFrame)
{
    if (_tacDriveTrain)
    {
        auto fp = std::make_shared<_FramePackage>();
        fp->_request = completedFrame;
        fp->_valid = true;
        _tacDriveTrain->receive(fp);
    }
}

void TACLiteProtocol::badFrame(const std::string& /*completedFrame*/) {}

void TACLiteProtocol::triggerElapsed() {}
