#ifndef TACDEVICE_H
#define TACDEVICE_H
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

/*
	Author: Michael Simpson (msimpson@qti.qualcomm.com)
			Biswajit Roy (biswroy@qti.qualcomm.com)
*/

#include "QCommonConsoleGlobal.h"

#include "DebugBoardType.h"
#include "PlatformConfiguration.h"
#include "TACCommand.h"
#include "private/TACDriveThread.h"
#include "AlpacaScript.h"
#include "PlatformID.h"
#include "StringUtilities.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

const std::string kMACNotSupported{"Not supported"};

class _AlpacaDevice;

typedef std::shared_ptr<_AlpacaDevice>  AlpacaDevice;
typedef std::vector<AlpacaDevice>       AlpacaDevices;

// Simple 2D size — replaces QSize (used only for window dimensions)
struct TACWindowSize { int width{0}; int height{0}; };

class QCOMMONCONSOLE_EXPORT _AlpacaDevice
{
public:
    _AlpacaDevice() = default;
    virtual ~_AlpacaDevice();

    static void getAlpacaDevices(AlpacaDevices& alpacaDevices, DebugBoardType debugBoardTypeFilter = eUnknownDebugBoard);
    static uint32_t updateAlpacaDevices();

    static AlpacaDevice findAlpacaDevice(HashType hash);
    static AlpacaDevice findAlpacaDevice(const std::string& portName);
    static AlpacaDevice findAlpacaDeviceBySerialNumber(const std::string& serialNumber, bool usePartial = true);
    static AlpacaDevice findAlpacaDeviceByDescription(const std::string& description, bool usePartial = true);
    static AlpacaDevice findAlpacaDeviceByUSBDescriptor(const std::string& descriptor, bool usePartial = true);

    std::string getLastError();

    virtual bool open() = 0;
    bool isOpen();
    void close();

    virtual void buildMapping() = 0;
    void buildQuickSettings();

    uint32_t    commandCount();
    TACCommand  commandEntry(uint32_t commandIndex);
    TACCommands commandList();

    uint32_t    quickCommandCount();
    std::string getQuickCommand(uint32_t index);

    uint32_t    scriptVariableCount();
    std::string getScriptVariable(uint32_t index);
    bool        updateScriptVariableValue(const std::string& scriptVariable, const std::string& value);

    bool getCommandState(const std::string& command);
    bool sendCommand(const std::string& command, bool state);
    bool quickCommand(const std::string& command);
    bool isCommandQueueClear();

    std::string getHelp();

    virtual void setPinState(PinID pin, bool state);
    void setWaitForCompletion();

    bool     active();
    HashType hash();

    std::string portName() const;
    void setPortName(const std::string& portName);

    DebugBoardType debugBoardType();
    std::string    debugBoardTypeString();
    std::string    hardwareVersionString();
    std::string    firmwareVersion();

    uint32_t    majorVersion();
    std::string chipVersion();
    uint32_t    minorVersion();
    uint32_t    revisionVersion();

    std::string        description() const;
    void               setDescription(const std::string& description);
    std::string        usbDescriptor();
    std::string        serialNumber() const;
    void               setSerialNumber(const std::string& serialNumber);
    PlatformID         platformID();
    std::string        macAddress();
    PlatformConfiguration platformConfiguration();

    void externalPowerControl(bool state);

    std::string name() const;
    void setName(const std::string& newName);

    TACWindowSize windowDimension();

    std::string uuid();

    int  getResetCount();
    void clearResetCount();

    void i2CReadRegister(uint32_t addr, uint32_t reg);
    void i2CWriteRegister(uint32_t addr, uint32_t reg, uint32_t data);

    void setActive(bool active = true) { _active = active; }

    // ── Callbacks replacing Qt signals ────────────────────────────────────────
    std::function<void(uint64_t pin, bool state)>  onPinStateChanged;
    std::function<void(uint8_t, NotificationLevel)> onProgress;
    std::function<void(const std::string&)>         onErrorEvent;

    // Called by TACDriveThread when pin state changes
    void on_pinStateChanged(uint64_t pin, bool state);
    void onDeviceDisconnect();

protected:
    static std::mutex       _mutex;
    static AlpacaDevices    _alpacaDevices;

    bool                    _active{false};
    HashType                _hash{0};
    DebugBoardType          _boardType{eUnknownDebugBoard};
    std::string             _portName;
    std::string             _description;
    std::string             _usbDescriptor;
    std::string             _macAddress{kMACNotSupported};
    std::string             _serialNumber;
    uint32_t                _chipVersion{0};
    PlatformID              _platformID{MICRO_EPM_BOARD_ID_UNKNOWN};
    std::string             _helpText;
    std::unique_ptr<TACDriveThread> _driveThread;

    // Protects _commands and _commandList — written by drive thread (via
    // on_pinStateChanged), read by API thread (getCommandState, sendCommand, etc.)
    mutable std::mutex      _commandsMutex;

    PlatformConfiguration   _platformConfiguration;
    TACCommandMap           _commands;
    TACCommands             _commandList;
    std::vector<std::string> _quickCommandList;
    AlpacaScript            _alpacaScript;
    std::string             _lastError;
};

#endif // TACDEVICE_H
