// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

// Author: msimpson

#include "TACPSOCDriveThread.h"
#include "TACCommands.h"
#include "AlpacaDevice.h"
#include "TACCommandHashes.h"
#include "TACPSOCCommand.h"
#include "AppCore.h"
#include "StringUtilities.h"

#include "DebugLog.h"

#include <chrono>
#include <thread>

#define PSOC_DBG(msg) TACDEV_DBG_TAG("PSOC-Thread", msg)

#define on true
#define off false

bool TACPSOCDriveThread::_initialized{false};

static const std::string kTACSerialDriveTrainName{"TAC Serial Drive Train"};

TACPSOCDriveThread::TACPSOCDriveThread(uint32_t hash) : TACDriveThread(hash)
{
    _driveTrainName = kTACSerialDriveTrainName;
    setProtocolInterface(&_tacProtocol);
    _tacProtocol.setTACDriveTrain(this);

    AlpacaDevice alpacaDevice = _AlpacaDevice::findAlpacaDevice(hash);
    if (alpacaDevice && alpacaDevice->active())
    {
        _portName = alpacaDevice->portName();
        _serialNumber = alpacaDevice->serialNumber();
        _description = alpacaDevice->description();
    }
}

TACPSOCDriveThread::~TACPSOCDriveThread()
{
    if (_serialPort)
    {
        _serialPort->close();
        delete _serialPort;
        _serialPort = nullptr;
    }
}

bool TACPSOCDriveThread::openSerialDevice()
{
    bool result{false};

    // Find serial port by name
    auto ports = SerialPortInfo::availablePorts();
    PSOC_DBG("Looking for port '" + _portName + "' among " +
        std::to_string(ports.size()) + " available ports");

    for (const auto& portInfo : ports)
    {
        if (portInfo.portName() == _portName || portInfo.serialNumber() == _portName)
        {
            _tacPortInfo = portInfo;
            PSOC_DBG("Matched port: '" + portInfo.portName() +
                "' serial='" + portInfo.serialNumber() + "'");
            break;
        }
    }

    if (_tacPortInfo.portName().empty())
    {
        _lastErrorMessage = "Port '" + _portName + "' not found in available serial ports";
        PSOC_DBG(_lastErrorMessage);
        return false;
    }

    _serialPort = new SerialPort(_tacPortInfo);

    SerialPortSettings settings = _serialPort->getSerialPortSettings();
    settings._baudRate = 115200;
    _serialPort->setSerialPortSettings(settings);

    if (_serialPort->open())
    {
        setSerialNumber(_tacPortInfo.serialNumber());
        setPortName(_tacPortInfo.portName());

        if (onSerialNumUpdate) onSerialNumUpdate(_serialNumber);

        PSOC_DBG("Device " + _tacPortInfo.serialNumber() + " opened");
        PSOC_DBG("Com port " + _tacPortInfo.portName());

        result = true;
    }
    else
    {
        std::string err = serialPortError();
        _lastErrorMessage = "Failed to open " + _tacPortInfo.portName() + ": " + err;
        PSOC_DBG("" + _lastErrorMessage);
        if (onErrorOnOpen) onErrorOnOpen(_lastErrorMessage);

        delete _serialPort;
        _serialPort = nullptr;
    }

    return result;
}

void TACPSOCDriveThread::sendCommand(const std::string& command, bool console,
    ReceiveInterface* receiveInterface, bool shouldStore)
{
    if (command.find(kHelpCommand) != 0)
    {
        Arguments args;
        std::string decodedCommand = decodeCommand(command, args);
        send(decodedCommand, args, console, receiveInterface, shouldStore);
    }
}

void TACPSOCDriveThread::endTransaction(ReceiveInterface* receiveInterface)
{
    _tacProtocol.endTransaction(receiveInterface);
}

void TACPSOCDriveThread::setPinState(uint16_t pin, bool state)
{
    {
        TACPSOCCommand tacCommand(this, this);
        tacCommand.setPinState(pin, state);
    }
    waitForCompletion();
}

void TACPSOCDriveThread::sendCommandSequence(CommandEntries& commandEntries)
{
    TACPSOCCommand tacCommand(this, this);

    for (const auto& commandEntry : commandEntries)
    {
        switch (commandEntry->_commandAction)
        {
        case _CommandEntry::eNotSet:
            break;

        case _CommandEntry::eSetPin:
        {
            bool s = std::holds_alternative<bool>(commandEntry->_arguement)
                ? std::get<bool>(commandEntry->_arguement) : false;
            tacCommand.setPinState(static_cast<uint16_t>(commandEntry->_pinID), s);
            break;
        }

        case _CommandEntry::eLog:
        {
            std::string comment = std::holds_alternative<std::string>(commandEntry->_arguement)
                ? std::get<std::string>(commandEntry->_arguement) : "";
            tacCommand.addLogComment(comment + "\n");
            break;
        }

        case _CommandEntry::eDelay:
        {
            uint32_t delay = std::holds_alternative<uint16_t>(commandEntry->_arguement)
                ? static_cast<uint32_t>(std::get<uint16_t>(commandEntry->_arguement)) : 0;
            tacCommand.addDelay(delay);
            break;
        }

        case _CommandEntry::eBaseCommand:
            break;
        }

        if (onProgress) onProgress(kProgressActive, eInfoNotification);
    }
}

int TACPSOCDriveThread::getResetCount()
{
    {
        TACPSOCCommand tacCommand(this, this);
        tacCommand.getResetCount();
    }
    waitForCompletion();
    return _resetCount;
}

void TACPSOCDriveThread::clearResetCount()
{
    {
        TACPSOCCommand tacCommand(this, this);
        tacCommand.clearResetCount();
    }
    waitForCompletion();
}

void TACPSOCDriveThread::i2CReadRegister(uint32_t addr, uint32_t reg)
{
    {
        TACPSOCCommand tacCommand(this, this);
        try { tacCommand.i2CReadRegister(addr, reg); }
        catch (...) {}
    }
    waitForCompletion();
}

void TACPSOCDriveThread::i2CWriteRegister(uint32_t addr, uint32_t reg, uint32_t data)
{
    {
        TACPSOCCommand tacCommand(this, this);
        try { tacCommand.i2CWriteRegister(addr, reg, data); }
        catch (...) {}
    }
    waitForCompletion();
}

void TACPSOCDriveThread::setName(const std::string& newName)
{
    if (newName.length() <= 32 && newName != _name && isAlphaNumeric(newName))
    {
        {
            TACPSOCCommand tacCommand(this, this);
            tacCommand.setName(newName);
        }
        waitForCompletion();
    }
}

uint32_t TACPSOCDriveThread::send(const std::string& sendMe, const Arguments& arguments,
    bool console, ReceiveInterface* recieveInterface, bool store)
{
    return _tacProtocol.sendCommand(sendMe, arguments, console, recieveInterface, store);
}

bool TACPSOCDriveThread::ready()
{
    return _tacProtocol.queueSize() == 0;
}

std::string TACPSOCDriveThread::serialPortError()
{
    if (_serialPort != nullptr)
        return _serialPort->errorString();
    return "";
}

void TACPSOCDriveThread::on_readyRead()
{
    _readyRead = true;
}

bool TACPSOCDriveThread::readSerialData()
{
    bool result{false};

    // Read with a short blocking timeout — replaces the unreliable
    // _readyRead flag + waitForReadyRead approach.
    std::string buffer = _serialPort->readAll(50);
    if (!buffer.empty())
    {
        PSOC_DBG("readSerialData: got " + std::to_string(buffer.size()) + " bytes: [" +
            buffer.substr(0, 80) + "]");
        _protocolInterface->handleRecievedData(buffer);
        result = true;
    }

    return result;
}

void TACPSOCDriveThread::receive(FramePackage& framePackage)
{
    if (framePackage->_endTransaction == true)
    {
        clearWaitForCompletion();
        if (onTransactionEnded) onTransactionEnded();
        if (onProgress) onProgress(kProgressMax, eInfoNotification);
    }
    else
    {
        if (framePackage->_valid == true)
        {
            switch (framePackage->_requestHash)
            {
            case kVersionCommandHash: handleVersionResponse(framePackage); break;
            case kGetPlatformIDCommandHash: handlePlatformID(framePackage); break;
            case kGetUUIDCommandHash: handleUUIDResponse(framePackage); break;
            case kGetNameCommandHash: handleGetNameResponse(framePackage); break;
            case kSetNameCommandHash: handleSetName(framePackage); break;
            case kClearResetCountCommandHash:
                if (onResetCountCleared) onResetCountCleared();
                break;
            case kI2CReadRegisterCommandHash:
            case kI2CReadRegisterValueCommandHash:
                handleI2CRead(framePackage); break;
            case kI2CWriteRegisterCommandHash: handleI2CWrite(framePackage); break;
            case kSetPinCommandHash: handleSetPin(framePackage); break;
            default: break;
            }
        }
        else
        {
            clearWaitForCompletion();

            switch (framePackage->_requestHash)
            {
            case kGetPlatformIDCommandHash:
            {
                TACPSOCCommand tacCommand(this, this);
                tacCommand.version();
                break;
            }
            case kI2CReadRegisterCommandHash:
            case kI2CReadRegisterValueCommandHash:
                handleI2CRead(framePackage); break;
            case kI2CWriteRegisterCommandHash:
                handleI2CWrite(framePackage); break;
            default: break;
            }
        }
    }

    log(framePackage);
    _protocolInterface->clearPendingFrame();
}

void TACPSOCDriveThread::run()
{
    AppCore::getAppCore()->setRunLogging(AppCore::getAppCore()->getPreferences()->loggingActive());
    PSOC_DBG("run() started, port='" + _portName + "'");

    if (openSerialDevice() == true)
    {
        PSOC_DBG("Serial device opened OK, sending discovery commands...");
        if (onDeviceOpen) onDeviceOpen();

        // Register readyRead callback on serial port
        _serialPort->onReadyRead = [this]() { on_readyRead(); };

        startRunning();

        if (onDeviceStatusChange) onDeviceStatusChange("Starting");

        _readyRead = false;
        _connected = false;

        {
            TACPSOCCommand tacCommand(this, this);
            PSOC_DBG("Sending platformID command...");
            tacCommand.platformID();
            PSOC_DBG("Sending version command...");
            tacCommand.version();
        }

        if (weAreRunning())
        {
            bool loopFinished{false};

            while (!loopFinished)
            {
                FramePackage framePackage = _protocolInterface->getNextFramePackage();
                if (framePackage != nullptr)
                {
                    if (framePackage->_delayInMilliSeconds != 0 ||
                        !framePackage->_comment.empty() ||
                        framePackage->_endTransaction == true ||
                        checkLocalStore(framePackage) == true)
                    {
                        receive(framePackage);
                    }
                    else
                    {
                        PSOC_DBG("WRITE cmd='" + framePackage->_request + "' encoded=" +
                            std::to_string(framePackage->_codedRequest.size()) + " bytes: [" +
                            framePackage->_codedRequest.substr(0, 60) + "]");
                        int64_t bytesWritten = _serialPort->write(framePackage->_codedRequest);
                        PSOC_DBG("WRITE result: " + std::to_string(bytesWritten) + " bytes written");
                        if (bytesWritten == -1)
                        {
                            PSOC_DBG("WRITE FAILED: " + _serialPort->errorString());
                            stopRunning();
                        }
                    }
                }
                else
                {
                    if (weAreRunning() == false)
                        loopFinished = true;
                }

                // Always try to read — readSerialData() blocks up to 10ms
                if (readSerialData())
                {
                    _protocolInterface->idle();
                }
            }
        }

        _serialPort->close();
        PSOC_DBG("Serial port closed, run() exiting normally");
        _connected = false;
    }
    else
    {
        PSOC_DBG("openSerialDevice() FAILED — run() exiting");
        if (onErrorOnOpen) onErrorOnOpen(_lastErrorMessage.empty() ? "PSOC device open failed" : _lastErrorMessage);
    }

    shutdownLogging();
}

void TACPSOCDriveThread::handleGetNameResponse(FramePackage& framePackage)
{
    if (framePackage->_responses.size() > 1)
    {
        std::lock_guard<std::recursive_mutex> lk(_stateMutex);
        _name = framePackage->_responses.at(1);
        if (onNameUpdate) onNameUpdate(_name);
    }
}

void TACPSOCDriveThread::handleGetResetCount(FramePackage& framePackage)
{
    if (framePackage != nullptr)
    {
        if (framePackage->_responses.size() >= 3)
        {
            if (framePackage->_responses.at(0) == "getresetcount")
            {
                if (framePackage->_responses.at(2) == "ok")
                {
                    std::lock_guard<std::recursive_mutex> lk(_stateMutex);
                    try { _resetCount = std::stoi(framePackage->_responses.at(1)); }
                    catch (...) {}
                }
            }
        }
    }

    if (onResetCountUpdate) onResetCountUpdate(static_cast<uint32_t>(_resetCount));
}

void TACPSOCDriveThread::handleI2CRead(FramePackage& framePackage)
{
    if (framePackage->_requestHash == kI2CReadRegisterValueCommandHash)
    {
        if (framePackage->_valid)
        {
            std::string response;
            auto responses = framePackage->_responses;
            if (!responses.empty())
            {
                responses.pop_back();
                response = responses.empty() ? framePackage->_responses.back() : responses.back();
            }
            if (onI2CReadResult) onI2CReadResult(response, true);
        }
        else
        {
            std::string lastResp = framePackage->_responses.empty() ? "" : framePackage->_responses.back();
            if (onI2CReadResult) onI2CReadResult(lastResp, false);
        }
    }
}

void TACPSOCDriveThread::handleI2CWrite(FramePackage& framePackage)
{
    if (framePackage->_valid)
    {
        FrameArgument arg0 = framePackage->getArgument(0);
        std::string argStr = std::holds_alternative<std::string>(arg0) ? std::get<std::string>(arg0) :
                             std::holds_alternative<uint32_t>(arg0) ? std::to_string(std::get<uint32_t>(arg0)) : "";
        std::string result = framePackage->_request + " " + argStr + " Successful";
        if (onI2CWriteResult) onI2CWriteResult(result);
    }
    else
    {
        std::string lastResp = framePackage->_responses.empty() ? "" : framePackage->_responses.back();
        std::string result = "\"" + lastResp + "\" Are the parameters correct?";
        if (onI2CWriteResult) onI2CWriteResult(result);
    }
}

void TACPSOCDriveThread::handleSetPin(FramePackage& framePackage)
{
    FrameArgument stateArg = framePackage->getArgument(0);
    FrameArgument pinArg = framePackage->getArgument(1);

    bool state = std::holds_alternative<bool>(stateArg) ? std::get<bool>(stateArg) : false;
    uint32_t pin = std::holds_alternative<uint32_t>(pinArg) ? std::get<uint32_t>(pinArg) : 0;

    framePackage->_synonym = "Pin " + std::to_string(pin) + " " + (state ? "on" : "off");

    if (onPinStateChanged) onPinStateChanged(static_cast<uint64_t>(pin), state);
}

void TACPSOCDriveThread::handleSetName(FramePackage& framePackage)
{
    std::lock_guard<std::recursive_mutex> lk(_stateMutex);
    FrameArgument arg = framePackage->getArgument(0);
    if (std::holds_alternative<std::string>(arg))
    {
        _name = std::get<std::string>(arg);
        framePackage->_synonym = "Set Name " + _name;
    }
    if (onNameUpdate) onNameUpdate(_name);
}

void TACPSOCDriveThread::handleUUIDResponse(FramePackage& framePackage)
{
    if (framePackage->_responses.size() > 1)
    {
        std::lock_guard<std::recursive_mutex> lk(_stateMutex);
        _uuid = framePackage->_responses.at(1);
        if (onUuidUpdate) onUuidUpdate(_uuid);
    }
}

void TACPSOCDriveThread::handleVersionResponse(FramePackage& framePackage)
{
    static int retryCount{1};

    if (framePackage->_responses.size() > 1)
    {
        std::lock_guard<std::recursive_mutex> lk(_stateMutex);
        bool isEPM{false};
        std::string versionString = framePackage->_responses.at(0);
        if (versionString.find("EPM") != std::string::npos)
            isEPM = true;
        else
        {
            versionString = framePackage->_responses.at(1);
            if (versionString.find("EPM") != std::string::npos)
                isEPM = true;
        }

        _versionString = versionString;
        AppCore::writeToApplicationLogLine("Version String: " + _versionString);

        retryCount = 1;

        if (isEPM)
            _hardwareType = ePSOC;
        else
            _hardwareType = eSpiderBoard;

        if (onHardwareTypeUpdate) onHardwareTypeUpdate(debugBoardTypeString());

        // Parse firmware version from version string
        // Format: "EPM ... FW : X.Y.Z.W ..."
        auto fwPos = _versionString.find("FW");
        if (fwPos != std::string::npos)
        {
            // Find version number after "FW" and delimiter
            auto colonPos = _versionString.find(':', fwPos);
            if (colonPos != std::string::npos && colonPos + 2 < _versionString.size())
            {
                std::string fwPart = _versionString.substr(colonPos + 2);
                // Trim to first space
                auto spacePos = fwPart.find(' ');
                if (spacePos != std::string::npos)
                    fwPart = fwPart.substr(0, spacePos);

                _firmwareString = fwPart;

                // Parse X.Y.Z.W
                try
                {
                    size_t pos = 0;
                    _firmwareMajor = std::stoul(fwPart, &pos);
                    if (pos < fwPart.size() && fwPart[pos] == '.') { fwPart = fwPart.substr(pos + 1); }
                    _firmwareChip = std::stoul(fwPart, &pos);
                    if (pos < fwPart.size() && fwPart[pos] == '.') { fwPart = fwPart.substr(pos + 1); }
                    _firmwareMinor = std::stoul(fwPart, &pos);
                    if (pos < fwPart.size() && fwPart[pos] == '.') { fwPart = fwPart.substr(pos + 1); }
                    _firmwareRevision = std::stoul(fwPart, &pos);

                    if (_firmwareMinor < 15)
                        _oldFirmware = true;
                }
                catch (...) {}

                if (onFirmwareVersionUpdate) onFirmwareVersionUpdate(_firmwareString);
            }
        }

        if (onDeviceStatusChange) onDeviceStatusChange("TAC Version Good");
        setupConnected();
    }
    else if (retryCount < 4)
    {
        AppCore::writeToApplicationLogLine("Version Failed Retry " + std::to_string(retryCount));
        TACPSOCCommand tacCommand(this, this);
        tacCommand.version();
        retryCount++;
    }
}

void TACPSOCDriveThread::handlePlatformID(FramePackage& framePackage)
{
    std::lock_guard<std::recursive_mutex> lk(_stateMutex);
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

void TACPSOCDriveThread::setupConnected()
{
    if (_connected == false)
    {
        if (onHardwareVersionUpdate) onHardwareVersionUpdate(hardwareVersionString());

        _connected = true;

        AppCore::writeToApplicationLogLine("");
        TACPSOCCommand tacCommand(this, this);

        AppCore::writeToApplicationLogLine("tacCommand.uuid()");
        tacCommand.uuid();

        AppCore::writeToApplicationLogLine("tacCommand.name()");
        tacCommand.name();

        if (onDeviceConnected) onDeviceConnected();
    }
}

void TACPSOCDriveThread::setupDiscovery()
{
    if (onHardwareVersionUpdate) onHardwareVersionUpdate(hardwareVersionString());

    TACPSOCCommand tacCommand(this, this);
    tacCommand.uuid();
    tacCommand.name();
    tacCommand.platformID();
}

void TACPSOCDriveThread::handleIdle(FramePackage& /*framePackage*/) {}

void TACPSOCDriveThread::log(FramePackage& framePackage)
{
    AppCore::writeToApplicationLogLine("");
    AppCore::writeToApplicationLogLine("Frame Package Start");
    AppCore::writeToApplicationLogLine(framePackage->_valid ? "Frame Valid" : "Frame Invalid");

    if (framePackage->_delayInMilliSeconds > 0)
    {
        std::string logEntry = "Delay " + std::to_string(framePackage->_delayInMilliSeconds) + " in msecs";
        timeStampLogMessage(logEntry);
        AppCore::writeToApplicationLogLine(logEntry);
    }
    else if (!framePackage->_comment.empty())
    {
        std::string logEntry = framePackage->_comment;
        timeStampLogMessage(logEntry);
        AppCore::writeToApplicationLogLine(logEntry);
    }
    else
    {
        std::string logEntry;
        if (framePackage->_console)
            logEntry = "Request from console: " + framePackage->_request;
        else
            logEntry = "Request: " + framePackage->_request;

        AppCore::writeToApplicationLogLine(logEntry);

        logEntry.clear();
        timeStampLogMessage(logEntry);
        AppCore::writeToApplicationLogLine(logEntry);

        if (!framePackage->_synonym.empty())
            AppCore::writeToApplicationLogLine("Synonym: " + framePackage->_synonym);

        AppCore::writeToApplicationLogLine("Responses");
        for (const auto& response : framePackage->_responses)
            AppCore::writeToApplicationLogLine("   " + response);
    }

    AppCore::writeToApplicationLogLine("Frame Package End");
}
