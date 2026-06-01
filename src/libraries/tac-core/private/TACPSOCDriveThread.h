#ifndef TACPSOCDRIVETHREAD_H
#define TACPSOCDRIVETHREAD_H
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

/*
	Author: Michael Simpson (msimpson@qti.qualcomm.com)
*/

#include "QCommonConsoleGlobal.h"

#include "TACDriveThread.h"
#include "TACPSOCProtocol.h"
#include "SerialPort.h"
#include "SerialPortInfo.h"

#include <cstdint>
#include <string>

class QCOMMONCONSOLE_EXPORT TACPSOCDriveThread : public TACDriveThread
{
public:
    TACPSOCDriveThread(uint32_t hash);
    ~TACPSOCDriveThread();

    virtual std::string locked() { return {}; }

    virtual void run();

    virtual void sendCommand(const std::string& command, bool console = false,
        ReceiveInterface* receiveInterface = nullptr, bool shouldStore = true);
    void endTransaction(ReceiveInterface* receiveInterface);

    virtual void setPinState(uint16_t pin, bool state);
    virtual void sendCommandSequence(CommandEntries& commandEntries);

    virtual int  getResetCount();
    virtual void clearResetCount();

    virtual void i2CReadRegister(uint32_t addr, uint32_t reg);
    virtual void i2CWriteRegister(uint32_t addr, uint32_t reg, uint32_t data);

    virtual void setName(const std::string& newName);

    virtual uint32_t send(const std::string& sendMe, const Arguments& arguments, bool console, ReceiveInterface* recieveInterface, bool store = true);
    virtual bool ready();

    virtual void receive(FramePackage& framePackage);

protected:
    bool openSerialDevice();

    // Called when data arrives on the serial port
    void on_readyRead();

private:
    std::string serialPortError();
    bool readSerialData();

    static bool      _initialized;
    bool             _connected{false};
    TACPSOCProtocol  _tacProtocol;
    SerialPortInfo   _tacPortInfo;
    SerialPort*      _serialPort{nullptr};
    bool             _readyRead{false};

    void handleGetNameResponse(FramePackage& framePackage);
    void handleGetResetCount(FramePackage& framePackage);
    void handleI2CRead(FramePackage& framePackage);
    void handleI2CWrite(FramePackage& framePackage);
    void handleIdle(FramePackage& framePackage);
    void handleSetPin(FramePackage& framePackage);
    void handlePlatformID(FramePackage& framePackage);
    void handleSetName(FramePackage& framePackage);
    void handleUUIDResponse(FramePackage& framePackage);
    void handleVersionResponse(FramePackage& framePackage);

    virtual void setupConnected();
    virtual void setupDiscovery();

    void log(FramePackage& framePackage);
};

#endif // TACPSOCDRIVETHREAD_H
