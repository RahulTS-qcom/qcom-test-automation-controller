// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

// Author: biswroy

#include "TACPIC32CXDriveThread.h"
#include "AlpacaDevice.h"
#include "AppCore.h"
#include "StringUtilities.h"
#include "TACPIC32CXCommand.h"
#include "TACCommandHashes.h"

#include <chrono>
#include <thread>

bool TACPIC32CXDriveThread::_initialized{false};

static const std::string kTACPIC32CXDriveTrainName{"TAC PIC32CX Drive Train"};

TACPIC32CXDriveThread::TACPIC32CXDriveThread(uint32_t hash) : TACDriveThread(hash)
{
    _driveTrainName = kTACPIC32CXDriveTrainName;
    setProtocolInterface(&_tacProtocol);
    _tacProtocol.setTACDriveTrain(this);

    AlpacaDevice alpacaDevice = _AlpacaDevice::findAlpacaDevice(hash);
    if (alpacaDevice && alpacaDevice->active())
    {
        _hardwareType = ePIC32CXAuto;
        _firmwareMajor = 1;
        _firmwareMinor = 1;
        _firmwareRevision = 1;
        _description = "PIC32CX (Automotive) Board";
        _uuid = "PIC32CX - No UUID";

        _portName = alpacaDevice->portName();
        _serialNumber = alpacaDevice->serialNumber();
        _description = alpacaDevice->description();
    }
}

TACPIC32CXDriveThread::~TACPIC32CXDriveThread()
{
    if (_serialPort)
    {
        _serialPort->close();
        delete _serialPort;
        _serialPort = nullptr;
    }
}

bool TACPIC32CXDriveThread::openSerialDevice()
{
    bool result{false};

    auto ports = SerialPortInfo::availablePorts();
    for (const auto& portInfo : ports)
    {
        if (portInfo.portName() == _portName || portInfo.serialNumber() == _portName)
        {
            _tacPortInfo = portInfo;
            break;
        }
    }

    if (!_tacPortInfo.portName().empty())
    {
        _serialPort = new SerialPort(_tacPortInfo);

        SerialPortSettings settings = _serialPort->getSerialPortSettings();
        settings._baudRate = 115200;
        settings._timeout = 500;
        _serialPort->setSerialPortSettings(settings);

        if (_serialPort->open())
        {
            setSerialNumber(_tacPortInfo.serialNumber());
            setPortName(_tacPortInfo.portName());

            if (onSerialNumUpdate) onSerialNumUpdate(_serialNumber);

            AppCore::writeToApplicationLogLine("Device " + _tacPortInfo.serialNumber() + " opened");
            AppCore::writeToApplicationLogLine("Com port " + _tacPortInfo.portName() + "\n");

            {
                TACPIC32CXCommand tacCommand(this, this);
                tacCommand.clearBuffer();
            }

            result = true;
        }
        else
        {
            AppCore::writeToApplicationLogLine("Unable to open TAC Port. " + serialPortError());
            if (onErrorOnOpen) onErrorOnOpen("PIC32CX Device Open Failed. Check the Application Log");

            delete _serialPort;
            _serialPort = nullptr;
        }
    }

    return result;
}

void TACPIC32CXDriveThread::sendCommand(const std::string& command, bool console,
    ReceiveInterface* receiveInterface, bool shouldStore)
{
    Arguments args;
    std::string decodedCommand = decodeCommand(command, args);
    send(decodedCommand, args, console, receiveInterface, shouldStore);
}

void TACPIC32CXDriveThread::endTransaction(ReceiveInterface* receiveInterface)
{
    _tacProtocol.endTransaction(receiveInterface);
}

void TACPIC32CXDriveThread::setPinState(uint16_t pin, bool state)
{
    {
        TACPIC32CXCommand tacCommand(this, this);
        tacCommand.setPinState(pin, state);
    }
    waitForCompletion();
}

void TACPIC32CXDriveThread::sendCommandSequence(CommandEntries& commandEntries)
{
    for (const auto& commandEntry : commandEntries)
    {
        {
            TACPIC32CXCommand tacCommand(this, this);

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
        }

        if (onProgress) onProgress(kProgressActive, eInfoNotification);
    }
}

int TACPIC32CXDriveThread::getResetCount()
{
    waitForCompletion();
    return _resetCount;
}

void TACPIC32CXDriveThread::clearResetCount()
{
    waitForCompletion();
}

void TACPIC32CXDriveThread::i2CReadRegister(uint32_t /*addr*/, uint32_t /*reg*/) {}
void TACPIC32CXDriveThread::i2CWriteRegister(uint32_t /*addr*/, uint32_t /*reg*/, uint32_t /*data*/) {}

void TACPIC32CXDriveThread::setName(const std::string& newName)
{
    if (newName.length() <= 32 && newName != _name && isAlphaNumeric(newName))
    {
        waitForCompletion();
    }
}

uint32_t TACPIC32CXDriveThread::send(const std::string& sendMe, const Arguments& arguments,
    bool console, ReceiveInterface* recieveInterface, bool store)
{
    return _tacProtocol.sendCommand(sendMe, arguments, console, recieveInterface, store);
}

bool TACPIC32CXDriveThread::ready()
{
    return _tacProtocol.queueSize() == 0;
}

std::string TACPIC32CXDriveThread::serialPortError()
{
    if (_serialPort != nullptr)
        return _serialPort->errorString();
    return "";
}

void TACPIC32CXDriveThread::on_readyRead()
{
    _readyRead = true;
}

bool TACPIC32CXDriveThread::readSerialData()
{
    bool result{false};

    // Read with a short blocking timeout — replaces the unreliable
    // _readyRead flag + waitForReadyRead approach.
    _serialBuffer.clear();

    std::string chunk = _serialPort->readAll(10);
    if (!chunk.empty())
    {
        _serialBuffer += chunk;
        // Drain any additional data that arrived
        while (true)
        {
            chunk = _serialPort->readAll(1);
            if (!chunk.empty())
                _serialBuffer += chunk;
            else
                break;
        }
    }

    if (!_serialBuffer.empty())
    {
        _protocolInterface->handleRecievedData(_serialBuffer);
        result = true;
    }

    return result;
}

void TACPIC32CXDriveThread::receive(FramePackage& framePackage)
{
    if (framePackage->_endTransaction == true)
    {
        clearWaitForCompletion();
        if (onTransactionEnded) onTransactionEnded();

        if (_protocolInterface->queueSize() == 0)
            if (onProgress) onProgress(kProgressMax, eInfoNotification);
    }
    else
    {
        if (framePackage->_valid == true)
        {
            switch (framePackage->_requestHash)
            {
            case kPIC32CXVersionCommandHash: handleVersionResponse(framePackage); break;
            case kPIC32CXSetPinCommandHash: handleSetPin(framePackage); break;
            case kPIC32CXClearBufferHash: handleClearBuffer(framePackage); break;
            default: break;
            }
        }
        else
        {
            clearWaitForCompletion();

            switch (framePackage->_requestHash)
            {
            case kPIC32CXVersionCommandHash:
            {
                TACPIC32CXCommand tacCommand(this, this);
                tacCommand.platformID();
                break;
            }
            default: break;
            }
        }
    }

    log(framePackage);
    _protocolInterface->clearPendingFrame();
}

void TACPIC32CXDriveThread::run()
{
    AppCore::getAppCore()->setRunLogging(AppCore::getAppCore()->getPreferences()->loggingActive());

    if (openSerialDevice() == true)
    {
        if (onDeviceOpen) onDeviceOpen();

        _serialPort->onReadyRead = [this]() { on_readyRead(); };

        startRunning();

        if (onDeviceStatusChange) onDeviceStatusChange("Starting");

        _readyRead = false;
        _connected = false;

        {
            TACPIC32CXCommand tacCommand(this, this);
            tacCommand.platformID();
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
                        // Clear serial buffer before version command
                        if (framePackage->_requestHash == kPIC32CXVersionCommandHash)
                        {
                            if (_serialPort->clear())
                                AppCore::writeToApplicationLogLine("Buffer cleared before identifying PIC32CX board");
                        }

                        int64_t bytesWritten = _serialPort->write(framePackage->_codedRequest);
                        if (bytesWritten == -1)
                        {
                            AppCore::writeToApplicationLogLine("TACPIC32CXDriveThread::run()::bytesWritten == -1");
                            std::string errorString = _serialPort->errorString();
                            if (!errorString.empty())
                                AppCore::writeToApplicationLogLine("Error on write " + errorString);
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
        _connected = false;
    }
    else
    {
        if (onErrorOnOpen) onErrorOnOpen("PIC32CX device open failed");
    }

    shutdownLogging();
}

void TACPIC32CXDriveThread::handleSetPin(FramePackage& framePackage)
{
    FrameArgument stateArg = framePackage->getArgument(0);
    FrameArgument pinArg = framePackage->getArgument(1);

    bool state = std::holds_alternative<bool>(stateArg) ? std::get<bool>(stateArg) : false;
    uint32_t pin = std::holds_alternative<uint32_t>(pinArg) ? std::get<uint32_t>(pinArg) : 0;

    framePackage->_synonym = "Pin " + std::to_string(pin) + " " + (state ? "on" : "off");

    if (onPinStateChanged) onPinStateChanged(static_cast<uint64_t>(pin), state);
}

void TACPIC32CXDriveThread::handleVersionResponse(FramePackage& framePackage)
{
    std::lock_guard<std::recursive_mutex> lk(_stateMutex);
    if (!framePackage->_responses.empty())
    {
        // PIC32CX returns comma-separated: name,firmware,?,mac,platformID,serial,mcn
        std::string response = framePackage->_responses.at(0);

        // Split by comma
        std::vector<std::string> parts;
        size_t start = 0;
        size_t end;
        while ((end = response.find(',', start)) != std::string::npos)
        {
            parts.push_back(response.substr(start, end - start));
            start = end + 1;
        }
        parts.push_back(response.substr(start));

        if (parts.size() < 7)
        {
            AppCore::writeToApplicationLogLine("Incomplete board response: " + response);
        }
        else
        {
            _name = parts[0];
            AppCore::writeToApplicationLogLine("PIC32CX board name: " + _name);

            _firmwareString = parts[1];
            AppCore::writeToApplicationLogLine("PIC32CX board firmware version: " + _firmwareString);

            _macAddress = parts[3];
            AppCore::writeToApplicationLogLine("PIC32CX board MAC: " + _macAddress);

            try
            {
                int boardID = std::stoi(parts[4]);
                _platformID = static_cast<PlatformID>(boardID);
            }
            catch (...)
            {
                _platformID = ALPACA_PIC32CX_ID;
            }
            AppCore::writeToApplicationLogLine("Identified platform id: " + std::to_string(static_cast<int>(_platformID)));

            _serialNumber = parts[5];
            AppCore::writeToApplicationLogLine("PIC32CX board Serial Number: " + _serialNumber);

            _mcnNumber = parts[6];
            AppCore::writeToApplicationLogLine("PIC32CX board MCN: " + _mcnNumber);
        }
    }

    if (onPlatformIDUpdate) onPlatformIDUpdate(static_cast<int>(_platformID));
    setupConnected();
}

void TACPIC32CXDriveThread::handleClearBuffer(FramePackage& framePackage)
{
    if (!framePackage->_responses.empty())
    {
        std::string joined;
        for (size_t i = 0; i < framePackage->_responses.size(); i++)
        {
            if (i > 0) joined += ",";
            joined += framePackage->_responses[i];
        }
        AppCore::writeToApplicationLogLine("Buffer cleared. Board response: '" + joined + "'");
    }
}

void TACPIC32CXDriveThread::setupConnected()
{
    if (_connected == false)
    {
        if (onHardwareVersionUpdate) onHardwareVersionUpdate(hardwareVersionString());

        _connected = true;

        AppCore::writeToApplicationLogLine("");

        TACPIC32CXCommand tacCommand(this, this);

        AppCore::writeToApplicationLogLine("tacCommand.platformID()");
        tacCommand.platformID();

        if (onDeviceConnected) onDeviceConnected();
    }
}

void TACPIC32CXDriveThread::setupDiscovery()
{
    if (onHardwareVersionUpdate) onHardwareVersionUpdate(hardwareVersionString());

    TACPIC32CXCommand tacCommand(this, this);
    tacCommand.platformID();
}

void TACPIC32CXDriveThread::log(FramePackage& framePackage)
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
