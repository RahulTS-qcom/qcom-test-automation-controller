// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

// Author: msimpson, biswroy

#include "TACLiteDriveThread.h"
#include "TACCommands.h"
#include "AlpacaDevice.h"
#include "TACCommandHashes.h"
#include "TACLiteCommand.h"
#include "AppCore.h"

#include "DebugLog.h"

#include <chrono>
#include <thread>

#define LITE_DBG(msg) TACDEV_DBG_TAG("LiteDrive", msg)

#define on true
#define off false

bool TACLiteDriveThread::_initialized{false};

static const std::string kTACLiteDriveTrainName{"TAC Lite Drive Train"};

TACLiteDriveThread::TACLiteDriveThread(uint32_t hash) : TACDriveThread(hash)
{
    LITE_DBG("Constructor hash=" + std::to_string(hash));
    _driveTrainName = kTACLiteDriveTrainName;
    setProtocolInterface(&_tacProtocol);
    _tacProtocol.setTACDriveTrain(this);

    AlpacaDevice alpacaDevice = _AlpacaDevice::findAlpacaDevice(hash);

    if (alpacaDevice && alpacaDevice->active())
    {
        LITE_DBG("Found device: port='" + alpacaDevice->portName() + "'");
        _ftdiChipset = _FTDIChipset::getDevice(alpacaDevice->portName());
        if (_ftdiChipset != nullptr)
        {
            _portName = _ftdiChipset->portName();
            _platformID = _ftdiChipset->platformID();
            _name = _ftdiChipset->serialNumber();

            _versionString = _ftdiChipset->versionString();
            _firmwareString = _ftdiChipset->firmwareString();
            _firmwareMajor = 1;
            _firmwareChip = 10000;
            _firmwareMinor = 1;
            _firmwareRevision = 1;
            _description = "VTP Port";
            _uuid = "FTDI - No UUID";
            _hardwareType = eFTDI;

            setSerialNumber(_ftdiChipset->serialNumber());
            setPortName(_ftdiChipset->portName());
            LITE_DBG("Chipset ready: port='" + _portName + "' serial='" + _ftdiChipset->serialNumber() + "'");
        }
        else
        {
            LITE_DBG("ERROR: _FTDIChipset::getDevice returned null for port '" + alpacaDevice->portName() + "'");
        }
    }
    else
    {
        LITE_DBG("ERROR: device not found or inactive for hash=" + std::to_string(hash));
    }
}

TACLiteDriveThread::~TACLiteDriveThread() {}

bool TACLiteDriveThread::openFTDIDevice()
{
    bool result{false};

    LITE_DBG("openFTDIDevice() chipset=" + std::string(_ftdiChipset != nullptr ? "valid" : "NULL") +
        " pinsets=" + std::to_string(static_cast<int>(_pinsets)));

    if (_ftdiChipset != nullptr)
    {
        if (_ftdiChipset->isOpen() == false)
        {
            LITE_DBG("Calling _ftdiChipset->open(pinsets=" + std::to_string(static_cast<int>(_pinsets)) + ")");
            if (_ftdiChipset->open(_pinsets))
            {
                LITE_DBG("openFTDIDevice SUCCESS");
                AppCore::writeToApplicationLogLine("Opened port " + _ftdiChipset->portName() + "\n");
                result = true;
            }
            else
            {
                LITE_DBG("openFTDIDevice FAILED — _ftdiChipset->open() returned false");
                AppCore::writeToApplicationLogLine("TACLiteDriveThread::openFTDIDevice _ftdiDevice->open(pinsets) failed");
                if (onErrorOnOpen) onErrorOnOpen("FTDI device open failed. Check the application log");
            }
        }
        else
        {
            LITE_DBG("openFTDIDevice — device already open");
            AppCore::writeToApplicationLogLine("TACLiteDriveThread::openFTDIDevice _ftdiDevice->isOpen() == true");
            if (onErrorOnOpen) onErrorOnOpen("FTDI device already open. Check the application log");
        }
    }
    else
    {
        LITE_DBG("openFTDIDevice — _ftdiChipset is NULL");
        AppCore::writeToApplicationLogLine("TACLiteDriveThread::openFTDIDevice _ftdiDevice is null");
        if (onErrorOnOpen) onErrorOnOpen("FTDI device open failed. Check the application log");
    }

    return result;
}

void TACLiteDriveThread::externalPowerControl(bool state)
{
    {
        TACLiteCommand tacCommand(this, this);
        tacCommand.externalPowerControl(state);
    }
    waitForCompletion();
}

void TACLiteDriveThread::setPinState(uint16_t pin, bool state)
{
    LITE_DBG("setPinState: pin=" + std::to_string(pin) + " state=" + (state ? "ON" : "OFF"));
    {
        TACLiteCommand tacCommand(this, this);
        tacCommand.setPinState(pin, state);
    }
    LITE_DBG("setPinState: calling waitForCompletion");
    waitForCompletion();
    LITE_DBG("setPinState: done");
}

void TACLiteDriveThread::sendCommandSequence(CommandEntries& commandEntries)
{
    LITE_DBG("sendCommandSequence: entries=" + std::to_string(commandEntries.size()));

    {
        TACLiteCommand tacCommand(this, this);

        for (const auto& commandEntry : commandEntries)
        {
            switch (commandEntry->_commandAction)
            {
            case _CommandEntry::eNotSet:
                LITE_DBG("  entry: eNotSet (skipped)");
                break;

            case _CommandEntry::eSetPin:
            {
                bool s = std::holds_alternative<bool>(commandEntry->_arguement)
                    ? std::get<bool>(commandEntry->_arguement) : false;
                LITE_DBG("  entry: eSetPin action='" + commandEntry->_action + "' pinID=" +
                    std::to_string(commandEntry->_pinID) + " state=" + (s ? "ON" : "OFF"));
                tacCommand.setPinState(static_cast<uint16_t>(commandEntry->_pinID), s);
                break;
            }

            case _CommandEntry::eLog:
            {
                std::string comment = std::holds_alternative<std::string>(commandEntry->_arguement)
                    ? std::get<std::string>(commandEntry->_arguement) : "";
                LITE_DBG("  entry: eLog '" + comment + "'");
                tacCommand.addLogComment(comment);
                break;
            }

            case _CommandEntry::eDelay:
            {
                uint32_t delay = std::holds_alternative<uint16_t>(commandEntry->_arguement)
                    ? static_cast<uint32_t>(std::get<uint16_t>(commandEntry->_arguement)) : 0;
                LITE_DBG("  entry: eDelay " + std::to_string(delay) + "ms");
                tacCommand.addDelay(delay);
                break;
            }

            case _CommandEntry::eBaseCommand:
                LITE_DBG("  entry: eBaseCommand (skipped)");
                break;
            }

            if (onProgress) onProgress(kProgressActive, eInfoNotification);
        }

        LITE_DBG("sendCommandSequence: TACLiteCommand destructor will call addEndTransaction");
    }

    LITE_DBG("sendCommandSequence: calling waitForCompletion");
    waitForCompletion();
    LITE_DBG("sendCommandSequence: waitForCompletion returned");
}

int TACLiteDriveThread::getResetCount() { return 0; }
void TACLiteDriveThread::clearResetCount() {}
void TACLiteDriveThread::i2CReadRegister(uint32_t /*addr*/, uint32_t /*reg*/) {}
void TACLiteDriveThread::i2CWriteRegister(uint32_t /*addr*/, uint32_t /*reg*/, uint32_t /*data*/) {}

void TACLiteDriveThread::setName(const std::string& /*newName*/) {}

uint32_t TACLiteDriveThread::send(const std::string& sendMe, const Arguments& arguments,
    bool console, ReceiveInterface* recieveInterface, bool store)
{
    return _tacProtocol.sendCommand(sendMe, arguments, console, recieveInterface, store);
}

bool TACLiteDriveThread::ready()
{
    return _tacProtocol.queueSize() == 0;
}

void TACLiteDriveThread::receive(FramePackage& framePackage)
{
    if (framePackage->_endTransaction == true)
    {
        clearWaitForCompletion();
        if (onTransactionEnded) onTransactionEnded();
        if (onProgress) onProgress(kProgressMax, eInfoNotification);
    }
    else if (framePackage->_valid == true)
    {
        switch (framePackage->_requestHash)
        {
        case kVersionCommandHash:
            handleVersionResponse(framePackage);
            break;

        case kGetPlatformIDCommandHash:
            handlePlatformID(framePackage);
            break;

        case kGetUUIDCommandHash:
            handleUUIDResponse(framePackage);
            break;

        case kSetNameCommandHash:
            handleSetName(framePackage);
            break;

        case kSetPinCommandHash:
            handleSetPin(framePackage);
            break;

        default:
            break;
        }
    }
    else
    {
        clearWaitForCompletion();

        if (!framePackage->_lastError.empty())
            if (onErrorOnOpen) onErrorOnOpen(framePackage->_lastError);
    }

    log(framePackage);
    _protocolInterface->clearPendingFrame();
}

void TACLiteDriveThread::run()
{
    AppCore::writeToApplicationLogLine("TACLiteDriveThread::run()");

    if (openFTDIDevice() == true)
    {
        AppCore::writeToApplicationLogLine("TACLiteDriveThread::run() openFTDIDevice() == true");
        if (onDeviceOpen) onDeviceOpen();
        startRunning();
    }

    if (weAreRunning())
    {
        if (onHardwareTypeUpdate) onHardwareTypeUpdate(_versionString);
        if (onHardwareVersionUpdate) onHardwareVersionUpdate("0.1.1");
        if (onFirmwareVersionUpdate) onFirmwareVersionUpdate(_firmwareString);
        if (onNameUpdate) onNameUpdate(_name);
        if (onUuidUpdate) onUuidUpdate(_uuid);
        if (onSerialNumUpdate) onSerialNumUpdate(_serialNumber);
        if (onPlatformIDUpdate) onPlatformIDUpdate(static_cast<int>(_platformID));

        if (onDeviceOpen) onDeviceOpen();
        if (onDeviceStatusChange) onDeviceStatusChange("Starting");

        AppCore::writeToApplicationLogLine("setupConnected()");
        setupConnected();
    }
    else
    {
        AppCore::writeToApplicationLogLine("TACLiteDriveThread::run() emit deviceDisconnected()");
        if (onDeviceDisconnected) onDeviceDisconnected();
    }

    AppCore::writeToApplicationLogLine("Hardware: " + std::to_string(_hardwareType));
    AppCore::writeToApplicationLogLine("Platform ID: " + PlatformContainer::toString(_platformID) +
        "(" + std::to_string(static_cast<int>(_platformID)) + ")\n");

    if (weAreRunning())
    {
        bool loopFinished{false};

        LITE_DBG("Main loop starting");

        while (!loopFinished)
        {
            FramePackage framePackage = _protocolInterface->getNextFramePackage();
            if (framePackage != nullptr)
            {
                if (framePackage->_delayInMilliSeconds != 0)
                {
                    LITE_DBG("Processing DELAY: " + std::to_string(framePackage->_delayInMilliSeconds) + "ms");
                    receive(framePackage);
                }
                else if (!framePackage->_comment.empty())
                {
                    LITE_DBG("Processing LOG: " + framePackage->_comment);
                    receive(framePackage);
                }
                else if (framePackage->_endTransaction == true)
                {
                    LITE_DBG("Processing END_TRANSACTION");
                    receive(framePackage);
                }
                else if (checkLocalStore(framePackage) == true)
                {
                    LITE_DBG("Processing LOCAL_STORE: " + framePackage->_request);
                    receive(framePackage);
                }
                else
                {
                    // Get pin number from coded request
                    int pin = 0;
                    try { pin = std::stoi(framePackage->_codedRequest); }
                    catch (...) {
                        LITE_DBG("WARNING: stoi failed for codedRequest='" + framePackage->_codedRequest + "'");
                    }

                    bool state = getElectrialPinValue(framePackage->_requestHash, framePackage->_arguments);

                    LITE_DBG("Processing PIN_WRITE: cmd='" + framePackage->_request +
                        "' codedRequest='" + framePackage->_codedRequest +
                        "' pin=" + std::to_string(pin) + " state=" + (state ? "ON" : "OFF") +
                        " hash=" + std::to_string(framePackage->_requestHash) +
                        " args=" + std::to_string(framePackage->_arguments.size()));

                    if (_ftdiChipset != nullptr)
                    {
                        bool writeOk = _ftdiChipset->write(static_cast<uint8_t>(pin), state);
                        LITE_DBG("  ftdiChipset->write(pin=" + std::to_string(pin) + ", " + (state ? "ON" : "OFF") + ") = " + (writeOk ? "OK" : "FAILED"));

                        if (writeOk == false)
                        {
                            LITE_DBG("  WRITE FAILURE — stopping");
                            stopRunning();
                        }
                        else
                        {
                            std::string argument;

                            if (!framePackage->_arguments.empty() &&
                                std::holds_alternative<bool>(framePackage->_arguments.at(0)))
                            {
                                argument = std::get<bool>(framePackage->_arguments.at(0)) ? "on" : "off";
                            }
                            else if (!framePackage->_arguments.empty() &&
                                     std::holds_alternative<std::string>(framePackage->_arguments.at(0)))
                            {
                                argument = std::get<std::string>(framePackage->_arguments.at(0));
                            }

                            framePackage->_responses.push_back(framePackage->_request + " " + argument);
                            receive(framePackage);
                        }
                    }
                    else
                    {
                        LITE_DBG("  FATAL: ftdiChipset is NULL");
                        stopRunning();
                    }
                }
            }
            else
            {
                if (weAreRunning() == false)
                {
                    LITE_DBG("Main loop finished (stop signal received)");
                    loopFinished = true;
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        }
    }

    AppCore::writeToApplicationLogLine("TACLiteDriveThread::run() _ftdiDevice->close()");

    if (_ftdiChipset != nullptr)
        _ftdiChipset->close();

    _connected = false;

    if (onDeviceDisconnected) onDeviceDisconnected();

    shutdownLogging();
}

void TACLiteDriveThread::sendCommand(const std::string& command, bool console,
    ReceiveInterface* receiveInterface, bool shouldStore)
{
    if (command.find(kHelpCommand) != 0)
    {
        Arguments args;
        std::string decodedCommand = decodeCommand(command, args);
        send(decodedCommand, args, console, receiveInterface, shouldStore);
    }
}

bool TACLiteDriveThread::getElectrialPinValue(uint32_t kCommandHash, const Arguments& arguments)
{
    bool result{false};

    if (!arguments.empty() && std::holds_alternative<bool>(arguments.at(0)))
    {
        bool argValue = std::get<bool>(arguments.at(0));
        switch (kCommandHash)
        {
        case kSetPinCommandHash:
            result = argValue;
            break;
        }
    }

    return result;
}

void TACLiteDriveThread::handleSetName(FramePackage& framePackage)
{
    FrameArgument arg = framePackage->getArgument(0);
    if (std::holds_alternative<std::string>(arg))
    {
        _name = std::get<std::string>(arg);
        framePackage->_synonym = "Set Name " + _name;
    }
    if (onNameUpdate) onNameUpdate(_name);
}

void TACLiteDriveThread::handleSetPin(FramePackage& framePackage)
{
    FrameArgument stateArg = framePackage->getArgument(0);
    FrameArgument pinArg = framePackage->getArgument(1);

    bool state = std::holds_alternative<bool>(stateArg) ? std::get<bool>(stateArg) : false;
    uint32_t pin = std::holds_alternative<uint32_t>(pinArg) ? std::get<uint32_t>(pinArg) : 0;

    framePackage->_synonym = "Set Pin " + std::to_string(pin) + " " + (state ? "on" : "off");

    if (onPinStateChanged) onPinStateChanged(static_cast<uint64_t>(pin), state);
}

void TACLiteDriveThread::handleUUIDResponse(FramePackage& framePackage)
{
    if (framePackage->_responses.size() > 1)
    {
        _uuid = framePackage->_responses.at(1);
        if (onUuidUpdate) onUuidUpdate(_uuid);
    }
}

void TACLiteDriveThread::handleVersionResponse(FramePackage& framePackage)
{
    (void)framePackage;
    if (onDeviceStatusChange) onDeviceStatusChange("TAC Version Good");
    setupConnected();
}

void TACLiteDriveThread::handlePlatformID(FramePackage& framePackage)
{
    if (framePackage->_responses.size() >= 2)
    {
        try
        {
            int boardID = std::stoi(framePackage->_responses.at(1));
            _platformID = static_cast<PlatformID>(boardID);
        }
        catch (...) {}
    }

    if (onPlatformIDUpdate) onPlatformIDUpdate(static_cast<int>(_platformID));
    setupConnected();
}

void TACLiteDriveThread::setupConnected()
{
    if (_connected == false)
    {
        _connected = true;
        if (onDeviceConnected) onDeviceConnected();
    }
}

void TACLiteDriveThread::setupDiscovery()
{
    // nothing to be done here
}

void TACLiteDriveThread::handleIdle(FramePackage& /*framePackage*/) {}

std::string TACLiteDriveThread::portDescription() { return _description; }
