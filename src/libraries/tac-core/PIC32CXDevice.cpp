// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

// Author: Biswajit Roy <biswroy@qti.qualcomm.com>

#include "PIC32CXDevice.h"

#include "private/TACPIC32CXDriveThread.h"
#include "AppCore.h"
#include "SerialPortInfo.h"

#include <algorithm>
#include <chrono>
#include <thread>

static const std::string kPIC32CXDescription{"ALPACA PIC32CX Debug Board"};

PIC32CXDevice::~PIC32CXDevice()
{
}

uint32_t PIC32CXDevice::updateAlpacaDevices()
{
    // VID/PID filter — only enumerate serial ports that match Microchip PIC32CX boards.
    // Matches original Qt behavior via PIC32CXSerialTableModel filter key.
    static constexpr uint16_t kPIC32CX_VID = 0x04D8;  // Microchip
    static constexpr uint16_t kPIC32CX_PID = 0x000A;

    SerialPortInfos ports = SerialPortInfo::availablePorts();

    for (const auto& portInfo : ports)
    {
        // Only accept ports matching Microchip PIC32CX VID/PID
        if (!portInfo.matchesVidPid(kPIC32CX_VID, kPIC32CX_PID))
            continue;

        std::string portName = portInfo.portName();
        HashType hash = ::arrayHash(portName);

        AlpacaDevice existing = _AlpacaDevice::findAlpacaDevice(hash);
        if (!existing)
        {
            PIC32CXDevice* dev = new PIC32CXDevice;
            dev->_portName    = portName;
            dev->_hash        = hash;
            dev->_active      = true;
            dev->_boardType   = ePIC32CXAuto;
            dev->_platformID  = ALPACA_PIC32CX_ID;
            dev->_description = kPIC32CXDescription;
            dev->_serialNumber= portInfo.serialNumber();

            // Use serial number prefix as USB descriptor for platform identification
            std::string sn = portInfo.serialNumber();
            size_t pos = sn.find("XX");
            if (pos != std::string::npos)
            {
                dev->_usbDescriptor = sn.substr(0, pos);
                if (_PlatformConfiguration::containsUSBDescriptor(dev->_usbDescriptor))
                    dev->_platformID = _PlatformConfiguration::getUSBDescriptor(dev->_usbDescriptor);
            }

            TACPlatformEntry platformEntry = _PlatformConfiguration::getEntry(dev->_platformID);
            if (platformEntry._platformEntry)
            {
                dev->_description = platformEntry._platformEntry->_description;
                dev->_platformConfiguration = platformEntry.getConfiguration();
            }
            else
            {
                dev->_description = "Unknown Platform (ID: " + std::to_string(static_cast<int>(dev->_platformID)) + ")";
                AppCore::writeToApplicationLog("WARNING: No platform config for PIC32CX ID " +
                    std::to_string(static_cast<int>(dev->_platformID)) + " on port " + portName + "\n");
            }

            _AlpacaDevice::_alpacaDevices.push_back(AlpacaDevice(dev));
            AppCore::writeToApplicationLog("Found a PIC32CX device with port name: '" + portName + "'\n");
        }
        else
        {
            existing->setActive();
        }
    }

    return static_cast<uint32_t>(_AlpacaDevice::_alpacaDevices.size());
}

bool PIC32CXDevice::open()
{
    bool result{false};

    if (!_driveThread)
    {
        _driveThread = std::make_unique<TACPIC32CXDriveThread>(static_cast<uint32_t>(_hash));
        _driveThread->setThreadDelay(300);

        // Wire up pin state callback (replaces Qt connect())
        _driveThread->onPinStateChanged = [this](uint64_t pin, bool state){
            this->on_pinStateChanged(pin, state);
        };

        _driveThread->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        for (int i = 0; i < 60; ++i)
        {
            result = _driveThread->isRunning();
            if (result) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    }

    if (result)
    {
        _pic32cxPlatformConfiguration = static_cast<_PIC32CXPlatformConfiguration*>(_platformConfiguration.get());
        buildMapping();
    }

    if (_driveThread)
    {
        _macAddress  = _driveThread->macAddress();
        _chipVersion = _driveThread->chipVersion();
        _driveThread->setThreadDelay(0);
    }

    return result;
}

void PIC32CXDevice::buildMapping()
{
    if (_commands.empty())
    {
        Pins pins = _platformConfiguration->getPins();
        for (const auto& pin : pins)
        {
            TACCommand cmd;
            cmd._pin       = pin._pin;
            cmd._command   = pin._pinCommand;
            cmd._helpText  = pin._pinTooltip;
            cmd._tabName   = pin._tabName;
            cmd._groupName = CommandGroup::toString(pin._commandGroup);
            cmd._cellLocation = std::to_string(pin._cellX) + "," + std::to_string(pin._cellY);
            _commands[cmd._command] = cmd;
            _commandList.push_back(cmd);
        }
    }
    buildQuickSettings();
}
