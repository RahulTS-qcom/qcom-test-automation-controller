#ifndef TACPIC32CXDRIVETHREAD_H
#define TACPIC32CXDRIVETHREAD_H
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

/*
	Author: Biswajit Roy (biswroy@qti.qualcomm.com)
*/

#include "QCommonConsoleGlobal.h"

#include "TACDriveThread.h"
#include "TACPIC32CXProtocol.h"
#include "SerialPort.h"
#include "SerialPortInfo.h"

#include <cstdint>
#include <string>

class QCOMMONCONSOLE_EXPORT TACPIC32CXDriveThread : public TACDriveThread
{
public:
    TACPIC32CXDriveThread(uint32_t hash);
    ~TACPIC32CXDriveThread();

    std::string locked() { return {}; }

    void run();

    void sendCommand(const std::string& command, bool console = false,
        ReceiveInterface* receiveInterface = nullptr, bool shouldStore = true);

    void setPinState(uint16_t pin, bool state);
    void sendCommandSequence(CommandEntries& commandEntries);

    int  getResetCount();
    void clearResetCount();

    void setName(const std::string& newName);

    uint32_t send(const std::string& sendMe, const Arguments& arguments, bool console, ReceiveInterface* recieveInterface, bool store = true);
    bool ready();

    void receive(FramePackage& framePackage);

    void i2CReadRegister(uint32_t addr, uint32_t reg);
    void i2CWriteRegister(uint32_t addr, uint32_t reg, uint32_t data);

    void endTransaction(ReceiveInterface* receiveInterface);

protected:
    bool openSerialDevice();
    void on_readyRead();

private:
    std::string serialPortError();
    bool readSerialData();

    static bool          _initialized;
    bool                 _connected{false};
    TACPIC32CXProtocol   _tacProtocol;
    SerialPortInfo       _tacPortInfo;
    SerialPort*          _serialPort{nullptr};
    bool                 _readyRead{false};
    std::string          _serialBuffer;

    void handleSetPin(FramePackage& framePackage);
    void handleVersionResponse(FramePackage& framePackage);
    void handleClearBuffer(FramePackage& framePackage);

    void setupConnected();
    void setupDiscovery();

    void log(FramePackage& framePackage);
};

#endif // TACPIC32CXDRIVETHREAD_H

