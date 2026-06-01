/*
	Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted (subject to the limitations in the
	disclaimer below) provided that the following conditions are met:

		* Redistributions of source code must retain the above copyright
		  notice, this list of conditions and the following disclaimer.

		* Redistributions in binary form must reproduce the above
		  copyright notice, this list of conditions and the following
		  disclaimer in the documentation and/or other materials provided
		  with the distribution.

		* Neither the name of Qualcomm Technologies, Inc. nor the names of its
		  contributors may be used to endorse or promote products derived
		  from this software without specific prior written permission.

	NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
	GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
	HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
	WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
	IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
	ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
	GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
	IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
	OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
	IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
	Author: Biswajit Roy (biswroy@qti.qualcomm.com)
*/


#include "FTDIPlatformConfiguration.h"

#include "DebugBoardType.h"
#include "PlatformConfigurationException.h"
#include "StringUtilities.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

using json = nlohmann::json;

static const char* kChipCount   = "chip_count";
static const char* kChipIndex   = "chip_index";
static const char* kBus         = "bus";
static const char* kPinNumber   = "pin_number";
static const char* kEnabled     = "enabled";
static const char* kInput       = "input";
static const char* kName        = "name";
static const char* kToolTip     = "help_hint";
static const char* kInitialValue= "initial_value";
static const char* kPriority    = "priority";
static const char* kInverted    = "inverted";
static const char* kCommand     = "command";
static const char* kCommandGroup= "command_group";
static const char* kTabName     = "group";
static const char* kBusFunction = "bus_function";
static const char* kPinEntries  = "pins";
static const char* kBusEntries  = "bus";
static const char* kCellLocation= "cellLocation";

const int kMaximumChipCount{4};

static std::string toLower(const std::string& s)
{
    std::string r;
    for (auto c : s) r += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

_FTDIPlatformConfiguration::_FTDIPlatformConfiguration(uint16_t chipCount)
{
    _resetActive = false;

    Tab generalTab, deviceInfoTab, fusionTab, terminalTab;

    generalTab._name = "General";       generalTab._visible = true;  generalTab._moveable = false; generalTab._configurable = true;  generalTab._ordinal = 0; generalTab._userTab = false;
    deviceInfoTab._name = "Device Info";deviceInfoTab._visible = true;deviceInfoTab._moveable = false;deviceInfoTab._configurable = false;deviceInfoTab._ordinal = 1;deviceInfoTab._userTab = false;
    fusionTab._name = "Fusion";         fusionTab._visible = true;   fusionTab._moveable = true;   fusionTab._configurable = false;  fusionTab._ordinal = 3;  fusionTab._userTab = false;
    terminalTab._name = "Terminal";     terminalTab._visible = true; terminalTab._moveable = true;  terminalTab._configurable = false;terminalTab._ordinal = 4;terminalTab._userTab = false;

    _tabs.push_back(generalTab);
    _tabs.push_back(deviceInfoTab);
    _tabs.push_back(fusionTab);
    _tabs.push_back(terminalTab);

    initialize(chipCount);
}

void _FTDIPlatformConfiguration::initialize(uint16_t chipCount)
{
    _platform = eFTDI;

    static const std::vector<uint32_t> pinList = {0,1,2,3,4,5,6,7};

    if (chipCount > kMaximumChipCount)
        throw PlatformConfigurationException("Invalid chip count");

    _chipCount = chipCount;

    for (int chipIndex = 0; chipIndex < chipCount; ++chipIndex)
    {
        std::vector<Bus> busList;
        if (chipIndex == 0)
            busList = {'A','B'};
        else
            busList = {'A','B','C','D'};

        for (auto busIndex : busList)
        {
            for (auto pinIndex : pinList)
            {
                FTDIPinData pinData(chipIndex, busIndex, pinIndex);
                FTDIBusData busData(chipIndex, busIndex,
                    (busIndex == 'A' || busIndex == 'B') ? eBusFunctionVCP : eBusFunctionD2XX);

                pinData._initializationPriority = -1;
                pinData._commandGroup = eUnknownCommandGroup;
                pinData._cellX = -1; pinData._cellY = -1;

                _pinEntries[pinData._hash] = pinData;
                _busFunctions[busData._hash] = busData;
            }
        }
    }

    auto addPin = [&](int chip, Bus bus, int chipPin, bool enabled, bool input,
                      const std::string& label, const std::string& cmd, const std::string& tip,
                      CommandGroups group, const std::string& tab, int cx, int cy,
                      bool inverted = false, int initPriority = -1, bool initialValue = false)
    {
        FTDIPinData pd(chip, bus, chipPin);
        pd._setPin = getSetPinIndex(chip, bus, chipPin);
        pd._enabled = enabled; pd._input = input;
        pd._initialValue = initialValue; pd._inverted = inverted;
        pd._initializationPriority = initPriority;
        pd._pinLabel = label; pd._pinCommand = cmd; pd._pinTooltip = tip;
        pd._commandGroup = group; pd._tabName = tab;
        pd._cellX = cx; pd._cellY = cy;
        _pinEntries[pd._hash] = pd;

        FTDIBusData bd(chip, bus, eBusFunctionD2XX);
        _busFunctions[bd._hash] = bd;
    };

    // Default pin assignments for ALPACA-LITE MTP DEBUG BOARD (platform ID 13).
    // MUST match the original Qt qcommon-console FTDIPlatformConfiguration defaults EXACTLY.
    // Verified line-by-line against qcommon-console/FTDIPlatformConfiguration.cpp lines 165-627.
    //
    // Parameters: chip, bus, pin, enabled, input, label, command, tooltip, group, tab, cellX, cellY, inverted, initPriority
    //
    // Bus C pins (CDBUS0-7):
    addPin(0,'C',0,true,true, "Force PS_HOLD High",                       "pshold",  "Force PS_HOLD to high",              eSwitchGroup,    "General",0,2);
    addPin(0,'C',1,true,true, "Disconnect UIM1",                          "uim1",    "Disconnects the UIM 1",              eSwitchGroup,    "General",0,0);
    addPin(0,'C',2,true,true, "Headset Disconnect",                       "headset", "Disconnects headset",                eSwitchGroup,    "General",0,5);
    addPin(0,'C',3,true,true, "Disconnect UIM2",                          "uim2",    "Disconnects the UIM 2",              eSwitchGroup,    "General",1,0);
    addPin(0,'C',4,true,true, "Disconnect SD Card",                       "sdcard",  "Disconnects the SD Card",            eSwitchGroup,    "General",0,3);
    addPin(0,'C',5,true,true, "Secondary Emergency Download Mode (EDL)",  "sedl",    "Secondary Emergency Download Mode",  eSwitchGroup,    "Fusion", 0,0);
    addPin(0,'C',6,true,true, "USB 1 (VBUS Only)",                        "usb1",    "Disconnects VBUS 1",                 eConnectionGroup,"General",2,0, /*inverted=*/true, /*initPriority=*/2);
    addPin(0,'C',7,true,true, "Secondary PM_RESIN_N_SEC",                 "sresn",   "Fusion Secondary PM_RESIN_N",        eSwitchGroup,    "Fusion", 0,1);
    // Bus D pins (DDBUS0-7):
    addPin(0,'D',0,true,true, "Volume Up",                                "volup",   "VOL_UP + PWR_ON = Held for boot to UEFI menu", eButtonGroup,"General",0,1);
    addPin(0,'D',1,true,true, "Battery",                                  "battery", "Battery power off/on",               eConnectionGroup,"General",0,0, /*inverted=*/true, /*initPriority=*/2);
    addPin(0,'D',2,true,true, "Volume Down",                              "voldn",   "(PM_RESIN_N) (Held to boot to fastboot)", eButtonGroup,"General",0,2);
    addPin(0,'D',3,true,true, "Power Key",                                "pkey",    "Power On (VOL_UP + PWR_ON = Held for boot to UEFI menu)", eButtonGroup,"General",0,0);
    addPin(0,'D',4,true,true, "EUD",                                      "eud",     "Embedded USB Debug (EUD)",           eSwitchGroup,    "General",0,4);
    addPin(0,'D',5,true,true, "EDL",                                      "pedl",    "Primary Emergency Download Mode",    eSwitchGroup,    "General",0,1);
    // D6: TC_READY_N — CRITICAL: must be set HIGH at init to enable output on other FTDI pins.
    // Without this, pins DDBUS 0/2/4/7 and CDBUS 0/2/4/7 are disabled.
    addPin(0,'D',6,true,true, "<type a label name>",                      "TC_READY_N","SW must program it to 1 to enable some output signals", eUnknownCommandGroup,"<select a group>",-1,-1, /*inverted=*/false, /*initPriority=*/1, /*initialValue=*/true);
    addPin(0,'D',7,true,false,"USB 0 (VBUS Only)",                        "usb0",    "Disconnects VBUS0",                  eConnectionGroup,"General",1,0, /*inverted=*/true, /*initPriority=*/2);
}

_FTDIPlatformConfiguration::~_FTDIPlatformConfiguration()
{
}

FTDIPinData _FTDIPlatformConfiguration::getPinData(ChipIndex chipIndex, Bus bus, PinID pin)
{
    HashType hash = FTDIPinData::makeFTDIHash(chipIndex, bus, pin);
    auto it = _pinEntries.find(hash);
    if (it != _pinEntries.end()) return it->second;
    return {};
}

FTDIPinList _FTDIPlatformConfiguration::getAllPins() const
{
    FTDIPinList result;
    for (const auto& kv : _pinEntries) result.push_back(kv.second);
    return result;
}

FTDIPinList _FTDIPlatformConfiguration::getActivePins() const
{
    FTDIPinList result;
    for (const auto& kv : _pinEntries)
        if (kv.second._enabled) result.push_back(kv.second);
    return result;
}

FTDIPinList _FTDIPlatformConfiguration::getActivePins(ChipIndex chipIndex, Bus bus) const
{
    FTDIPinList result;
    for (const auto& kv : _pinEntries)
        if (kv.second._enabled && kv.second._chipIndex == chipIndex && kv.second._bus == bus)
            result.push_back(kv.second);
    return result;
}

int _FTDIPlatformConfiguration::getChipCount() { return _chipCount; }

Pins _FTDIPlatformConfiguration::getPins()
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
        pe._pinLabel = pd._pinLabel;
        pe._pinTooltip = pd._pinTooltip;
        pe._initialValue = pd._initialValue;
        pe._initializationPriority = pd._initializationPriority;
        pe._inverted = pd._inverted;
        pe._pinCommand = pd._pinCommand;
        pe._commandGroup = pd._commandGroup;
        pe._cellX = pd._cellX; pe._cellY = pd._cellY;
        pe._tabName = pd._tabName;
        result.push_back(pe);
    }
    return result;
}

static FTDIPinData* findPin(FTDIPinEntries& entries, ChipIndex chip, Bus bus, PinID pin)
{
    HashType h = FTDIPinData::makeFTDIHash(chip, bus, pin);
    auto it = entries.find(h);
    return (it != entries.end()) ? &it->second : nullptr;
}

bool _FTDIPlatformConfiguration::getPinEnableState(ChipIndex c, Bus b, PinID p) const
{ HashType h = FTDIPinData::makeFTDIHash(c,b,p); auto it = _pinEntries.find(h); return it != _pinEntries.end() ? it->second._enabled : false; }
void _FTDIPlatformConfiguration::setPinEnableState(HashType h, bool v)
{ auto it = _pinEntries.find(h); if (it != _pinEntries.end()) it->second._enabled = v; }
void _FTDIPlatformConfiguration::setPinEnableState(ChipIndex c, Bus b, PinID p, bool v)
{ if (auto* pd = findPin(_pinEntries,c,b,p)) pd->_enabled = v; }

bool _FTDIPlatformConfiguration::getPinInputState(ChipIndex c, Bus b, PinID p) const
{ HashType h = FTDIPinData::makeFTDIHash(c,b,p); auto it = _pinEntries.find(h); return it != _pinEntries.end() ? it->second._input : false; }
void _FTDIPlatformConfiguration::setPinInputState(HashType h, bool v)
{ auto it = _pinEntries.find(h); if (it != _pinEntries.end()) it->second._input = v; }
void _FTDIPlatformConfiguration::setPinInputState(ChipIndex c, Bus b, PinID p, bool v)
{ if (auto* pd = findPin(_pinEntries,c,b,p)) pd->_input = v; }

bool _FTDIPlatformConfiguration::getInitialPinValue(ChipIndex c, Bus b, PinID p) const
{ HashType h = FTDIPinData::makeFTDIHash(c,b,p); auto it = _pinEntries.find(h); return it != _pinEntries.end() ? it->second._initialValue : false; }
void _FTDIPlatformConfiguration::setInitialPinValue(HashType h, bool v)
{ auto it = _pinEntries.find(h); if (it != _pinEntries.end()) it->second._initialValue = v; }
void _FTDIPlatformConfiguration::setInitialPinValue(ChipIndex c, Bus b, PinID p, bool v)
{ if (auto* pd = findPin(_pinEntries,c,b,p)) pd->_initialValue = v; }

int _FTDIPlatformConfiguration::getPinInitializationPriority(ChipIndex c, Bus b, PinID p) const
{ HashType h = FTDIPinData::makeFTDIHash(c,b,p); auto it = _pinEntries.find(h); return it != _pinEntries.end() ? it->second._initializationPriority : -1; }
void _FTDIPlatformConfiguration::setPinInitializationPriority(HashType h, int v)
{ auto it = _pinEntries.find(h); if (it != _pinEntries.end()) it->second._initializationPriority = v; }
void _FTDIPlatformConfiguration::setPinInitializationPriority(ChipIndex c, Bus b, PinID p, int v)
{ if (auto* pd = findPin(_pinEntries,c,b,p)) pd->_initializationPriority = v; }

bool _FTDIPlatformConfiguration::getPinInvertedState(ChipIndex c, Bus b, PinID p) const
{ HashType h = FTDIPinData::makeFTDIHash(c,b,p); auto it = _pinEntries.find(h); return it != _pinEntries.end() ? it->second._inverted : false; }
void _FTDIPlatformConfiguration::setPinInvertedState(HashType h, bool v)
{ auto it = _pinEntries.find(h); if (it != _pinEntries.end()) it->second._inverted = v; }
void _FTDIPlatformConfiguration::setPinInvertedState(ChipIndex c, Bus b, PinID p, bool v)
{ if (auto* pd = findPin(_pinEntries,c,b,p)) pd->_inverted = v; }

std::string _FTDIPlatformConfiguration::getPinLabel(ChipIndex c, Bus b, PinID p) const
{ HashType h = FTDIPinData::makeFTDIHash(c,b,p); auto it = _pinEntries.find(h); return it != _pinEntries.end() ? it->second._pinLabel : ""; }
void _FTDIPlatformConfiguration::setPinLabel(HashType h, const std::string& v)
{ auto it = _pinEntries.find(h); if (it != _pinEntries.end()) it->second._pinLabel = v; }
void _FTDIPlatformConfiguration::setPinLabel(ChipIndex c, Bus b, PinID p, const std::string& v)
{ if (auto* pd = findPin(_pinEntries,c,b,p)) pd->_pinLabel = v; }

std::string _FTDIPlatformConfiguration::getPinTooltip(ChipIndex c, Bus b, PinID p) const
{ HashType h = FTDIPinData::makeFTDIHash(c,b,p); auto it = _pinEntries.find(h); return it != _pinEntries.end() ? it->second._pinTooltip : ""; }
void _FTDIPlatformConfiguration::setPinTooltip(HashType h, const std::string& v)
{ auto it = _pinEntries.find(h); if (it != _pinEntries.end()) it->second._pinTooltip = v; }
void _FTDIPlatformConfiguration::setPinTooltip(ChipIndex c, Bus b, PinID p, const std::string& v)
{ if (auto* pd = findPin(_pinEntries,c,b,p)) pd->_pinTooltip = v; }

std::string _FTDIPlatformConfiguration::getPinCommand(ChipIndex c, Bus b, PinID p) const
{ HashType h = FTDIPinData::makeFTDIHash(c,b,p); auto it = _pinEntries.find(h); return it != _pinEntries.end() ? it->second._pinCommand : ""; }
void _FTDIPlatformConfiguration::setPinCommand(HashType h, const std::string& v)
{ auto it = _pinEntries.find(h); if (it != _pinEntries.end()) it->second._pinCommand = v; }
void _FTDIPlatformConfiguration::setPinCommand(ChipIndex c, Bus b, PinID p, const std::string& v)
{ if (auto* pd = findPin(_pinEntries,c,b,p)) pd->_pinCommand = v; }

CommandGroups _FTDIPlatformConfiguration::getPinGroup(ChipIndex c, Bus b, PinID p) const
{ HashType h = FTDIPinData::makeFTDIHash(c,b,p); auto it = _pinEntries.find(h); return it != _pinEntries.end() ? it->second._commandGroup : eUnknownCommandGroup; }
void _FTDIPlatformConfiguration::setPinGroup(HashType h, CommandGroups v)
{ auto it = _pinEntries.find(h); if (it != _pinEntries.end()) it->second._commandGroup = v; }
void _FTDIPlatformConfiguration::setPinGroup(ChipIndex c, Bus b, PinID p, CommandGroups v)
{ if (auto* pd = findPin(_pinEntries,c,b,p)) pd->_commandGroup = v; }

std::string _FTDIPlatformConfiguration::getTabName(ChipIndex c, Bus b, PinID p) const
{ HashType h = FTDIPinData::makeFTDIHash(c,b,p); auto it = _pinEntries.find(h); return it != _pinEntries.end() ? it->second._tabName : ""; }
void _FTDIPlatformConfiguration::setTabName(HashType h, const std::string& v)
{ auto it = _pinEntries.find(h); if (it != _pinEntries.end()) it->second._tabName = v; }
void _FTDIPlatformConfiguration::setTabName(ChipIndex c, Bus b, PinID p, const std::string& v)
{ if (auto* pd = findPin(_pinEntries,c,b,p)) pd->_tabName = v; }

void _FTDIPlatformConfiguration::getPinCellLocation(ChipIndex c, Bus b, PinID p, int& x, int& y) const
{
    HashType h = FTDIPinData::makeFTDIHash(c,b,p);
    auto it = _pinEntries.find(h);
    if (it != _pinEntries.end()) { x = it->second._cellX; y = it->second._cellY; }
    else { x = -1; y = -1; }
}
void _FTDIPlatformConfiguration::setPinCellLocation(HashType h, int x, int y)
{ auto it = _pinEntries.find(h); if (it != _pinEntries.end()) { it->second._cellX = x; it->second._cellY = y; } }
void _FTDIPlatformConfiguration::setPinCellLocation(ChipIndex c, Bus b, PinID p, int x, int y)
{ if (auto* pd = findPin(_pinEntries,c,b,p)) { pd->_cellX = x; pd->_cellY = y; } }

FTDIBusData _FTDIPlatformConfiguration::getBusFunction(ChipIndex c, Bus b) const
{
    HashType h = FTDIBusData::makeFTDIHash(c,b);
    auto it = _busFunctions.find(h);
    return it != _busFunctions.end() ? it->second : FTDIBusData{};
}
void _FTDIPlatformConfiguration::setBusFunction(HashType h, FTDIBusFunction v)
{ auto it = _busFunctions.find(h); if (it != _busFunctions.end()) it->second._busFunction = v; }
void _FTDIPlatformConfiguration::setBusFunction(ChipIndex c, Bus b, FTDIBusFunction v)
{
    HashType h = FTDIBusData::makeFTDIHash(c,b);
    auto it = _busFunctions.find(h);
    if (it != _busFunctions.end()) it->second._busFunction = v;
}

FTDIPinSets _FTDIPlatformConfiguration::getPinSet(ChipIndex chipIndex)
{
    if (chipIndex < kMaxPinSetCount && chipIndex >= 0)
        return _pinSets[chipIndex];
    return NoOptions;
}

void _FTDIPlatformConfiguration::setPinSet(ChipIndex chipIndex, FTDIPinSets pinSet)
{
    if (chipIndex < kMaxPinSetCount && chipIndex >= 0)
        _pinSets[chipIndex] = pinSet;
}

PinID _FTDIPlatformConfiguration::getSetPinIndex(int chipIndex, Bus bus, PinID pinId)
{
    int busOffset = 0;
    switch (bus)
    {
    case 'A': busOffset = 0;  break;
    case 'B': busOffset = 8;  break;
    case 'C': busOffset = 16; break;
    case 'D': busOffset = 24; break;
    default:  busOffset = 0;  break;
    }
    return static_cast<PinID>(chipIndex * 32 + busOffset + pinId);
}

void _FTDIPlatformConfiguration::cascadeTabDelete(const std::string& deleteMe)
{
    for (auto& kv : _pinEntries)
        if (kv.second._tabName == deleteMe)
            kv.second._tabName = "General";
}

void _FTDIPlatformConfiguration::cascadeTabRename(const std::string& oldName, const std::string& newName)
{
    for (auto& kv : _pinEntries)
        if (kv.second._tabName == oldName)
            kv.second._tabName = newName;
}

// Helper: safely read an integer from JSON that might be stored as string or number
static int jsonInt(const json& val, int defaultVal = 0)
{
    if (val.is_number()) return val.get<int>();
    if (val.is_string())
    {
        try { return std::stoi(val.get<std::string>()); } catch (...) {}
    }
    return defaultVal;
}

bool _FTDIPlatformConfiguration::read(const json& j)
{
    bool result = _PlatformConfiguration::read(j);

    if (j.contains(kChipCount))
        initialize(static_cast<uint16_t>(jsonInt(j[kChipCount], 1)));

    if (j.contains(kPinEntries) && j[kPinEntries].is_array())
    {
        for (const auto& pe : j[kPinEntries])
        {
            int chip = pe.contains(kChipIndex) ? jsonInt(pe[kChipIndex]) : 0;
            Bus bus  = 'A';
            if (pe.contains(kBus) && pe[kBus].is_string() && !pe[kBus].get<std::string>().empty())
                bus = pe[kBus].get<std::string>()[0];
            int pin  = pe.contains(kPinNumber) ? jsonInt(pe[kPinNumber]) : 0;

            HashType h = FTDIPinData::makeFTDIHash(chip, bus, pin);
            auto it = _pinEntries.find(h);
            if (it == _pinEntries.end()) continue;
            auto& pd = it->second;

            if (pe.contains(kEnabled))      pd._enabled      = pe[kEnabled].get<bool>();
            if (pe.contains(kInput))        pd._input        = pe[kInput].get<bool>();
            if (pe.contains(kName))         pd._pinLabel     = pe[kName].is_string() ? pe[kName].get<std::string>() : "";
            if (pe.contains(kToolTip))      pd._pinTooltip   = pe[kToolTip].is_string() ? pe[kToolTip].get<std::string>() : "";
            if (pe.contains(kInitialValue)) pd._initialValue = pe[kInitialValue].get<bool>();
            if (pe.contains(kPriority))     pd._initializationPriority = jsonInt(pe[kPriority], -1);
            if (pe.contains(kInverted))     pd._inverted     = pe[kInverted].get<bool>();
            if (pe.contains(kCommand))      pd._pinCommand   = pe[kCommand].is_string() ? pe[kCommand].get<std::string>() : "";
            if (pe.contains(kCommandGroup)) pd._commandGroup = static_cast<CommandGroups>(jsonInt(pe[kCommandGroup]));
            if (pe.contains(kTabName))      pd._tabName      = pe[kTabName].is_string() ? pe[kTabName].get<std::string>() : "";
            if (pe.contains(kCellLocation))
            {
                std::string cl = pe[kCellLocation].is_string() ? pe[kCellLocation].get<std::string>() : "";
                auto pos = cl.find(',');
                if (pos != std::string::npos)
                {
                    try { pd._cellX = std::stoi(cl.substr(0, pos)); } catch (...) {}
                    try { pd._cellY = std::stoi(cl.substr(pos+1)); } catch (...) {}
                }
            }
        }
    }

    if (j.contains(kBusEntries) && j[kBusEntries].is_array())
    {
        for (const auto& be : j[kBusEntries])
        {
            int chip = be.contains(kChipIndex) ? jsonInt(be[kChipIndex]) : 0;
            Bus bus  = be.contains(kBus) && !be[kBus].get<std::string>().empty()
                       ? be[kBus].get<std::string>()[0] : 'A';
            HashType h = FTDIBusData::makeFTDIHash(chip, bus);
            auto it = _busFunctions.find(h);
            if (it == _busFunctions.end()) continue;
            if (be.contains(kBusFunction))
                it->second._busFunction = FTDIBusData::fromString(be[kBusFunction].is_string() ? be[kBusFunction].get<std::string>() : "");
        }
    }

    return result;
}

void _FTDIPlatformConfiguration::write(json& j)
{
    _PlatformConfiguration::write(j);

    j[kChipCount] = _chipCount;

    json pinList = json::array();
    for (const auto& kv : _pinEntries)
    {
        const auto& pd = kv.second;
        json p;
        p[kChipIndex]   = pd._chipIndex;
        p[kBus]         = std::string(1, pd._bus);
        p[kPinNumber]   = static_cast<int>(pd._chipPin);
        p[kEnabled]     = pd._enabled;
        p[kInput]       = pd._input;
        p[kName]        = pd._pinLabel;
        p[kToolTip]     = pd._pinTooltip;
        p[kInitialValue]= pd._initialValue;
        p[kPriority]    = pd._initializationPriority;
        p[kInverted]    = pd._inverted;
        p[kCommand]     = pd._pinCommand;
        p[kCommandGroup]= static_cast<int>(pd._commandGroup);
        p[kTabName]     = pd._tabName;
        p[kCellLocation]= std::to_string(pd._cellX) + "," + std::to_string(pd._cellY);
        pinList.push_back(p);
    }
    j[kPinEntries] = pinList;

    json busList = json::array();
    for (const auto& kv : _busFunctions)
    {
        const auto& bd = kv.second;
        json b;
        b[kChipIndex]  = bd._chipIndex;
        b[kBus]        = std::string(1, bd._bus);
        b[kBusFunction]= FTDIBusData::toString(bd._busFunction);
        busList.push_back(b);
    }
    j[kBusEntries] = busList;
}
