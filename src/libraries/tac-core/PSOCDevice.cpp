#include "PSOCDevice.h"
#include "private/TACPSOCDriveThread.h"

#include "AppCore.h"
#include "PSOCPlatformConfiguration.h"
#include "SerialPortInfo.h"

#include "DebugLog.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>

#define PSOC_DBG(msg) TACDEV_DBG_TAG("PSOC", msg)

PSOCDevice::~PSOCDevice()
{
}

uint32_t PSOCDevice::updateAlpacaDevices()
{
    // VID/PID filter — only enumerate serial ports that match known PSOC/Cypress boards.
    // Matches original Qt behavior via PSOCSerialTableModel filter keys.
    static constexpr uint16_t kPSOC_VID_1 = 0x05C6;  // Qualcomm
    static constexpr uint16_t kPSOC_PID_1 = 0x9302;
    static constexpr uint16_t kPSOC_VID_2 = 0x16C0;  // Teensy/Cypress
    static constexpr uint16_t kPSOC_PID_2 = 0x0483;

    PSOC_DBG("updateAlpacaDevices: Enumerating serial ports...");
    PSOC_DBG("Looking for VID:PID 0x05C6:0x9302 or 0x16C0:0x0483");

    SerialPortInfos ports = SerialPortInfo::availablePorts();
    PSOC_DBG("Total serial ports found: " + std::to_string(ports.size()));

    for (const auto& portInfo : ports)
    {
        // Log every port we see (helps debug enumeration issues)
        char vidBuf[16], pidBuf[16];
        std::snprintf(vidBuf, sizeof(vidBuf), "0x%04X", portInfo.vendorIdentifier());
        std::snprintf(pidBuf, sizeof(pidBuf), "0x%04X", portInfo.productIdentifier());
        PSOC_DBG("  Port: '" + portInfo.portName() +
            "' desc='" + portInfo.description() +
            "' serial='" + portInfo.serialNumber() +
            "' VID=" + vidBuf + " PID=" + pidBuf);

        // Only accept ports matching known PSOC VID/PID pairs
        if (!portInfo.matchesVidPid(kPSOC_VID_1, kPSOC_PID_1) &&
            !portInfo.matchesVidPid(kPSOC_VID_2, kPSOC_PID_2))
        {
            PSOC_DBG("    -> SKIPPED (VID/PID mismatch)");
            continue;
        }

        PSOC_DBG("    -> MATCHED PSOC VID/PID");

        std::string portName = portInfo.portName();
        HashType hash = ::arrayHash(portName);

        AlpacaDevice existing = _AlpacaDevice::findAlpacaDevice(hash);
        if (!existing)
        {
            PSOCDevice* psocDevice = new PSOCDevice;
            psocDevice->_portName     = portName;
            psocDevice->_hash         = hash;
            psocDevice->_active       = true;
            psocDevice->_boardType    = ePSOC;
            psocDevice->_platformID   = MICRO_EPM_BOARD_ID_UNKNOWN;
            psocDevice->_description  = portInfo.description();
            psocDevice->_serialNumber = portInfo.serialNumber();

            _AlpacaDevice::_alpacaDevices.push_back(AlpacaDevice(psocDevice));
            PSOC_DBG("Added new PSOC device: port='" + portName +
                "' serial='" + portInfo.serialNumber() + "' hash=" + std::to_string(hash));
        }
        else
        {
            existing->setActive(true);
            PSOC_DBG("Device already known, reactivated: '" + portName + "'");
        }
    }

    PSOC_DBG("updateAlpacaDevices complete. Total devices: " +
        std::to_string(_AlpacaDevice::_alpacaDevices.size()));
    return static_cast<uint32_t>(_AlpacaDevice::_alpacaDevices.size());
}

bool PSOCDevice::open()
{
    bool result{false};

    PSOC_DBG("open() called: port='" + _portName +
        "' serial='" + _serialNumber + "' hash=" + std::to_string(_hash) +
        "' boardType=" + debugBoardTypeToString(_boardType));

    if (!_driveThread)
    {
        PSOC_DBG("Creating TACPSOCDriveThread...");
        _driveThread = std::make_unique<TACPSOCDriveThread>(static_cast<uint32_t>(_hash));

        _driveThread->onPinStateChanged = [this](uint64_t pin, bool state){
            this->on_pinStateChanged(pin, state);
        };

        PSOC_DBG("Starting drive thread...");
        _driveThread->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Wait for thread to reach running state (serial port opened inside run())
        for (int i = 0; i < 60; ++i)
        {
            if (_driveThread->weAreRunning())
            {
                PSOC_DBG("Drive thread is running (iteration " + std::to_string(i) + ")");
                break;
            }
            // If thread exited already (open failed), stop waiting
            if (!_driveThread->isRunning())
            {
                _lastError = _driveThread->lastErrorMessage();
                if (_lastError.empty())
                    _lastError = "PSOC drive thread exited unexpectedly (serial port open failed?)";
                PSOC_DBG("Drive thread EXITED early: " + _lastError);
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        if (!_lastError.empty())
        {
            PSOC_DBG("open() FAILED (thread error): " + _lastError);
        }
        else
        {
            // Wait for platform ID discovery
            PSOC_DBG("Waiting for platform ID discovery...");
            for (int i = 0; i < 20; ++i)
            {
                PlatformID platformID = _driveThread->platformID();
                if (platformID != MICRO_EPM_BOARD_ID_UNKNOWN)
                {
                    PSOC_DBG("Platform ID discovered: " +
                        std::to_string(static_cast<int>(platformID)) +
                        " (" + PlatformContainer::toString(platformID) + ")");
                    _platformID = platformID;
                    break;
                }

                if (_driveThread->oldFirmware())
                {
                    _lastError = "Firmware version is too old";
                    PSOC_DBG("Old firmware: " + _lastError);
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if (_lastError.empty() && _platformID != MICRO_EPM_BOARD_ID_UNKNOWN)
            {
                TACPlatformEntry platformEntry = _PlatformConfiguration::getEntry(_platformID);
                if (platformEntry._platformEntry)
                {
                    _description = platformEntry._platformEntry->_description;
                    _platformConfiguration = platformEntry.getConfiguration();
                    _psocPlatformConfiguration = static_cast<_PSOCPlatformConfiguration*>(_platformConfiguration.get());
                    result = true;
                    PSOC_DBG("Loaded platform config: '" + _description + "'");
                }
                else
                {
                    _description = "Unknown Platform (ID: " + std::to_string(static_cast<int>(_platformID)) + ")";
                    _lastError = "No platform configuration found for ID " + std::to_string(static_cast<int>(_platformID));
                    PSOC_DBG("WARNING: " + _lastError);
                }
            }
            else if (_lastError.empty())
            {
                // Prefer the drive thread's real failure (e.g. "Failed to open
                // COM21: Access is denied.") over the generic timeout — the port
                // may never have opened, in which case discovery could not run.
                std::string threadError = _driveThread->lastErrorMessage();
                _lastError = !threadError.empty()
                    ? threadError
                    : "PSOC board did not respond to discovery commands within timeout";
                PSOC_DBG(_lastError);
            }
        }
    }
    else
    {
        PSOC_DBG("open() skipped — drive thread already exists");
    }

    if (result)
    {
        PSOC_DBG("Building command mapping...");
        buildMapping();
        PSOC_DBG("Commands mapped: " + std::to_string(_commands.size()));

        Pins initialPins;
        Pins pins = _platformConfiguration->getPins();
        for (const auto& pin : pins)
            if (pin._initializationPriority > 0) initialPins.push_back(pin);
        for (const auto& pin : pins)
            if (pin._initialValue) initialPins.push_back(pin);

        if (!initialPins.empty())
        {
            std::sort(initialPins.begin(), initialPins.end(),
                [](const PinEntry& a, const PinEntry& b){ return a._initializationPriority < b._initializationPriority; });
            PSOC_DBG("Initializing " + std::to_string(initialPins.size()) + " pins...");
            for (const auto& pin : initialPins)
            {
                _driveThread->setPinState(pin._pin, true);
                PSOC_DBG("  Pin " + std::to_string(pin._pin) +
                    " (" + pin._pinCommand + ") = HIGH");
            }
        }

        PSOC_DBG("open() SUCCESS");
    }
    else if (_lastError.empty())
    {
        _lastError = "PSOC open failed: platform ID not discovered within timeout";
        PSOC_DBG("open() FAILED: " + _lastError);
    }

    if (_driveThread) _chipVersion = _driveThread->chipVersion();
    PSOC_DBG("open() returning " + std::string(result ? "true" : "false") +
        " chipVersion=" + std::to_string(_chipVersion));
    return result;
}

void PSOCDevice::buildMapping()
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
