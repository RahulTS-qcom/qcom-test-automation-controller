// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

// Author: msimpson, biswroy

#include "AppCore.h"
#include "AlpacaDefines.h"
#include "ConsoleApplicationEnhancements.h"

#include <chrono>
#include <ctime>
#include <filesystem>

AppCore* AppCore::_appCore{nullptr};

class AppCoreDestructor
{
public:
    AppCoreDestructor() {}
    ~AppCoreDestructor()
    {
        if (AppCore::_appCore != nullptr)
        {
            delete AppCore::_appCore;
            AppCore::_appCore = nullptr;
        }
    }
} gAppCoreDestructor;

AppCore::AppCore()
{
    initializeStringProof();

    if (AppCore::_appCore == nullptr)
        AppCore::_appCore = this;

#ifdef _WIN32
    _internalBuild = std::filesystem::exists("C:\\Program Files (x86)\\Qualcomm\\QTAC\\ib.conf");
#else
    _internalBuild = std::filesystem::exists("/opt/qcom/QTAC/bin/ib.conf");
#endif
}

AppCore::~AppCore()
{
    if (_runThreadedLog)
    {
        _runThreadedLog->close();
        _runThreadedLog.reset();
    }

    if (_appThreadedLog)
    {
        _appThreadedLog->close();
        _appThreadedLog.reset();
    }
}

AppCore* AppCore::getAppCore()
{
    if (AppCore::_appCore == nullptr)
    {
        AppCore::_appCore = new AppCore;
    }
    return AppCore::_appCore;
}

void AppCore::setPreferences(PreferencesBase* preferences)
{
    _preferences = preferences;
    _appName = _preferences->appName();
    _appVersion = _preferences->appVersion();
    setAppLogging(_preferences->loggingActive());
}

uint32_t AppCore::daysSinceInstall()
{
    // Simplified: always return 0 (no expiration check for open-source)
    return 0;
}

void AppCore::setAppLogging(bool loggingState)
{
    if (_appThreadedLog)
    {
        _appThreadedLog->close();
        _appThreadedLog.reset();
    }

    if (loggingState)
    {
        std::string logPath;

        if (_preferences != nullptr)
        {
            logPath = _preferences->appLogPath() + "/" + _ThreadedLog::createLogName(_appName + "_");
        }
        else
        {
            logPath = defaultGlobalLoggingPath() + "/" + _ThreadedLog::createLogName(_appName + "_");
        }

        _appThreadedLog = _ThreadedLog::createThreadedLog();
        _appThreadedLog->open(logPath);

        writeToAppLog(_appName + " Application log started\n");
        writeToAppLog("QTAC Version: " + std::string(ALPACA_VERSION) + "\n");
        writeToAppLog(_appName + " Version: " + _appVersion + "\n");
    }
}

bool AppCore::appLoggingActive()
{
    return _appThreadedLog != nullptr;
}

void AppCore::setRunLogging(bool loggingState)
{
    if (_runThreadedLog)
    {
        _runThreadedLog->close();
        _runThreadedLog.reset();
    }

    if (loggingState)
    {
        std::string logPath;

        if (_preferences != nullptr)
        {
            logPath = _preferences->runLogPath() + "/" + _ThreadedLog::createLogName(_appName + "_");
        }
        else
        {
            logPath = defaultLoggingPath(_appName) + "/" + _ThreadedLog::createLogName(_appName + "_");
        }

        _runThreadedLog = _ThreadedLog::createThreadedLog();
        _runThreadedLog->open(logPath);
        _runThreadedLog->addLogEntry("Starting Log\n");
    }
}

bool AppCore::runLoggingActive()
{
    return _runThreadedLog != nullptr;
}

std::string AppCore::loggingPath()
{
    std::string result{defaultLoggingPath(_appName)};

    if (_preferences != nullptr)
        result = _preferences->runLogPath();

    return result;
}

bool AppCore::checkLicense(const std::string& /*productID*/, const std::string& /*featureID*/)
{
    return true;
}

bool AppCore::isLicenseManagerValid()
{
    return true;
}

void AppCore::postStartEvent() {}
void AppCore::postAutomationEvent() {}

void AppCore::postMetric(const std::string& /*metricID*/, double /*metric*/) {}

void AppCore::writeToAppLog(const std::string& writeMe)
{
    if (_appThreadedLog)
        _appThreadedLog->addLogEntry(writeMe);
}

void AppCore::writeToApplicationLog(const std::string& writeMe)
{
    AppCore* appCore = AppCore::getAppCore();
    if (appCore != nullptr)
        appCore->writeToAppLog(writeMe);
}

void AppCore::writeToApplicationLogLine(const std::string& writeMe)
{
    AppCore::writeToApplicationLog(writeMe + "\n");
}

void AppCore::writeToRunLog(const std::string& writeMe)
{
    if (_runThreadedLog)
        _runThreadedLog->addLogEntry(writeMe);
}

void AppCore::writeToRuntimeLog(const std::string& writeMe)
{
    AppCore* appCore = AppCore::getAppCore();
    if (appCore != nullptr)
        appCore->writeToRunLog(writeMe);
}
