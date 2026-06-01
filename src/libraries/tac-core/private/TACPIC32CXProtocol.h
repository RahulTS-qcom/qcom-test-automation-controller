#ifndef TACPIC32CXPROTOCOL_H
#define TACPIC32CXPROTOCOL_H
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#include "QCommonConsoleGlobal.h"

class TACDriveThread;

#include "ProtocolInterface.h"
#include "ReceiveInterface.h"

#include <cstdint>
#include <string>
#include <vector>

class QCOMMONCONSOLE_EXPORT TACPIC32CXProtocol :
    public ProtocolInterface,
    public ReceiveInterface
{
public:
    TACPIC32CXProtocol();
    virtual ~TACPIC32CXProtocol();

    void setTACDriveTrain(TACDriveThread* tacDriveTrain);

    uint32_t sendCommand(const std::string& command, const Arguments& arguments, bool console = false,
        ReceiveInterface* receiveInterface = nullptr, bool shouldStore = true);
    void endTransaction(ReceiveInterface* receiveInterface = nullptr);
    void sendHelpCommand();

    virtual void receive(FramePackage& framePackage);
    virtual void idle();

protected:
    virtual void frameComplete(const std::string& completedFrame);
    virtual void badFrame(const std::string& completedFrame);

private:
    TACDriveThread*  _tacDriveTrain{nullptr};
    uint64_t         _tickCount{0};
    std::string      _currentCommand;
    std::vector<std::string> _responseFrames;

    void triggerElapsed();
};

#endif // TACPIC32CXPROTOCOL_H
