// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

/*
	Author: Biswajit Roy (biswroy@qti.qualcomm.com)
*/

#include "PSOCPlatformConfiguration.h"

#include "PlatformConfigurationException.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <string>

using json = nlohmann::json;

static const char* kPlatformEntries    = "pins";
static const char* kMinFirmwareVersion = "supportedFirmwareVer";
static const char* kRunPriority        = "run_priority";
static const char* kPinNumber          = "pin_number";
static const char* kEnabled            = "enabled";
static const char* kIntialValue        = "initial_value";
static const char* kPriority           = "initialization_priority";
static const char* kInverted           = "inverted";
static const char* kName               = "name";
static const char* kToolTip            = "help_hint";
static const char* kCommand            = "command";
static const char* kCommmandGroup      = "command_group";
static const char* kClassicAction      = "classic_action";
static const char* kTabName            = "tab_name";

const bool kDefaultInitialPinValue{false};
const bool kDefaultPinInvertedState{false};
const CommandGroups kDefaultCommandGroup{eUnknownCommandGroup};

PSOCPinEntries _PSOCPlatformConfiguration::_classicActions;

_PSOCPlatformConfiguration::_PSOCPlatformConfiguration()
{
    _platform   = ePSOC;
    _platformId = kMaxPSOCPlatformId;
    _resetActive = true;

    if (_classicActions.empty())
        _PSOCPlatformConfiguration::initialize();

    for (const auto& kv : _classicActions)
        _pinEntries[kv.second._pin] = kv.second;

    Tab generalTab, deviceInfoTab, i2cTab, fusionTab, terminalTab;

    generalTab._name = "General";       generalTab._moveable = false; generalTab._configurable = true;  generalTab._ordinal = 0; generalTab._userTab = false;
    deviceInfoTab._name = "Device Info";deviceInfoTab._moveable = false;deviceInfoTab._configurable = false;deviceInfoTab._ordinal = 1;deviceInfoTab._userTab = false;
    i2cTab._name = "I2C";               i2cTab._moveable = true;  i2cTab._visible = true;  i2cTab._configurable = false; i2cTab._ordinal = 2;  i2cTab._userTab = true;
    fusionTab._name = "Fusion";         fusionTab._moveable = true;  fusionTab._configurable = true;  fusionTab._ordinal = 3;  fusionTab._userTab = true;
    terminalTab._name = "Terminal";     terminalTab._moveable = true; terminalTab._configurable = false;terminalTab._ordinal = 4;terminalTab._userTab = false;

    _tabs.push_back(generalTab);
    _tabs.push_back(deviceInfoTab);
    _tabs.push_back(i2cTab);
    _tabs.push_back(fusionTab);
    _tabs.push_back(terminalTab);

    _supportedFirmwareVer.push_back(kDefaultFirmwareVersion);
}

_PSOCPlatformConfiguration::~_PSOCPlatformConfiguration()
{
}

Pins _PSOCPlatformConfiguration::getPins()
{
    Pins result;
    for (const auto& kv : _pinEntries)
    {
        const auto& pd = kv.second;
        if (!pd._enabled) continue;
        PinEntry pe;
        pe._pin      = pd._pin;
        pe._enabled  = pd._enabled;
        pe._hash     = pd._hash;
        pe._cellX    = pd._cellX;
        pe._cellY    = pd._cellY;
        pe._commandGroup = pd._commandGroup;
        pe._pinCommand   = pd._pinCommand;
        pe._initialValue = pd._initialValue;
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

PSOCPinList _PSOCPlatformConfiguration::getAllPins()
{
    PSOCPinList result;
    for (const auto& kv : _pinEntries) result.push_back(kv.second);
    std::sort(result.begin(), result.end(), [](const PSOCPinData& a, const PSOCPinData& b){ return a._pin < b._pin; });
    return result;
}

PSOCPinList _PSOCPlatformConfiguration::getActivePins()
{
    PSOCPinList result;
    for (const auto& kv : _pinEntries)
        if (kv.second._enabled) result.push_back(kv.second);
    std::sort(result.begin(), result.end(), [](const PSOCPinData& a, const PSOCPinData& b){ return a._pin < b._pin; });
    return result;
}

bool _PSOCPlatformConfiguration::getPinEnableState(PinID pinId) const
{
    auto it = _pinEntries.find(pinId);
    return it != _pinEntries.end() ? it->second._enabled : true;
}

void _PSOCPlatformConfiguration::setPinEnableState(PinID pinId, bool newState)
{
    auto it = _pinEntries.find(pinId);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to set enable state for an invalid pin!");
    if (it->second._enabled != newState) { it->second._enabled = newState; _dirty = true; }
}

bool _PSOCPlatformConfiguration::getInitialPinValue(PinID pinId) const
{
    auto it = _pinEntries.find(pinId);
    return it != _pinEntries.end() ? it->second._initialValue : kDefaultInitialPinValue;
}

void _PSOCPlatformConfiguration::setInitialPinValue(PinID pinId, bool newState)
{
    auto it = _pinEntries.find(pinId);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to set initial pin value for an invalid pin!");
    if (it->second._initialValue != newState) { it->second._initialValue = newState; _dirty = true; }
}

uint64_t _PSOCPlatformConfiguration::getPinInitializationPriority(PinID pinId) const
{
    auto it = _pinEntries.find(pinId);
    return it != _pinEntries.end() ? static_cast<uint64_t>(it->second._initializationPriority) : static_cast<uint64_t>(pinId);
}

void _PSOCPlatformConfiguration::setPinInitializationPriority(PinID pinId, int priority)
{
    auto it = _pinEntries.find(pinId);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to set priority for an invalid pin!");
    if (it->second._initializationPriority != priority) { it->second._initializationPriority = priority; _dirty = true; }
}

std::string _PSOCPlatformConfiguration::getPinLabel(PinID pinId) const
{
    auto it = _pinEntries.find(pinId);
    return it != _pinEntries.end() ? it->second._pinLabel : "";
}

void _PSOCPlatformConfiguration::setPinLabel(PinID pinId, const std::string& v)
{
    auto it = _pinEntries.find(pinId);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to set pin label for an invalid pin!");
    it->second._pinLabel = v;
}

std::string _PSOCPlatformConfiguration::getPinTooltip(PinID pinId) const
{
    auto it = _pinEntries.find(pinId);
    return it != _pinEntries.end() ? it->second._pinTooltip : "";
}

void _PSOCPlatformConfiguration::setPinTooltip(PinID pinId, const std::string& v)
{
    auto it = _pinEntries.find(pinId);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to set tooltip for an invalid pin!");
    it->second._pinTooltip = v;
}

bool _PSOCPlatformConfiguration::getPinInvertedState(PinID pinId) const
{
    auto it = _pinEntries.find(pinId);
    return it != _pinEntries.end() ? it->second._inverted : kDefaultPinInvertedState;
}

void _PSOCPlatformConfiguration::setPinInvertedState(PinID pinId, bool newState)
{
    auto it = _pinEntries.find(pinId);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to set invert state for an invalid pin!");
    it->second._inverted = newState;
}

std::string _PSOCPlatformConfiguration::getPinCommand(PinID pinId) const
{
    auto it = _pinEntries.find(pinId);
    return it != _pinEntries.end() ? it->second._pinCommand : "";
}

void _PSOCPlatformConfiguration::setPinCommand(PinID pinId, const std::string& v)
{
    auto it = _pinEntries.find(pinId);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to set pin command for an invalid pin!");
    it->second._pinCommand = v;
}

CommandGroups _PSOCPlatformConfiguration::getPinGroup(PinID pinId) const
{
    auto it = _pinEntries.find(pinId);
    return it != _pinEntries.end() ? it->second._commandGroup : kDefaultCommandGroup;
}

void _PSOCPlatformConfiguration::setPinGroup(PinID pinId, CommandGroups cg)
{
    auto it = _pinEntries.find(pinId);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to set pin group for an invalid pin!");
    it->second._commandGroup = cg;
}

std::string _PSOCPlatformConfiguration::getClassicAction(PinID pinId) const
{
    auto it = _pinEntries.find(pinId);
    return it != _pinEntries.end() ? it->second._classicAction : "";
}

void _PSOCPlatformConfiguration::setClassicAction(PinID pinId, const std::string& v)
{
    auto it = _pinEntries.find(pinId);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to set classic action for an invalid pin!");
    it->second._classicAction = v;
}

void _PSOCPlatformConfiguration::getPinCellLocation(PinID pinId, int& x, int& y) const
{
    auto it = _pinEntries.find(pinId);
    if (it != _pinEntries.end()) { x = it->second._cellX; y = it->second._cellY; }
    else { x = -1; y = -1; }
}

void _PSOCPlatformConfiguration::setPinCellLocation(PinID pinId, int x, int y)
{
    auto it = _pinEntries.find(pinId);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to set cell location for an invalid pin!");
    it->second._cellX = x; it->second._cellY = y;
}

std::string _PSOCPlatformConfiguration::getTabName(PinID pinId) const
{
    auto it = _pinEntries.find(pinId);
    return it != _pinEntries.end() ? it->second._tabName : "";
}

void _PSOCPlatformConfiguration::setTabName(PinID pinId, const std::string& v)
{
    auto it = _pinEntries.find(pinId);
    if (it == _pinEntries.end()) throw PlatformConfigurationException("Attempt to set tab name for an invalid pin!");
    it->second._tabName = v;
}

void _PSOCPlatformConfiguration::cascadeTabDelete(const std::string& deleteMe)
{
    _tabs.erase(std::remove_if(_tabs.begin(), _tabs.end(),
        [&](const Tab& t){ return t._name == deleteMe; }), _tabs.end());
    for (auto& kv : _pinEntries)
        if (kv.second._tabName == deleteMe)
            kv.second._tabName.clear();
}

void _PSOCPlatformConfiguration::cascadeTabRename(const std::string& oldName, const std::string& newName)
{
    for (auto& kv : _pinEntries)
        if (kv.second._tabName == oldName)
            kv.second._tabName = newName;
}

bool _PSOCPlatformConfiguration::read(const json& j)
{
    bool result = _PlatformConfiguration::read(j);

    _supportedFirmwareVer.clear();
    if (j.contains(kMinFirmwareVersion) && j[kMinFirmwareVersion].is_array())
    {
        for (const auto& v : j[kMinFirmwareVersion])
        {
            uint32_t fv = 0;
            if (v.is_number()) fv = v.get<uint32_t>();
            else if (v.is_string()) { try { fv = static_cast<uint32_t>(std::stoul(v.get<std::string>())); } catch (...) {} }
            _supportedFirmwareVer.push_back(fv);
        }
    }
    if (_supportedFirmwareVer.empty())
        _supportedFirmwareVer.push_back(kDefaultFirmwareVersion);

    if (j.contains(kPlatformEntries) && j[kPlatformEntries].is_array())
    {
        for (const auto& pe : j[kPlatformEntries])
        {
            PSOCPinData pd;
            if (pe.contains(kPinNumber))
            {
                auto& pn = pe[kPinNumber];
                if (pn.is_number()) pd._pin = static_cast<PinID>(pn.get<uint64_t>());
                else if (pn.is_string()) { try { pd._pin = static_cast<PinID>(std::stoull(pn.get<std::string>())); } catch (...) {} }
            }
            if (pe.contains(kEnabled))      pd._enabled= pe[kEnabled].get<bool>();
            if (pe.contains(kIntialValue))  pd._initialValue = pe[kIntialValue].get<bool>();
            if (pe.contains(kPriority))
            {
                auto& pv = pe[kPriority];
                if (pv.is_number()) pd._initializationPriority = pv.get<int>();
                else if (pv.is_string()) { try { pd._initializationPriority = std::stoi(pv.get<std::string>()); } catch (...) {} }
            }
            if (pe.contains(kInverted))     pd._inverted = pe[kInverted].get<bool>();
            if (pe.contains(kName))         pd._pinLabel = pe[kName].is_string() ? pe[kName].get<std::string>() : "";
            if (pe.contains(kToolTip))      pd._pinTooltip = pe[kToolTip].is_string() ? pe[kToolTip].get<std::string>() : "";
            if (pe.contains(kCommand))      pd._pinCommand = pe[kCommand].is_string() ? pe[kCommand].get<std::string>() : "";
            if (pe.contains(kCommmandGroup))
            {
                auto& gv = pe[kCommmandGroup];
                int g = 0;
                if (gv.is_number()) g = gv.get<int>();
                else if (gv.is_string()) { try { g = std::stoi(gv.get<std::string>()); } catch (...) {} }
                pd._commandGroup = static_cast<CommandGroups>(g);
            }
            if (pe.contains(kClassicAction))pd._classicAction = pe[kClassicAction].is_string() ? pe[kClassicAction].get<std::string>() : "";
            if (pe.contains(kTabName))      pd._tabName = pe[kTabName].is_string() ? pe[kTabName].get<std::string>() : "";
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
            pd._hash = pd._pin;
            _pinEntries[pd._pin] = pd;
        }
    }

    return result;
}

void _PSOCPlatformConfiguration::write(json& j)
{
    _PlatformConfiguration::write(j);

    json fwList = json::array();
    for (auto v : _supportedFirmwareVer) fwList.push_back(static_cast<int>(v));
    j[kMinFirmwareVersion] = fwList;

    json pinList = json::array();
    for (const auto& kv : _pinEntries)
    {
        const auto& pd = kv.second;
        json p;
        p[kPinNumber]    = std::to_string(pd._pin);
        p[kEnabled]      = pd._enabled;
        p[kIntialValue]  = pd._initialValue;
        p[kPriority]     = pd._initializationPriority;
        p[kInverted]     = pd._inverted;
        p[kName]         = pd._pinLabel;
        p[kToolTip]      = pd._pinTooltip;
        p[kCommand]      = pd._pinCommand;
        p[kCommmandGroup]= static_cast<int>(pd._commandGroup);
        p[kClassicAction]= pd._classicAction;
        p[kRunPriority]  = std::to_string(pd._cellX) + "," + std::to_string(pd._cellY);
        p[kTabName]      = pd._tabName;
        pinList.push_back(p);
    }
    j[kPlatformEntries] = pinList;
}

void _PSOCPlatformConfiguration::initialize()
{
    auto add = [](PinID pin, const std::string& label, const std::string& cmd, const std::string& tip,
                  int priority, const std::string& tab, CommandGroups group, bool enabled,
                  const std::string& classic, int cx, int cy, bool inverted = false) -> PSOCPinData
    {
        PSOCPinData pd;
        pd._pin = pin; pd._hash = pin;
        pd._pinLabel = label; pd._pinCommand = cmd; pd._pinTooltip = tip;
        pd._initializationPriority = priority; pd._tabName = tab;
        pd._commandGroup = group; pd._enabled = enabled;
        pd._classicAction = classic;
        pd._cellX = cx; pd._cellY = cy;
        pd._initialValue = false; pd._inverted = inverted;
        return pd;
    };

    auto& ca = _PSOCPlatformConfiguration::_classicActions;

    ca[53] = add(53,"Battery",                              "battery","Battery Disconnect",                    2,"General",eConnectionGroup,true,"TAC_POWER_OFF",          0,0,true);
    ca[55] = add(55,"USB 0",                                "usb0",   "Disconnnects VBUS for USB 0",           2,"General",eConnectionGroup,true,"TAC_USB0_DIS",           1,0,true);
    ca[36] = add(36,"USB 1",                                "usb1",   "Disconnects VBUS for USB1",             2,"General",eConnectionGroup,true,"TAC_USB1_DIS",           2,0,true);
    ca[50] = add(50,"Power Key",                            "pkey",   "Presses the power key button",         -1,"General",eButtonGroup,    true,"TAC_TC_START",           0,0);
    ca[34] = add(34,"Volume Up",                            "volup",  "Presses the volume up button",         -1,"General",eButtonGroup,    true,"TAC_VOL_UP",             0,1);
    ca[51] = add(51,"Volume Down",                          "voldn",  "Presses the volume down button",       -1,"General",eButtonGroup,    true,"TAC_RESET_PRI",          0,2);
    ca[18] = add(18,"Disconnect UIM 1",                     "uim1",   "Disconnects UIM 1",                    -1,"General",eSwitchGroup,    true,"TAC_UIM1_DIS",           0,0);
    ca[19] = add(19,"Disconnect UIM 2",                     "uim2",   "Disconnects UIM 2",                    -1,"General",eSwitchGroup,    true,"TAC_UIM2_DIS",           1,0);
    ca[54] = add(54,"Emergency Download Mode (EDL)",        "pedl",   "Enables Primary Emergency Download Mode",-1,"General",eSwitchGroup, true,"TAC_SW_DWNLD_PRI",       0,1);
    ca[37] = add(37,"Force PS_HOLD High",                   "pshold", "Forces PSHold high",                   -1,"General",eSwitchGroup,    true,"TAC_FORCE_PS_HOLD",      0,2);
    ca[47] = add(47,"Disconnect SD Card",                   "sdcard", "Disconnects the SD card slot",         -1,"General",eSwitchGroup,    true,"TAC_SDCARD_DISC",        0,3);
    ca[16] = add(16,"Embedded USB Debug (EUD)",             "eud",    "Enabled the embedded USB debugger",    -1,"General",eSwitchGroup,    true,"TC_EUD_EARLY_BOOT_EN",   0,4);
    ca[21] = add(21,"Headset Disconnect",                   "headset","Disconnects the headset",              -1,"General",eSwitchGroup,    true,"TAC_HEADSET_DIS",        0,5);
    ca[30] = add(30,"Secondary Emergency Download Mode (EDL)","sedl", "Secondary EDL",                       -1,"Fusion", eSwitchGroup,    true,"TAC_FORCE_USB_BOOT_SEC", 0,0);
    ca[31] = add(31,"Secondary PM_RESIN_N",                 "sresn",  "Secondary PM_RESIN_N",                 -1,"Fusion", eSwitchGroup,    true,"TAC_RESET_SEC",          0,1);

    // Disabled pins
    for (PinID p : {29u, 38u, 39u, 46u, 48u, 49u})
    {
        PSOCPinData pd; pd._pin = p; pd._hash = p;
        pd._initializationPriority = -1; pd._enabled = false;
        ca[p] = pd;
    }

    // Pin 15 — soft reset (disabled)
    PSOCPinData pd15; pd15._pin = 15; pd15._hash = 15;
    pd15._initializationPriority = -1; pd15._enabled = false;
    pd15._classicAction = "TAC_SOFT_RESET";
    ca[15] = pd15;
}
