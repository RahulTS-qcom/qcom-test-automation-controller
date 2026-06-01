// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

// Author: msimpson

#include "TACPreferences.h"
#include "ConsoleApplicationEnhancements.h"

void TACPreferences::setAppName(const std::string& appName, const std::string& appVersion)
{
    PreferencesBase::setAppName(appName, appVersion);

    // In non-Qt build, use defaults (no QSettings persistence)
    _powerDelay = defaultPowerDelay();
    _buttonAssertTime = defaultButtonAssertTime();
    _autoShutdown = defaultAutoShutdown();
    _autoShutdownDelay = defaultAutoShutdownDelay();
    _openLast = defaultOpenLastStart();
    _tacEditorConfigLocation = defaultTACEditorConfigLocation();
}

int TACPreferences::defaultPowerDelay() { return 800; }
int TACPreferences::powerDelay() { return _powerDelay; }
void TACPreferences::setPowerDelay(int powerDelay) { _powerDelay = powerDelay; }
void TACPreferences::savePowerDelay(int powerDelay) { _powerDelay = powerDelay; }

int TACPreferences::defaultButtonAssertTime() { return 8; }
int TACPreferences::buttonAssertTime() { return _buttonAssertTime; }
void TACPreferences::setButtonAssertTime(int buttonAssertTime) { _buttonAssertTime = buttonAssertTime; }
void TACPreferences::saveButtonAssertTime(int buttonAssertTime) { _buttonAssertTime = buttonAssertTime; }

bool TACPreferences::defaultAutoShutdown() { return false; }
bool TACPreferences::autoShutdownActive() { return _autoShutdown; }
void TACPreferences::setAutoShutdownActive(bool shutdownActive) { _autoShutdown = shutdownActive; }
void TACPreferences::saveAutoShutdownActive(bool shutdownActive) { _autoShutdown = shutdownActive; }

double TACPreferences::defaultAutoShutdownDelay() { return 24.0; }
double TACPreferences::autoShutdownDelay() { return _autoShutdownDelay; }

int64_t TACPreferences::autoShutdownDelayInMSecs()
{
    double result = autoShutdownDelay();
    result *= 1000.0 * 60.0 * 60.0;
    return static_cast<int64_t>(result);
}

void TACPreferences::setAutoShutdownDelay(double delayInHours) { _autoShutdownDelay = delayInHours; }
void TACPreferences::saveAutoShutdownDelay(double delayInHours) { _autoShutdownDelay = delayInHours; }

bool TACPreferences::defaultOpenLastStart() { return false; }
bool TACPreferences::openLastStart() { return _openLast; }
void TACPreferences::setOpenLastStart(bool openLastOnStart) { _openLast = openLastOnStart; }
void TACPreferences::saveOpenLastStart(bool openLastOnStart) { _openLast = openLastOnStart; }

std::string TACPreferences::lastDevice()
{
    // TODO: persist to JSON config file
    return {};
}

void TACPreferences::saveLastDevice(const std::string& /*lastDevice*/)
{
    // TODO: persist to JSON config file
}

std::string TACPreferences::defaultTACEditorConfigLocation()
{
    return documentsDataPath("TAC Configurations");
}

std::string TACPreferences::tacEditorConfigLocation()
{
    return _tacEditorConfigLocation;
}

void TACPreferences::setTACEditorConfigLocation(const std::string& tacConfigLocation)
{
    _tacEditorConfigLocation = tacConfigLocation;
}

void TACPreferences::saveTACEditorConfigLocation(const std::string& saveLocation)
{
    setTACEditorConfigLocation(saveLocation);
    // TODO: persist to JSON config file
}
