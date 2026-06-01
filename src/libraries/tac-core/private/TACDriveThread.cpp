// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

/*
	Author: Michael Simpson (msimpson@qti.qualcomm.com)
			Biswajit Roy (biswroy@qti.qualcomm.com)
*/

#include "TACDriveThread.h"

#include "AlpacaDevice.h"
#include "AppCore.h"
#include "StringUtilities.h"
#include "TickCount.h"
#include "TACCommands.h"
#include "TACCommandHashes.h"
#include "TACLiteDriveThread.h"
#include "TACPSOCDriveThread.h"
#include "TACPIC32CXDriveThread.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <sstream>
#include <thread>

TACDriveThread::TACDriveThread(HashType hash) : _hash(hash)
{
}

TACDriveThread::~TACDriveThread()
{
}

std::unique_ptr<TACDriveThread> TACDriveThread::openPort(const std::string& portName)
{
    std::unique_ptr<TACDriveThread> result;

    if (_AlpacaDevice::updateAlpacaDevices() > 0)
    {
        AlpacaDevice alpacaDevice = _AlpacaDevice::findAlpacaDevice(portName);

        if (alpacaDevice->active())
        {
            switch (alpacaDevice->debugBoardType())
            {
            case ePSOC:
                result = std::make_unique<TACPSOCDriveThread>(static_cast<uint32_t>(alpacaDevice->hash()));
                break;
            case eFTDI:
                result = std::make_unique<TACLiteDriveThread>(static_cast<uint32_t>(alpacaDevice->hash()));
                break;
            case ePIC32CXAuto:
                result = std::make_unique<TACPIC32CXDriveThread>(static_cast<uint32_t>(alpacaDevice->hash()));
                break;
            default:
                break;
            }
        }
    }

    return result;
}

HashType TACDriveThread::hash() { return _hash; }

void TACDriveThread::waitForCompletion()
{
    // Replaces QThread::msleep + QCoreApplication::processEvents
    // Uses condition_variable — works in any context including Python scripts
    std::unique_lock<std::mutex> lock(_completionMutex);
    bool timedOut = !_completionCV.wait_for(lock, std::chrono::seconds(5),
        [this]{ return !_waitForCompletion.load(); });

    if (timedOut)
    {
        AppCore::writeToApplicationLogLine("Wait for completion timed out.");
        _waitForCompletion.store(false);
    }
}

void TACDriveThread::setWaitForCompletion()
{
    _waitForCompletion.store(true);
}

void TACDriveThread::clearWaitForCompletion()
{
    _waitForCompletion.store(false);
    _completionCV.notify_one();
}

bool TACDriveThread::waitForCompletionStatus()
{
    return _waitForCompletion.load();
}

bool TACDriveThread::oldFirmware() { return _oldFirmware; }

static std::string strToLower(const std::string& s)
{
    std::string r;
    for (auto c : s) r += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

static std::string strTrimmed(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    return s.substr(a, b - a + 1);
}

static bool strEndsWith(const std::string& s, const std::string& suffix)
{
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static bool strStartsWith(const std::string& s, const std::string& prefix)
{
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

std::string TACDriveThread::decodeCommand(const std::string& command, Arguments& args)
{
    std::string result = strTrimmed(strToLower(command));

    if (strEndsWith(result, " on"))
    {
        args.push_back(true);
        result = result.substr(0, result.size() - 3);
    }
    else if (strEndsWith(result, " off"))
    {
        args.push_back(false);
        result = result.substr(0, result.size() - 4);
    }
    else if (strEndsWith(result, " 1"))
    {
        args.push_back(true);
        result = result.substr(0, result.size() - 2);
    }
    else if (strEndsWith(result, " 0"))
    {
        args.push_back(false);
        result = result.substr(0, result.size() - 2);
    }

    if (strStartsWith(result, strToLower(kSetNameCommand)) || strStartsWith(result, "setname"))
    {
        // Remove the command prefix, rest is the name
        size_t pos = result.find(' ');
        std::string name = (pos != std::string::npos) ? strTrimmed(result.substr(pos)) : "";
        args.push_back(name);
        result = kSetNameCommand;
    }
    else if (strStartsWith(result, strToLower(kSetButtonAssertTime)) || strStartsWith(result, kSetButtonAssertTimeAlias))
    {
        size_t pos = result.rfind(' ');
        if (pos != std::string::npos)
        {
            try { args.push_back(static_cast<uint32_t>(std::stoul(result.substr(pos + 1)))); } catch (...) {}
        }
        result = kSetButtonAssertTime;
    }
    else if (strStartsWith(result, strToLower(kSetPowerKeyDelay)) || strStartsWith(result, kSetPowerKeyDelayAlias))
    {
        size_t pos = result.rfind(' ');
        if (pos != std::string::npos)
        {
            try { args.push_back(static_cast<uint32_t>(std::stoul(result.substr(pos + 1)))); } catch (...) {}
        }
        result = kSetPowerKeyDelay;
    }
    else if (strStartsWith(result, strToLower(kSetPinCommand)))
    {
        size_t pos = result.rfind(' ');
        if (pos != std::string::npos)
        {
            try { args.push_back(static_cast<uint32_t>(std::stoul(result.substr(pos + 1)))); } catch (...) {}
        }
        result = kSetPinCommand;
    }

    HashType hash = CommandStringToHash(result);
    if (hash != 0)
    {
        oldCommandEntry commandEntry = CommandHashToCommandEntry(hash);
        result = commandEntry._longCommand;
    }
    else
        result.clear();

    return result;
}

bool TACDriveThread::checkLocalStore(FramePackage& framePackage)
{
    std::lock_guard<std::recursive_mutex> lk(_stateMutex);
    bool result{false};

    switch (framePackage->_requestHash)
    {
    case kVersionCommandHash:
        if (!_versionString.empty())
        {
            framePackage->_responses.push_back(_versionString);
            result = true;
        }
        break;

    case kGetNameCommandHash:
        if (!_name.empty())
        {
            framePackage->_responses.push_back(_name);
            result = true;
        }
        break;

    case kGetUUIDCommandHash:
        if (!_uuid.empty())
        {
            framePackage->_responses.push_back(_uuid);
            result = true;
        }
        break;

    case kGetPlatformIDCommandHash:
        if (_platformID != MICRO_EPM_BOARD_ID_UNKNOWN)
        {
            std::string response = PlatformContainer::toString(_platformID) +
                                   "(" + std::to_string(static_cast<int>(_platformID)) + ")";
            framePackage->_responses.push_back(response);
            result = true;
        }
        break;

    case kPIC32CXVersionCommandHash:
        if (!_versionString.empty())
        {
            framePackage->_responses.push_back(_versionString);
            result = true;
        }
        break;
    }

    return result;
}

DebugBoardType TACDriveThread::debugBoardType()       { std::lock_guard<std::recursive_mutex> lk(_stateMutex); return _hardwareType; }
std::string    TACDriveThread::debugBoardTypeString() { std::lock_guard<std::recursive_mutex> lk(_stateMutex); return debugBoardTypeToString(_hardwareType); }
PlatformID     TACDriveThread::platformID()           { std::lock_guard<std::recursive_mutex> lk(_stateMutex); return _platformID; }
std::string    TACDriveThread::hardwareVersionString(){ std::lock_guard<std::recursive_mutex> lk(_stateMutex); return PlatformContainer::toString(_platformID); }
std::string    TACDriveThread::firmwareVersion()      { std::lock_guard<std::recursive_mutex> lk(_stateMutex); return _firmwareString; }
uint32_t       TACDriveThread::majorVersion()         { std::lock_guard<std::recursive_mutex> lk(_stateMutex); return _firmwareMajor; }
uint32_t       TACDriveThread::chipVersion()          { std::lock_guard<std::recursive_mutex> lk(_stateMutex); return _firmwareChip; }
uint32_t       TACDriveThread::minorVersion()         { std::lock_guard<std::recursive_mutex> lk(_stateMutex); return _firmwareMinor; }
uint32_t       TACDriveThread::revisionVersion()      { std::lock_guard<std::recursive_mutex> lk(_stateMutex); return _firmwareRevision; }

std::string TACDriveThread::name() const              { std::lock_guard<std::recursive_mutex> lk(_stateMutex); return _name; }
void TACDriveThread::setName(const std::string& n)    { std::lock_guard<std::recursive_mutex> lk(_stateMutex); _name = n; }

std::string TACDriveThread::portName() const          { std::lock_guard<std::recursive_mutex> lk(_stateMutex); return _portName; }
void TACDriveThread::setPortName(const std::string& p){ std::lock_guard<std::recursive_mutex> lk(_stateMutex); _portName = p; }

std::string TACDriveThread::description() const       { std::lock_guard<std::recursive_mutex> lk(_stateMutex); return _description; }
void TACDriveThread::setDescription(const std::string& d){ std::lock_guard<std::recursive_mutex> lk(_stateMutex); _description = d; }

std::string TACDriveThread::serialNumber() const      { std::lock_guard<std::recursive_mutex> lk(_stateMutex); return _serialNumber; }
void TACDriveThread::setSerialNumber(const std::string& s){ std::lock_guard<std::recursive_mutex> lk(_stateMutex); _serialNumber = s; }

std::string TACDriveThread::uuid()                    { std::lock_guard<std::recursive_mutex> lk(_stateMutex); return _uuid; }
std::string TACDriveThread::macAddress()              { std::lock_guard<std::recursive_mutex> lk(_stateMutex); return _macAddress; }

void TACDriveThread::setupLogging(bool loggingState)
{
    AppCore::getAppCore()->setRunLogging(loggingState);
}

void TACDriveThread::shutdownLogging()
{
    AppCore::getAppCore()->setRunLogging(false);
}

bool TACDriveThread::resetLogging()
{
    bool result{false};
    AppCore* appCore = AppCore::getAppCore();
    if (appCore != nullptr)
    {
        result = appCore->runLoggingActive();
        if (result)
        {
            appCore->setRunLogging(false);
            appCore->setRunLogging(true);
        }
    }
    return result;
}

void TACDriveThread::log(FramePackage& framePackage)
{
    AppCore::writeToApplicationLogLine("");
    AppCore::writeToApplicationLogLine("Frame Package Start");
    AppCore::writeToApplicationLogLine(framePackage->_valid ? "Frame Valid" : "Frame Invalid");

    if (framePackage->_delayInMilliSeconds > 0)
    {
        std::string logEntry = " Delay " + std::to_string(framePackage->_delayInMilliSeconds) + " in msecs";
        timeStampLogMessage(logEntry);
        AppCore::writeToApplicationLogLine(logEntry);
    }
    else if (!framePackage->_comment.empty())
    {
        AppCore::writeToApplicationLogLine(framePackage->_comment);
    }
    else
    {
        std::string logEntry = framePackage->_console
            ? "Request from console: " + framePackage->_request
            : "Request: " + framePackage->_request;
        AppCore::writeToApplicationLogLine(logEntry);

        if (!framePackage->_synonym.empty())
            AppCore::writeToApplicationLogLine("Synonym: " + framePackage->_synonym);

        AppCore::writeToApplicationLogLine("Responses");
        for (const auto& r : framePackage->_responses)
            AppCore::writeToApplicationLogLine("   " + r);
    }

    AppCore::writeToApplicationLogLine("Frame Package End");
    AppCore::writeToApplicationLogLine("");
}

void TACDriveThread::timeStampLogMessage(std::string& timeStampMe)
{
    static uint64_t lastTimeStamp{0};
    if (lastTimeStamp == 0)
        lastTimeStamp = tickCount();

    uint64_t current = tickCount();
    timeStampMe += " Time: " + std::to_string(current) +
                   " elapsed: " + std::to_string(current - lastTimeStamp) + " (ms)";
    lastTimeStamp = current;
}

void TACDriveThread::setThreadDelay(uint32_t delay)
{
    _delay = delay;
}
