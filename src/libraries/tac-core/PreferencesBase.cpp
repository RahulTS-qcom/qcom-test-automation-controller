// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

// Author: msimpson

#include "PreferencesBase.h"
#include "ConsoleApplicationEnhancements.h"

// Keys for preferences (not persisted in non-Qt build — in-memory only for now)
static const std::string kLoggingEnabled{"logEnabled"};
static const std::string kAppLogLocation{"appLogLocation"};
static const std::string kRunLogLocation{"runLogLocation"};
static const std::string kPlatformConfigLocation_key{"platformConfigLocation"};

PreferencesBase::PreferencesBase() {}

void PreferencesBase::setAppName(const std::string& appName, const std::string& appVersion)
{
    _appName = appName;
    _appVersion = appVersion;

    // In non-Qt build, use defaults (no QSettings persistence)
    _loggingActive = defaultLoggingState();
    _appLoggingPath = defaultAppLogPath();
    _runLoggingPath = defaultRunLogPath();
    _platformConfigLocation = defaultPlatformConfigLocation();
}

std::string PreferencesBase::appName()
{
    return _appName;
}

std::string PreferencesBase::appVersion()
{
    return _appVersion;
}

bool PreferencesBase::defaultLoggingState()
{
    return false;
}

bool PreferencesBase::loggingActive()
{
    return _loggingActive;
}

void PreferencesBase::setLoggingActive(bool loggingActive)
{
    _loggingActive = loggingActive;
}

void PreferencesBase::saveLoggingActive(bool loggingActive)
{
    _loggingActive = loggingActive;
    // TODO: persist to JSON config file
}

std::string PreferencesBase::defaultAppLogPath()
{
    return defaultGlobalLoggingPath();
}

std::string PreferencesBase::appLogPath()
{
    return _appLoggingPath;
}

void PreferencesBase::setAppLogPath(const std::string& loggingPath)
{
    _appLoggingPath = loggingPath;
}

void PreferencesBase::saveAppLogPath(const std::string& loggingPath)
{
    setAppLogPath(loggingPath);
    // TODO: persist to JSON config file
}

std::string PreferencesBase::defaultRunLogPath()
{
    return defaultLoggingPath(_appName);
}

std::string PreferencesBase::runLogPath()
{
    return _runLoggingPath;
}

void PreferencesBase::setRunLogPath(const std::string& loggingPath)
{
    _runLoggingPath = loggingPath;
}

void PreferencesBase::saveRunLogPath(const std::string& loggingPath)
{
    setRunLogPath(loggingPath);
    // TODO: persist to JSON config file
}

std::string PreferencesBase::defaultPlatformConfigLocation()
{
    std::string result;

#ifdef DEBUG
    result = "C:\\github\\open-source\\qcom-test-automation-controller\\configurations\\";
#else
    result = tacConfigRoot();
#endif

    return result;
}

std::string PreferencesBase::platformConfigLocation()
{
    return _platformConfigLocation;
}

void PreferencesBase::setPlatformConfigLocation(const std::string& /*platformConfigLocation*/)
{
}

void PreferencesBase::savePlatformConfigLocation(const std::string& /*saveLocation*/)
{
}
