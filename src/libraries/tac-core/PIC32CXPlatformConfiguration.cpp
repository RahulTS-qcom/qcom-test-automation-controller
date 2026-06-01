// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

// Author: Biswajit Roy <biswroy@qti.qualcomm.com>

#include "AlpacaScript.h"
#include "PIC32CXPlatformConfiguration.h"
#include "PlatformConfigurationException.h"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <string>

using json = nlohmann::json;

static const char* kPlatformEntries = "pins";
static const char* kRunPriority     = "run_priority";
static const char* kPinNumber       = "pin_number";
static const char* kEnabled         = "enabled";
static const char* kInverted        = "inverted";
static const char* kName            = "name";
static const char* kToolTip         = "help_hint";
static const char* kCommand         = "command";
static const char* kCommmandGroup   = "command_group";
static const char* kTabName         = "tab_name";

const bool kDefaultInitialPinValue{false};
const bool kDefaultPinInvertedState{false};
const CommandGroups kDefaultCommandGroup{eUnknownCommandGroup};

_PIC32CXPlatformConfiguration::_PIC32CXPlatformConfiguration()
{
    _platform    = ePIC32CXAuto;
    _resetActive = true;

    _PIC32CXPlatformConfiguration::initialize();

    Tab generalTab, deviceInfoTab, terminalTab;
    generalTab._name = "General";       generalTab._moveable = false;  generalTab._configurable = true;  generalTab._ordinal = 0; generalTab._userTab = false;
    deviceInfoTab._name = "Device Info";deviceInfoTab._moveable = false;deviceInfoTab._configurable = false;deviceInfoTab._ordinal = 1;deviceInfoTab._userTab = false;
    terminalTab._name = "Terminal";     terminalTab._moveable = true;  terminalTab._configurable = false;terminalTab._ordinal = 2;terminalTab._userTab = false;

    _tabs.push_back(generalTab);
    _tabs.push_back(deviceInfoTab);
    _tabs.push_back(terminalTab);
}

_PIC32CXPlatformConfiguration::~_PIC32CXPlatformConfiguration()
{
}

Pins _PIC32CXPlatformConfiguration::getPins()
{
    Pins result;
    for (const auto& kv : _pinEntries)
    {
        const auto& pd = kv.second;
        if (!pd._enabled) continue;
        PinEntry pe;
        pe._pin      = pd._setPin;
        pe._enabled  = pd._enabled;
        pe._hash     = pd._hash;
        pe._cellX    = pd._cellX;
        pe._cellY    = pd._cellY;
        pe._commandGroup = pd._commandGroup;
        pe._pinCommand   = pd._pinCommand;
        pe._inverted     = pd._inverted;
        pe._pinLabel     = pd._pinLabel;
        pe._pinTooltip   = pd._pinTooltip;
        pe._tabName      = pd._tabName;
        result.push_back(pe);
    }
    std::sort(result.begin(), result.end(), [](const PinEntry& a, const PinEntry& b){
        if (a._tabName != b._tabName) return a._tabName < b._tabName;
        if (a._commandGroup != b._commandGroup) return a._commandGroup < b._commandGroup;
        return a._pin < b._pin;
    });
    return result;
}

PIC32CXPinList _PIC32CXPlatformConfiguration::getAllPins()
{
    PIC32CXPinList result;
    for (const auto& kv : _pinEntries) result.push_back(kv.second);
    std::sort(result.begin(), result.end(), [](const PIC32CXPinData& a, const PIC32CXPinData& b){ return a._setPin < b._setPin; });
    return result;
}

PIC32CXPinList _PIC32CXPlatformConfiguration::getActivePins()
{
    PIC32CXPinList result;
    for (const auto& kv : _pinEntries)
        if (kv.second._enabled) result.push_back(kv.second);
    std::sort(result.begin(), result.end(), [](const PIC32CXPinData& a, const PIC32CXPinData& b){ return a._setPin < b._setPin; });
    return result;
}

bool _PIC32CXPlatformConfiguration::getPinEnableState(PinID pinId) const
{
    HashType h = PIC32CXPinData::makePIC32CXHash(pinId);
    auto it = _pinEntries.find(h);
    return it != _pinEntries.end() ? it->second._enabled : true;
}

void _PIC32CXPlatformConfiguration::setPinEnableState(HashType hash, bool newState)
{
    auto it = _pinEntries.find(hash);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to update state of an invalid pin!");
    if (it->second._enabled != newState) { it->second._enabled = newState; _dirty = true; }
}

bool _PIC32CXPlatformConfiguration::getPinInvertedState(PinID pinId) const
{
    HashType h = PIC32CXPinData::makePIC32CXHash(pinId);
    auto it = _pinEntries.find(h);
    return it != _pinEntries.end() ? it->second._inverted : false;
}

void _PIC32CXPlatformConfiguration::setPinInvertedState(HashType hash, bool newState)
{
    auto it = _pinEntries.find(hash);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to update state of an invalid pin!");
    if (it->second._inverted != newState) { it->second._inverted = newState; _dirty = true; }
}

std::string _PIC32CXPlatformConfiguration::getPinLabel(PinID pinId) const
{
    HashType h = PIC32CXPinData::makePIC32CXHash(pinId);
    auto it = _pinEntries.find(h);
    return it != _pinEntries.end() ? it->second._pinLabel : "";
}

void _PIC32CXPlatformConfiguration::setPinLabel(HashType hash, const std::string& v)
{
    auto it = _pinEntries.find(hash);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to add label to an invalid pin!");
    it->second._pinLabel = v;
}

std::string _PIC32CXPlatformConfiguration::getPinTooltip(PinID pinId) const
{
    HashType h = PIC32CXPinData::makePIC32CXHash(pinId);
    auto it = _pinEntries.find(h);
    return it != _pinEntries.end() ? it->second._pinTooltip : "";
}

void _PIC32CXPlatformConfiguration::setPinTooltip(HashType hash, const std::string& v)
{
    auto it = _pinEntries.find(hash);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to add tooltip to an invalid pin!");
    it->second._pinTooltip = v;
}

std::string _PIC32CXPlatformConfiguration::getPinCommand(PinID pinId) const
{
    HashType h = PIC32CXPinData::makePIC32CXHash(pinId);
    auto it = _pinEntries.find(h);
    return it != _pinEntries.end() ? it->second._pinCommand : "";
}

void _PIC32CXPlatformConfiguration::setPinCommand(HashType hash, const std::string& v)
{
    auto it = _pinEntries.find(hash);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to add command to an invalid pin!");
    if (it->second._pinCommand != v) { it->second._pinCommand = v; _dirty = true; }
}

CommandGroups _PIC32CXPlatformConfiguration::getPinGroup(PinID pinId) const
{
    HashType h = PIC32CXPinData::makePIC32CXHash(pinId);
    auto it = _pinEntries.find(h);
    return it != _pinEntries.end() ? it->second._commandGroup : eUnknownCommandGroup;
}

void _PIC32CXPlatformConfiguration::setPinGroup(HashType hash, CommandGroups cg)
{
    auto it = _pinEntries.find(hash);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to add category to an invalid pin!");
    if (it->second._commandGroup != cg) { it->second._commandGroup = cg; _dirty = true; }
}

std::string _PIC32CXPlatformConfiguration::getTabName(PinID pinId) const
{
    HashType h = PIC32CXPinData::makePIC32CXHash(pinId);
    auto it = _pinEntries.find(h);
    return it != _pinEntries.end() ? it->second._tabName : "";
}

void _PIC32CXPlatformConfiguration::setTabName(HashType hash, const std::string& v)
{
    auto it = _pinEntries.find(hash);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to add tab name for an invalid pin!");
    if (it->second._tabName != v) { it->second._tabName = v; _dirty = true; }
}

void _PIC32CXPlatformConfiguration::getPinCellLocation(PinID pinId, int& x, int& y) const
{
    HashType h = PIC32CXPinData::makePIC32CXHash(pinId);
    auto it = _pinEntries.find(h);
    if (it != _pinEntries.end()) { x = it->second._cellX; y = it->second._cellY; }
    else { x = -1; y = -1; }
}

void _PIC32CXPlatformConfiguration::setPinCellLocation(PinID pinId, int x, int y)
{
    HashType h = PIC32CXPinData::makePIC32CXHash(pinId);
    auto it = _pinEntries.find(h);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to set cell location for an invalid pin!");
    if (it->second._cellX != x || it->second._cellY != y)
    {
        it->second._cellX = x; it->second._cellY = y;
        _dirty = true;
    }
}

PinID _PIC32CXPlatformConfiguration::bitFromSetPin(PinID setPin)
{
    return setPin; // identity mapping for PIC32CX
}

void _PIC32CXPlatformConfiguration::cascadeTabDelete(const std::string& deleteMe)
{
    _tabs.erase(std::remove_if(_tabs.begin(), _tabs.end(),
        [&](const Tab& t){ return t._name == deleteMe; }), _tabs.end());
    for (auto& kv : _pinEntries)
        if (kv.second._tabName == deleteMe)
            kv.second._tabName.clear();
}

void _PIC32CXPlatformConfiguration::cascadeTabRename(const std::string& oldName, const std::string& newName)
{
    for (auto& kv : _pinEntries)
        if (kv.second._tabName == oldName)
            kv.second._tabName = newName;
}

bool _PIC32CXPlatformConfiguration::read(const json& j)
{
    bool result = _PlatformConfiguration::read(j);

    if (j.contains(kPlatformEntries) && j[kPlatformEntries].is_array())
    {
        for (const auto& pe : j[kPlatformEntries])
        {
            PIC32CXPinData pd;
            if (pe.contains(kPinNumber))
            {
                auto& pn = pe[kPinNumber];
                int pinVal = 0;
                if (pn.is_number()) pinVal = pn.get<int>();
                else if (pn.is_string()) { try { pinVal = std::stoi(pn.get<std::string>()); } catch (...) {} }
                pd._setPin = static_cast<PinID>(pinVal);
                pd._hash   = PIC32CXPinData::makePIC32CXHash(pd._setPin);
            }
            if (pe.contains(kEnabled))      pd._enabled      = pe[kEnabled].get<bool>();
            if (pe.contains(kInverted))     pd._inverted     = pe[kInverted].get<bool>();
            if (pe.contains(kName))         pd._pinLabel     = pe[kName].is_string() ? pe[kName].get<std::string>() : "";
            if (pe.contains(kToolTip))      pd._pinTooltip   = pe[kToolTip].is_string() ? pe[kToolTip].get<std::string>() : "";
            if (pe.contains(kCommand))      pd._pinCommand   = pe[kCommand].is_string() ? pe[kCommand].get<std::string>() : "";
            if (pe.contains(kCommmandGroup))
            {
                auto& gv = pe[kCommmandGroup];
                int g = 0;
                if (gv.is_number()) g = gv.get<int>();
                else if (gv.is_string()) { try { g = std::stoi(gv.get<std::string>()); } catch (...) {} }
                pd._commandGroup = static_cast<CommandGroups>(g);
            }
            if (pe.contains(kTabName))      pd._tabName      = pe[kTabName].is_string() ? pe[kTabName].get<std::string>() : "";
            if (pe.contains(kRunPriority))
            {
                std::string cl;
                if (pe[kRunPriority].is_string()) cl = pe[kRunPriority].get<std::string>();
                else if (pe[kRunPriority].is_number()) cl = std::to_string(pe[kRunPriority].get<int>());
                auto pos = cl.find(',');
                if (pos != std::string::npos)
                {
                    try { pd._cellX = std::stoi(cl.substr(0, pos)); } catch (...) {}
                    try { pd._cellY = std::stoi(cl.substr(pos+1)); } catch (...) {}
                }
            }
            _pinEntries[pd._hash] = pd;
        }
    }

    return result;
}

void _PIC32CXPlatformConfiguration::write(json& j)
{
    _PlatformConfiguration::write(j);

    json pinList = json::array();
    for (const auto& kv : _pinEntries)
    {
        const auto& pd = kv.second;
        json p;
        p[kPinNumber]    = static_cast<int>(pd._setPin);
        p[kEnabled]      = pd._enabled;
        p[kInverted]     = pd._inverted;
        p[kName]         = pd._pinLabel;
        p[kToolTip]      = pd._pinTooltip;
        p[kCommand]      = pd._pinCommand;
        p[kCommmandGroup]= static_cast<int>(pd._commandGroup);
        p[kRunPriority]  = std::to_string(pd._cellX) + "," + std::to_string(pd._cellY);
        p[kTabName]      = pd._tabName;
        pinList.push_back(p);
    }
    j[kPlatformEntries] = pinList;
}

void _PIC32CXPlatformConfiguration::initialize()
{
    auto add = [&](PinID setPin, bool enabled, const std::string& label, const std::string& cmd,
                   const std::string& tip, CommandGroups group, const std::string& tab, int cx, int cy,
                   bool inverted = false)
    {
        PIC32CXPinData pd(setPin);
        pd._enabled = enabled; pd._inverted = inverted;
        pd._pinLabel = label; pd._pinCommand = cmd; pd._pinTooltip = tip;
        pd._commandGroup = group; pd._tabName = tab;
        pd._cellX = cx; pd._cellY = cy;
        _pinEntries[pd._hash] = pd;
    };

    add(3,   false,"JMI2C.2",              "iic2",          "Debug pin",                          eConnectionGroup,    "General",-1,-1);
    add(4,   true, "Battery",              "battery",       "Disconnects power to the device",    eConnectionGroup,    "General", 0, 0);
    add(6,   true, "KK Power Enable",      "kkpwr",         "Enables KK Power",                   eButtonGroup,        "General", 0, 0);
    add(7,   true, "MD PS HOLD",           "pshold",        "MD PS HOLD",                         eButtonGroup,        "General", 1, 0);
    add(10,  true, "KK Resin N",           "sresn",         "KK Resin N",                         eButtonGroup,        "General", 2, 0);
    add(11,  false,"PMS Power On",         "pmspwr",        "PMS Power On",                       eButtonGroup,        "General",-1,-1);
    add(16,  true, "UEFI",                 "uefi",          "UEFI",                               eButtonGroup,        "General", 0, 2);
    add(20,  true, "Power Off",            "pkey",          "Power Off",                          eButtonGroup,        "General", 0, 1);
    add(21,  true, "PCIE0 Attention",      "pcie0",         "PCIE 0 Attention",                   eSwitchGroup,        "General", 0, 0);
    add(27,  true, "PCIE1 Attention",      "pcie1",         "PCIE 1 Attention",                   eSwitchGroup,        "General", 1, 0);
    add(101, true, "PCIE2 Attention",      "pcie2",         "PCIE 2 Attention",                   eSwitchGroup,        "General", 2, 0);
    add(102, true, "PCIE3 Attention",      "pcie3",         "PCIE 3 Attention",                   eSwitchGroup,        "General", 0, 1);
    add(103, false,"JMI2C.10",             "iic10",         "Debug pin",                          eSwitchGroup,        "General",-1,-1);
    add(110, false,"Kratos Trigger",       "kratos",        "Kratos Trigger",                     eSwitchGroup,        "General",-1,-1);
    add(111, false,"JMI2C.4",              "iic4",          "Debug pin",                          eSwitchGroup,        "General",-1,-1);
    add(114, false,"QCC BLD Disconnect",   "qccbld",        "QCC BLD Disconnect",                 eButtonGroup,        "General",-1,-1);
    add(115, false,"BB BLD Disconnect",    "bbbld",         "BB BLD Disconnect",                  eSwitchGroup,        "General",-1,-1);
    add(116, true, "Secondary Fastboot",   "sfastboot",     "Fastboot SS",                        eSwitchGroup,        "General", 1, 1);
    add(117, false,"JMI2C.5",              "iic5",          "Debug pin",                          eSwitchGroup,        "General",-1,-1);
    add(118, false,"PG QAM VR4P5",         "inputvr4p5",    "PG QAM VR4P5",                       eButtonGroup,        "General",-1,-1);
    add(119, false,"PG QAM SAIL VR3P3",    "inputvr3p3",    "PG QAM SAIL VR3P3",                  eButtonGroup,        "General",-1,-1);
    add(120, true, "Force MD PS HOLD",     "forcemdpshold", "Force MD PS HOLD",                   eButtonGroup,        "General", 1, 1);
    add(121, true, "Force SS PS HOLD",     "forcesspshold", "Force SS PS HOLD",                   eButtonGroup,        "General", 2, 1);
    add(123, false,"",                     "",              "",                                   eUnknownCommandGroup,"General",-1,-1);
    add(128, true, "USB0",                 "usb0",          "Disconnect USB0",                    eConnectionGroup,    "General", 1, 0);
    add(129, false,"PMS PGOOD",            "pgood",         "-",                                  eButtonGroup,        "General", 4, 0);
    add(204, false,"",                     "",              "",                                   eUnknownCommandGroup,"General",-1,-1);
    add(205, true, "USB2",                 "usb2",          "USB 2 Disable",                      eConnectionGroup,    "General", 0, 1);
    add(206, false,"Fast Power Off",       "inputpwr",      "Fast K Power Off Enable",            eButtonGroup,        "General", 4, 2);
    add(207, true, "Mode 0",               "mode0",         "Mode 0 for Primary SOC",             eButtonGroup,        "General", 1, 2);
    add(215, true, "Mode 1",               "mode1",         "Mode 1",                             eButtonGroup,        "General", 2, 2);
    add(216, true, "MD EDL",               "pedl",          "MD EDL",                             eButtonGroup,        "General", 0, 3);
    add(217, false,"Mode 0",               "mode0",         "Mode 0",                             eButtonGroup,        "General",-1,-1);
    add(219, true, "SS EDL",               "sedl",          "SS EDL",                             eButtonGroup,        "General", 1, 3);
    add(221, false,"JMI2C.13",             "iic13",         "Debug pin",                          eButtonGroup,        "General",-1,-1);
    add(224, false,"PMS Fast Power Off",   "inputpoff",     "PMS Fast Power Off",                 eUnknownCommandGroup,"General",-1,-1);
    add(225, false,"PMS Enable",           "inputpms",      "PMS Enable",                         eUnknownCommandGroup,"General",-1,-1);
    add(226, false,"BB BLD EN",            "bbblden",       "-",                                  eButtonGroup,        "General",-1,-1);
    add(227, false,"QCC BLD EN",           "qccblden",      "-",                                  eButtonGroup,        "General",-1,-1);
    add(228, false,"Mode 2",               "mode2",         "Mode 2",                             eSwitchGroup,        "General",-1,-1);
    add(311, true, "EUD",                  "eud",           "EUD for Primary SOC",                eButtonGroup,        "General", 2, 3);
    add(312, true, "USB1",                 "usb1",          "USB1 Disconnect",                    eConnectionGroup,    "General", 2, 0);
    add(320, false,"JMI2C.7",              "iic7",          "Debug I2C 7",                        eButtonGroup,        "General",-1,-1);
    add(321, true, "Fastboot MD",          "fastboot",      "MD Fastboot",                        eSwitchGroup,        "General", 2, 1);

    // Quick Settings
    _buttons.clear();
    auto addBtn = [&](const std::string& label, const std::string& tip, const std::string& cmd, int cx, int cy)
    {
        Button btn;
        btn._label = label; btn._tab = "General";
        btn._cellX = cx; btn._cellY = cy;
        btn._commandGroup = eQuickSettingsGroup;
        btn._toolTip = tip; btn._command = cmd;
        btn._hash = Button::makeHash(btn);
        _buttons[btn._hash] = btn;
    };

    addBtn("Power On",              "Power on the MTP/Device",                    "powerOn",              0, 0);
    addBtn("Power Off",             "Power off the MTP/Device",                   "powerOff",             1, 0);
    addBtn("Boot MD EDL",           "Boots the device to primary EDL",            "bootToEDL",            2, 0);
    addBtn("Boot SS EDL",           "Boots the device to secondary EDL",          "bootToSecondaryEDL",   0, 1);
    addBtn("Boot to UEFI",          "Boots the device to UEFI",                   "bootToUEFI",           1, 1);
    addBtn("Boot to MD Fastboot",   "Boots the device to primary fastboot",       "bootToFastboot",       2, 1);
    addBtn("Boot to SS MD Fastboot","Boots the device to secondary fastboot",     "bootToSecondaryFastboot",3,0);

    _alpacaScript = AlpacaScript::defaultScript(_platform);

    // Script variables
    _scriptVariables.clear();
    auto addVar = [&](const std::string& name, const std::string& label, const std::string& tip, int defVal, int cx, int cy)
    {
        ScriptVariable v;
        v._name = name; v._label = label; v._tooltip = tip;
        v._type = eIntegerType; v._defaultValue = defVal;
        v._cellX = cx; v._cellY = cy;
        _scriptVariables[name] = v;
    };

    addVar("edl",      "EDL timing (ms)",      "Configurable Boot to EDL timing in milliseconds",      1500,  0, 0);
    addVar("uefi",     "UEFI timing (ms)",     "Configurable Boot to UEFI timing in milliseconds",     10000, 0, 1);
    addVar("fastboot", "Fastboot timing (ms)", "Configurable Boot to fastboot timing in milliseconds", 10000, 1, 0);
}
