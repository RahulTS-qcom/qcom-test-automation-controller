// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

/*
	Author: Michael Simpson (msimpson@qti.qualcomm.com)
*/

#include "FTDIDevice.h"

#include "private/TACLiteDriveThread.h"

#include "AppCore.h"
#include "FTDIPlatformConfiguration.h"

#include "DebugLog.h"

#define FTDI_DBG(msg) TACDEV_DBG_TAG("FTDI", msg)

#include <algorithm>
#include <chrono>
#include <cstring>
#include <thread>

// FTDI
#include "ftd2xx.h"

class FTDIInitializer
{
public:
    FTDIInitializer()  { FT_Initialise(); }
    ~FTDIInitializer() { FT_Finalise(); }
} gFTDIInitializer;

FTDIDevice::~FTDIDevice()
{
}

bool FTDIDevice::programDevice(AlpacaDevice alpacaDevice, PlatformID platformID, std::string& errorMessage)
{
    bool result{false};

    TACPlatformEntry tacPlatformEntry = _PlatformConfiguration::getEntry(platformID);
    if (!tacPlatformEntry._platformEntry || tacPlatformEntry._platformEntry->_platformID != platformID)
    {
        errorMessage = "No platform entry for ID " + std::to_string(static_cast<int>(platformID));
        return false;
    }

    FT_STATUS ftStatus;
    FT_HANDLE ftHandle{nullptr};

    std::string serialNumber = alpacaDevice->serialNumber() + "A";
    std::string usbDescriptor = tacPlatformEntry._platformEntry->_usbDescriptor;
    if (usbDescriptor.empty())
        usbDescriptor = "ALPACA-LITE MTP DEBUG BOARD";

    ftStatus = FT_OpenEx(const_cast<char*>(serialNumber.c_str()), FT_OPEN_BY_SERIAL_NUMBER, &ftHandle);
    if (ftStatus == FT_OK)
    {
        FT_PROGRAM_DATA ftData;
        ::memset(&ftData, 0, sizeof(FT_PROGRAM_DATA));

        char manufacturerBuf[32]{};
        char manufacturerIdBuf[16]{};
        char descriptionBuf[64]{};
        char serialNumberBuf[16]{};

        ftData.Signature1    = 0x00000000;
        ftData.Signature2    = 0xffffffff;
        ftData.Version       = 0x00000008;
        ftData.Manufacturer  = manufacturerBuf;
        ftData.ManufacturerId= manufacturerIdBuf;
        ftData.Description   = descriptionBuf;
        ftData.SerialNumber  = serialNumberBuf;

        ftStatus = FT_EE_Read(ftHandle, &ftData);
        if (ftStatus == FT_OK)
        {
            std::memset(descriptionBuf, 0, sizeof(descriptionBuf));
            size_t copyLen = usbDescriptor.size() < sizeof(descriptionBuf) - 1
                           ? usbDescriptor.size() : sizeof(descriptionBuf) - 1;
            std::memcpy(descriptionBuf, usbDescriptor.c_str(), copyLen);

            auto& ps = tacPlatformEntry._platformEntry->_pinSets[0];
            ftData.AIsVCP8 = (ps & eA) ? 0 : 1;
            ftData.BIsVCP8 = (ps & eB) ? 0 : 1;
            ftData.CIsVCP8 = (ps & eC) ? 0 : 1;
            ftData.DIsVCP8 = (ps & eD) ? 0 : 1;

            ftStatus = FT_EE_Program(ftHandle, &ftData);
            if (ftStatus == FT_OK)
            {
                result = true;
                FT_CyclePort(ftHandle);
            }
            else
                errorMessage = "FT_EE_Program failed Error Code: " + _FTDIChipset::ftidStatusToString(ftStatus);
        }
        else
            errorMessage = "FT_EE_ReadEx failed Error Code: " + _FTDIChipset::ftidStatusToString(ftStatus);

        FT_Close(ftHandle);
    }
    else
        errorMessage = "FT_OpenEx failed Error Code: " + _FTDIChipset::ftidStatusToString(ftStatus);

    return result;
}

uint32_t FTDIDevice::updateAlpacaDevices()
{
    FTDI_DBG("Starting enumeration");
    AppCore::writeToApplicationLog("[FTDIDevice::updateAlpacaDevices] Starting enumeration\n");
    uint32_t deviceCount = _FTDIChipset::getDeviceCount();
    FTDI_DBG("Chipsets found: " + std::to_string(deviceCount));
    AppCore::writeToApplicationLog("[FTDIDevice::updateAlpacaDevices] FTDI chipsets found: " + std::to_string(deviceCount) + "\n");

    for (uint32_t i = 0; i < deviceCount; ++i)
    {
        FTDIChipset ftdiChipset = _FTDIChipset::getDevice(i);
        HashType hash = ftdiChipset->hash();

        AlpacaDevice existing = _AlpacaDevice::findAlpacaDevice(hash);
        if (!existing)
        {
            FTDIDevice* ftdiDevice = new FTDIDevice;
            ftdiDevice->_active       = true;
            ftdiDevice->_boardType    = eFTDI;
            ftdiDevice->_portName     = ftdiChipset->portName();
            ftdiDevice->_hash         = hash;
            ftdiDevice->_chipVersion  = 10000;
            ftdiDevice->_usbDescriptor= ftdiChipset->usbDescriptor();
            ftdiDevice->_serialNumber = ftdiChipset->serialNumber();
            ftdiDevice->_platformID   = ftdiChipset->platformID();

            AppCore::writeToApplicationLog("[FTDIDevice::updateAlpacaDevices] Device " + std::to_string(i) +
                ": port='" + ftdiDevice->_portName + "' desc='" + ftdiDevice->_usbDescriptor +
                "' platformID=" + std::to_string(static_cast<int>(ftdiDevice->_platformID)) + "\n");

            TACPlatformEntry platformEntry = _PlatformConfiguration::getEntry(ftdiDevice->_platformID);
            if (platformEntry._platformEntry)
            {
                ftdiDevice->_description = platformEntry._platformEntry->_description;
                AppCore::writeToApplicationLog("[FTDIDevice::updateAlpacaDevices] Loaded config: '" + ftdiDevice->_description + "'\n");
                ftdiDevice->_platformConfiguration = platformEntry.getConfiguration();
            }
            else
            {
                ftdiDevice->_description = "Unknown Platform (ID: " + std::to_string(static_cast<int>(ftdiDevice->_platformID)) + ")";
                AppCore::writeToApplicationLog("WARNING: No platform config for ID " +
                    std::to_string(static_cast<int>(ftdiDevice->_platformID)) + " on port " + ftdiDevice->_portName + "\n");
            }

            AlpacaDevice dev(ftdiDevice);
            _AlpacaDevice::_alpacaDevices.push_back(dev);

            AppCore::writeToApplicationLog("Found a FTDI device with port name: '" +
                ftdiDevice->_portName + "' and USB descriptor: '" + ftdiDevice->_usbDescriptor + "'\n");
        }
        else
        {
            existing->setActive();
        }
    }

    return static_cast<uint32_t>(_AlpacaDevice::_alpacaDevices.size());
}

bool FTDIDevice::open()
{
    const int maxIterations{50};
    bool result{false};

    FTDI_DBG("open() port='" + _portName + "' platformID=" + std::to_string(static_cast<int>(_platformID)) +
        " hasConfig=" + (_platformConfiguration ? "yes" : "NO"));

    if (!_platformConfiguration)
    {
        _lastError = "No platform configuration for platformID " +
            std::to_string(static_cast<int>(_platformID)) + " on port " + _portName;
        FTDI_DBG("ABORT: " + _lastError);
        return false;
    }

    // NOTE: programDevice() is NOT called here. It is a one-time EEPROM
    // programming operation that should be run from the Device Catalog tool
    // (or a separate setup script) BEFORE using the device for automation.
    // Calling it on every open() would:
    //   - Wear the EEPROM flash unnecessarily
    //   - Add a 3-second delay for USB re-enumeration
    //   - Potentially fail if another process has the device open
    //
    // If open() fails because the board is in VCP mode, the user needs to
    // run the Device Catalog or call programDevice() once to switch channels
    // C+D from VCP to D2XX mode. This persists in the FTDI EEPROM permanently.

    if (_platformConfiguration)
    {
        _ftdiPlatformConfiguration = static_cast<_FTDIPlatformConfiguration*>(_platformConfiguration.get());
        buildMapping();

        FTDI_DBG("Commands mapped: " + std::to_string(_commands.size()) +
            " pinSet=" + std::to_string(static_cast<int>(_ftdiPlatformConfiguration->getPinSet(0))));

        if (!_driveThread)
        {
            auto driveThread = std::make_unique<TACLiteDriveThread>(static_cast<uint32_t>(_hash));
            driveThread->setPinSets(_ftdiPlatformConfiguration->getPinSet(0));

            // Wire up pin state callback (replaces Qt connect())
            driveThread->onPinStateChanged = [this](uint64_t pin, bool state){
                this->on_pinStateChanged(pin, state);
            };

            _driveThread = std::move(driveThread);
            _driveThread->start();

            FTDI_DBG("Drive thread started, waiting for running...");

            for (int count = 0; count < maxIterations; ++count)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                result = _driveThread->weAreRunning();
                if (result) break;
            }

            FTDI_DBG(std::string("Drive thread ") +
                (result ? "IS running" : "FAILED to start (timed out after 1s)"));

            if (result)
            {
                Pins initialPins;
                Pins pins = _platformConfiguration->getPins();
                FTDI_DBG("Total pins from config: " + std::to_string(pins.size()));
                for (const auto& pin : pins)
                {
                    if (pin._initialValue)
                    {
                        initialPins.push_back(pin);
                        FTDI_DBG("  Init pin: cmd='" + pin._pinCommand + "' pin=" + std::to_string(pin._pin) +
                            " priority=" + std::to_string(pin._initializationPriority) + " value=true");
                    }
                }

                FTDI_DBG("Pins to initialize: " + std::to_string(initialPins.size()));

                if (!initialPins.empty())
                {
                    std::sort(initialPins.begin(), initialPins.end(),
                        [](const PinEntry& a, const PinEntry& b){ return a._initializationPriority < b._initializationPriority; });

                    for (const auto& pin : initialPins)
                    {
                        FTDI_DBG("  Setting pin " + std::to_string(pin._pin) + " (" + pin._pinCommand + ") = HIGH");
                        _driveThread->setPinState(pin._pin, pin._initialValue);
                    }
                }
            }
        }
    }

    return result;
}

void FTDIDevice::buildMapping()
{
    buildCommandList();
    buildQuickSettings();
}

void FTDIDevice::buildCommandList()
{
    if (_commands.empty())
    {
        FTDIPinList ftdiPinList = _ftdiPlatformConfiguration->getActivePins();
        for (const auto& ftdiPin : ftdiPinList)
        {
            TACCommand cmd;
            cmd._pin          = ftdiPin._setPin;
            cmd._command      = ftdiPin._pinCommand;
            cmd._helpText     = ftdiPin._pinTooltip;
            cmd._currentState = ftdiPin._initialValue;
            cmd._isInverted   = ftdiPin._inverted;
            cmd._tabName      = ftdiPin._tabName;
            cmd._groupName    = CommandGroup::toString(ftdiPin._commandGroup);
            cmd._cellLocation = std::to_string(ftdiPin._cellX) + "," + std::to_string(ftdiPin._cellY);
            _commands[cmd._command] = cmd;
        }
    }
}
