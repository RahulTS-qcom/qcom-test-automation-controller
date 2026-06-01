#ifndef TACDRIVETHREAD_H
#define TACDRIVETHREAD_H
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

/*
	Author: Michael Simpson (msimpson@qti.qualcomm.com)
			Biswajit Roy (biswroy@qti.qualcomm.com)
*/

#include "QCommonConsoleGlobal.h"

#include "DebugBoardType.h"
#include "Notification.h"
#include "AlpacaScript.h"
#include "DriveThread.h"
#include "PlatformID.h"
#include "ReceiveInterface.h"
#include "StringUtilities.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

class QCOMMONCONSOLE_EXPORT TACDriveThread :
    public DriveThread,
    public ReceiveInterface
{
public:
    TACDriveThread(HashType hash);
    ~TACDriveThread();

    bool resetLogging();

    static std::unique_ptr<TACDriveThread> openPort(const std::string& portName);

    HashType hash();

    void waitForCompletion();
    void setWaitForCompletion();
    void clearWaitForCompletion();
    bool waitForCompletionStatus();

    std::string decodeCommand(const std::string& command, Arguments& arg);
    bool checkLocalStore(FramePackage& framePackage);

    virtual void sendCommand(const std::string& command, bool console = false,
        ReceiveInterface* receiveInterface = nullptr, bool shouldStore = true) = 0;

    virtual void setPinState(uint16_t pin, bool state) = 0;
    virtual void sendCommandSequence(CommandEntries& commandEntries) = 0;

    virtual int getResetCount() = 0;
    virtual void clearResetCount() = 0;

    virtual void i2CReadRegister(uint32_t addr, uint32_t reg) = 0;
    virtual void i2CWriteRegister(uint32_t addr, uint32_t reg, uint32_t data) = 0;

    // Properties
    DebugBoardType debugBoardType();
    std::string    debugBoardTypeString();
    PlatformID     platformID();
    virtual std::string hardwareVersionString();

    bool        oldFirmware();
    std::string firmwareVersion();
    uint32_t    majorVersion();
    uint32_t    chipVersion();
    uint32_t    minorVersion();
    uint32_t    revisionVersion();

    std::string name() const;
    void setName(const std::string& newName);

    std::string portName() const;
    void setPortName(const std::string& portName);

    std::string description() const;
    void setDescription(const std::string& description);

    std::string serialNumber() const;
    void setSerialNumber(const std::string& serialNumber);

    std::string uuid();
    std::string macAddress();

    // SendInterface
    virtual uint32_t send(const std::string& sendMe, const Arguments& arguments, bool console, ReceiveInterface* recieveInterface, bool store = true) = 0;
    virtual bool ready() = 0;

    // ReceiveInterface
    virtual void receive(FramePackage& framePackage) = 0;

    void setThreadDelay(uint32_t delay);

    // ── Callbacks replacing Qt signals ────────────────────────────────────────
    // Set these before calling start(). Called from the drive thread.
    std::function<void(uint64_t pin, bool state)>          onPinStateChanged;
    std::function<void()>                                  onTransactionEnded;
    std::function<void(uint8_t value, NotificationLevel)>  onProgress;
    std::function<void()>                                  onDeviceOpen;
    std::function<void(const std::string&)>                onErrorOnOpen;
    std::function<void(const std::string&)>                onDeviceStatusChange;
    std::function<void(const std::string&)>                onHardwareTypeUpdate;
    std::function<void(const std::string&)>                onHardwareVersionUpdate;
    std::function<void(const std::string&)>                onFirmwareVersionUpdate;
    std::function<void(const std::string&)>                onNameUpdate;
    std::function<void(const std::string&)>                onUuidUpdate;
    std::function<void(const std::string&)>                onSerialNumUpdate;
    std::function<void(int)>                               onPlatformIDUpdate;
    std::function<void()>                                  onDeviceConnected;
    std::function<void()>                                  onDeviceDisconnected;
    std::function<void()>                                  onResetCountCleared;
    std::function<void(uint32_t)>                          onResetCountUpdate;
    std::function<void(const std::string&, bool)>          onI2CReadResult;
    std::function<void(const std::string&)>                onI2CWriteResult;

protected:
    // Protects shared state (_name, _uuid, _versionString, _platformID, etc.)
    // that is written by the worker thread and read by the main thread.
    // Recursive because handlers may call accessors (e.g. debugBoardTypeString()).
    mutable std::recursive_mutex _stateMutex;

    // waitForCompletion uses condition_variable instead of QCoreApplication::processEvents()
    std::atomic<bool>            _waitForCompletion{false};
    std::mutex                   _completionMutex;
    std::condition_variable      _completionCV;

    HashType        _hash{0};
    bool            _oldFirmware{false};
    std::string     _portName;
    DebugBoardType  _hardwareType{eUnknownDebugBoard};
    PlatformID      _platformID{MICRO_EPM_BOARD_ID_UNKNOWN};
    std::string     _versionString;
    std::string     _firmwareString;
    uint32_t        _firmwareMajor{0};
    uint32_t        _firmwareChip{0};
    uint32_t        _firmwareMinor{0};
    uint32_t        _firmwareRevision{0};
    std::string     _name;
    std::string     _macAddress;
    std::string     _description;
    std::string     _uuid;
    std::string     _serialNumber;
    std::string     _mcnNumber;
    std::string     _helpText;

    int             _resetCount{0};
    std::atomic<uint32_t> _delay{0};

    virtual void setupConnected() = 0;
    virtual void setupDiscovery() = 0;

    void setupLogging(bool loggingState);
    void shutdownLogging();

    void log(FramePackage& framePackage);
    void timeStampLogMessage(std::string& timeStampMe);
};

#endif // TACDRIVETHREAD_H
