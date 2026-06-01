// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

/*
	Author: Michael Simpson (msimpson@qti.qualcomm.com)
			Biswajit Roy (biswroy@qti.qualcomm.com)
*/

#include "PlatformConfiguration.h"

#include "PIC32CXPlatformConfiguration.h"
#include "ConsoleApplicationEnhancements.h"
#include "FTDIPlatformConfiguration.h"
#include "PSOCPlatformConfiguration.h"
#include "AlpacaScript.h"
#include "AppCore.h"
#include "PlatformID.h"
#include "StringUtilities.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;
namespace fs = std::filesystem;

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

// Helper: safely read a string from JSON that might be stored as number or string
static std::string jsonStr(const json& val, const std::string& defaultVal = "")
{
    if (val.is_string()) return val.get<std::string>();
    if (val.is_number_integer()) return std::to_string(val.get<int64_t>());
    if (val.is_number_float()) return std::to_string(val.get<double>());
    if (val.is_boolean()) return val.get<bool>() ? "true" : "false";
    return defaultVal;
}

// JSON key constants
static const char* kName                  = "name";
static const char* kAuthor                = "author";
static const char* kDescription           = "description";
static const char* kPlatformType          = "platform_type";
static const char* kFileVersion           = "fileVersion";
static const char* kPineVersion           = "pineVersion";
static const char* kPlatformId            = "platform_id";
static const char* kCreationDate          = "creation_date";
static const char* kModifyDate            = "modification_date";
static const char* kUSBDescriptor         = "usb_descriptor";
static const char* kResetEnabled          = "reset_enabled";
static const char* kFormDimension         = "form_dimension";
static const char* kTabs                  = "tabs";
static const char* kUserTab               = "user_tab";
static const char* kMoveable              = "moveable";
static const char* kVisible               = "visible";
static const char* kConfigurable          = "configurable";
static const char* kOrdinal               = "ordinal";
static const char* kButtons               = "buttons";
static const char* kCommand               = "command";
static const char* kTab                   = "tab";
static const char* kCommandGroup          = "command_group";
static const char* kCellLocation          = "cellLocation";
static const char* kTooltip               = "tooltip";
static const char* kScriptVariables       = "variables";
static const char* kDefaultScriptVarLabel = "label";
static const char* kDefaultScriptVarType  = "type";
static const char* kDefaultScriptDefaultValue = "default_value";
static const char* kScript                = "script";

static const std::string kDefaultLabel   {"<type a label name>"};
static const std::string kDefaultTooltip {"<add a tooltip>"};
static const std::string kDefaultCommand {"<type a command>"};
static const std::string kDefaultTab     {"General"};
static const std::string kNoVariableValue{"<no value>"};

static std::string currentDateTimeStr()
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

static std::string toLowerStr(const std::string& s)
{
    std::string r;
    for (auto c : s) r += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

static std::string cellLocationToStr(int x, int y)
{
    return std::to_string(x) + "," + std::to_string(y);
}

static void strToCellLocation(const std::string& s, int& x, int& y)
{
    x = -1; y = -1;
    auto pos = s.find(',');
    if (pos != std::string::npos)
    {
        try { x = std::stoi(s.substr(0, pos)); } catch (...) {}
        try { y = std::stoi(s.substr(pos + 1)); } catch (...) {}
    }
}

// Static member definitions
std::string         _PlatformConfiguration::_lastError;
Buttons             _PlatformConfiguration::_classicButtons;
bool                _PlatformConfiguration::_dynamicConfigurationsInitialized{false};
TACPlatformEntries  _PlatformConfiguration::_tacPlatformEntries;
USBDescriptors      _PlatformConfiguration::_usbDescriptors;

_PlatformConfiguration::_PlatformConfiguration()
{
    initialize();
    _buttons = _PlatformConfiguration::_classicButtons;
    defaultAlpacaScript();
    defaultScriptVariables();
    _creationDate = currentDateTimeStr();
    _modifyDate   = currentDateTimeStr();
}

_PlatformConfiguration::~_PlatformConfiguration()
{
}

PlatformConfiguration _PlatformConfiguration::createPlatformConfiguration(DebugBoardType debugBoardType, int chipCount)
{
    switch (debugBoardType)
    {
    case ePSOC:      return std::make_shared<_PSOCPlatformConfiguration>();
    case eFTDI:      return std::make_shared<_FTDIPlatformConfiguration>(static_cast<uint16_t>(chipCount));
    case ePIC32CXAuto: return std::make_shared<_PIC32CXPlatformConfiguration>();
    default:         return {};
    }
}

PlatformConfiguration _PlatformConfiguration::openPlatformConfiguration(const std::string& filePath)
{
    if (!fs::is_regular_file(filePath))
        return {};

    std::string fileName = fs::path(filePath).filename().string();
    std::string lower = toLowerStr(fileName);

    PlatformConfiguration result;
    if (lower.find("_psoc_") != std::string::npos)
        result = std::make_shared<_PSOCPlatformConfiguration>();
    else if (lower.find("_ftdi_") != std::string::npos)
        result = std::make_shared<_FTDIPlatformConfiguration>(0);
    else if (lower.find("_pic32cxauto_") != std::string::npos)
        result = std::make_shared<_PIC32CXPlatformConfiguration>();
    else
        _lastError = "Unknown Platform";

    if (result)
        result->load(filePath);

    return result;
}

std::vector<PlatformID> _PlatformConfiguration::platformEntryIds()
{
    std::vector<PlatformID> result;
    for (const auto& kv : _tacPlatformEntries)
        result.push_back(kv.first);
    return result;
}

TACPlatformEntry _PlatformConfiguration::getEntry(PlatformID platformID)
{
    initializeDynamicPlatform();
    auto it = _tacPlatformEntries.find(platformID);
    if (it != _tacPlatformEntries.end())
        return it->second;
    return {};
}

void _PlatformConfiguration::initializeDynamicPlatform()
{
    if (!_dynamicConfigurationsInitialized)
    {
        PlatformContainer::initialize();
        for (const auto& platformID : PlatformContainer::getEntries())
        {
            TACPlatformEntry entry;
            entry._platformEntry = platformID;
            _tacPlatformEntries[platformID->_platformID] = entry;
        }
        _dynamicConfigurationsInitialized = true;
    }
}

std::string _PlatformConfiguration::makeConfigName(const std::string& chip, PlatformID platformID)
{
    return "TAC_" + chip + "_" + std::to_string(platformID) + ".tcnf";
}

bool _PlatformConfiguration::parseConfigName(const std::string& fileName, DebugBoardType& debugBoardType, PlatformID& platformID)
{
    fs::path p(fileName);
    if (toLowerStr(p.extension().string()) != ".tcnf")
        return false;

    std::string base = toLowerStr(p.stem().string());
    // split by '_'
    std::vector<std::string> parts;
    std::istringstream ss(base);
    std::string part;
    while (std::getline(ss, part, '_'))
        if (!part.empty()) parts.push_back(part);

    if (parts.size() < 3 || parts[0] != "tac")
        return false;

    debugBoardType = debugBoardTypeFromString(parts[1]);
    try { platformID = static_cast<PlatformID>(std::stoul(parts[2])); }
    catch (...) { return false; }
    return true;
}

PlatformID _PlatformConfiguration::getUSBDescriptor(const std::string& usbDescriptorString)
{
    std::string lower = toLowerStr(usbDescriptorString);
    for (const auto& kv : _tacPlatformEntries)
    {
        if (toLowerStr(kv.second._platformEntry->_usbDescriptor) == lower)
            return kv.second._platformEntry->_platformID;
    }
    if (usbDescriptorString.substr(0, 12) == "ALPACA-LITE ")
        return ALPACA_LITE_ID;
    return 0;
}

bool _PlatformConfiguration::containsUSBDescriptor(const std::string& usbDescriptorString)
{
    if (usbDescriptorString.empty())
        return false;

    initializeDynamicPlatform();
    std::string lower = toLowerStr(usbDescriptorString);
    for (const auto& kv : _tacPlatformEntries)
    {
        if (!kv.second._platformEntry) continue;
        if (kv.second._platformEntry->_usbDescriptor.empty()) continue;
        if (toLowerStr(kv.second._platformEntry->_usbDescriptor) == lower)
            return true;
    }
    return false;
}

std::string _PlatformConfiguration::lastError()
{
    std::string result = _lastError;
    _lastError.clear();
    return result;
}

void _PlatformConfiguration::setName(const std::string& name)
{
    if (_name != name) { _name = name; _dirty = true; }
}

void _PlatformConfiguration::setAuthor(const std::string& author)
{
    if (_author != author) { _author = author; _dirty = true; }
}

void _PlatformConfiguration::setDescription(const std::string& description)
{
    if (_description != description) { _description = description; _dirty = true; }
}

void _PlatformConfiguration::setPlatform(DebugBoardType platform)
{
    if (_platform != platform) { _platform = platform; _dirty = true; }
}

void _PlatformConfiguration::setPlatform(const std::string& platform)
{
    std::string lower = toLowerStr(platform);
    if (lower == "psoc")                    _platform = ePSOC;
    else if (lower == "ftdi")               _platform = eFTDI;
    else if (lower == "pic32cx (automotive)") _platform = ePIC32CXAuto;
}

void _PlatformConfiguration::deleteTabs(Tabs& tabs)
{
    for (const auto& tab : tabs)
        cascadeTabDelete(tab._name);
}

void _PlatformConfiguration::updateTabs(Tabs& tabs)
{
    _tabs.clear();
    for (const auto& tab : tabs)
        if (tab._userTab && !tab._newText.empty() && tab._newText != tab._name)
            cascadeTabRename(tab._name, tab._newText);
    for (const auto& tab : tabs)
        if (!tab._deleted)
            _tabs.push_back(tab);
    _dirty = true;
}

PlatformID _PlatformConfiguration::getPlatformId() { return _platformId; }

std::string _PlatformConfiguration::getFileVersion() { return std::to_string(_fileVersion); }
uint32_t    _PlatformConfiguration::fileVersion()    { return _fileVersion; }
std::string _PlatformConfiguration::getPineVersion() { return std::to_string(_pineVersion); }
void        _PlatformConfiguration::setPineVersion(uint32_t v) { _pineVersion = v; }
std::string _PlatformConfiguration::getPlatformString() { return debugBoardTypeToString(_platform); }

void _PlatformConfiguration::setPlatformID(PlatformID platformId)
{
    _platformId = platformId;
    _platformFile = makeConfigName(getPlatformString(), _platformId);
}

Tabs _PlatformConfiguration::getTabs() { return _tabs; }

std::vector<std::string> _PlatformConfiguration::getAllTabs()
{
    std::vector<std::string> result;
    for (const auto& tab : _tabs) result.push_back(tab._name);
    return result;
}

std::vector<std::string> _PlatformConfiguration::getVisibleTabs()
{
    std::vector<std::string> result;
    for (const auto& tab : _tabs)
        if (tab._visible) result.push_back(tab._name);
    return result;
}

void _PlatformConfiguration::renameTab(const std::string& oldName, const std::string& newName)
{
    for (auto& tab : _tabs)
        if (tab._name == oldName) { tab._name = newName; break; }
    cascadeTabRename(oldName, newName);
}

void _PlatformConfiguration::setTabVisible(const std::string& tabName, bool visible)
{
    for (auto& tab : _tabs)
        if (tab._name == tabName) { tab._visible = visible; break; }
}

Pins _PlatformConfiguration::getPins() { return {}; }

ButtonList _PlatformConfiguration::getButtons()
{
    ButtonList result;
    for (const auto& kv : _buttons) result.push_back(kv.second);
    std::sort(result.begin(), result.end(), [](const Button& a, const Button& b){
        if (a._tab != b._tab) return a._tab < b._tab;
        return a._commandGroup < b._commandGroup;
    });
    return result;
}

Buttons _PlatformConfiguration::getButtonsMap() { return _buttons; }
ScriptVariables _PlatformConfiguration::getVariables() { return _scriptVariables; }

bool _PlatformConfiguration::setVariableName(const std::string& variableName)
{
    if (variableName == kDefaultVariableName) return false;
    if (!_scriptVariables.count(variableName))
    {
        ScriptVariable sv;
        sv._name = variableName;
        _scriptVariables[variableName] = sv;
    }
    return true;
}

void _PlatformConfiguration::setVariableLabel(const std::string& n, const std::string& v)
{ if (setVariableName(n)) _scriptVariables[n]._label = v; }

void _PlatformConfiguration::setVariableTooltip(const std::string& n, const std::string& v)
{ if (setVariableName(n)) _scriptVariables[n]._tooltip = v; }

void _PlatformConfiguration::setVariableType(const std::string& n, VariableType t)
{ if (setVariableName(n)) _scriptVariables[n]._type = t; }

bool _PlatformConfiguration::setVariableDefaultValue(const std::string& n, const ScriptVariableValue& v)
{
    if (setVariableName(n)) { _scriptVariables[n]._defaultValue = v; return true; }
    return false;
}

void _PlatformConfiguration::setVariableCellLocation(const std::string& n, int x, int y)
{ if (setVariableName(n)) { _scriptVariables[n]._cellX = x; _scriptVariables[n]._cellY = y; } }

void _PlatformConfiguration::deleteVariable(const std::string& n)
{ _scriptVariables.erase(n); }

void _PlatformConfiguration::deleteButton(HashType hash)
{ _buttons.erase(hash); }

// Helper: find or create button by label+tab
static Button& findOrCreateButton(Buttons& buttons, const std::string& label, const std::string& tab, HashType& hash)
{
    Button tmp;
    tmp._label = label;
    tmp._tab   = tab;
    hash = Button::makeHash(tmp);
    for (auto& kv : buttons)
        if (kv.second._hash == hash) return kv.second;
    tmp._hash = hash;
    buttons[hash] = tmp;
    return buttons[hash];
}

HashType _PlatformConfiguration::addButtonLabel(const std::string& label, const std::string& tab)
{
    if (label == kDefaultLabel && tab == kDefaultTab) return 0;
    HashType hash;
    auto& btn = findOrCreateButton(_buttons, label, tab, hash);
    btn._label = label;
    _buttons[btn._hash] = btn;
    return hash;
}

bool _PlatformConfiguration::setButtonLabel(HashType hash, const std::string& label)
{
    for (auto& kv : _buttons)
        if (kv.second._hash == hash) { kv.second._label = label; kv.second._hash = Button::makeHash(kv.second); return true; }
    return false;
}

HashType _PlatformConfiguration::addButtonTooltip(const std::string& label, const std::string& tab, const std::string& tip)
{
    if (label == kDefaultLabel && tab == kDefaultTab && tip == kDefaultTooltip) return 0;
    HashType hash;
    auto& btn = findOrCreateButton(_buttons, label, tab, hash);
    btn._toolTip = tip;
    _buttons[btn._hash] = btn;
    return hash;
}

bool _PlatformConfiguration::setButtonTooltip(HashType hash, const std::string& tip)
{
    for (auto& kv : _buttons)
        if (kv.second._hash == hash) { kv.second._toolTip = tip; return true; }
    return false;
}

HashType _PlatformConfiguration::addButtonCommand(const std::string& label, const std::string& tab, const std::string& cmd)
{
    if (label == kDefaultLabel && tab == kDefaultTab && cmd == kDefaultCommand) return 0;
    HashType hash;
    auto& btn = findOrCreateButton(_buttons, label, tab, hash);
    btn._command = cmd;
    _buttons[btn._hash] = btn;
    return hash;
}

bool _PlatformConfiguration::setButtonCommand(HashType hash, const std::string& cmd)
{
    for (auto& kv : _buttons)
        if (kv.second._hash == hash) { kv.second._command = cmd; return true; }
    return false;
}

HashType _PlatformConfiguration::addButtonCommandGroup(const std::string& label, const std::string& tab, CommandGroups cg)
{
    if (label == kDefaultLabel && tab == kDefaultTab && cg == eUnknownCommandGroup) return 0;
    HashType hash;
    auto& btn = findOrCreateButton(_buttons, label, tab, hash);
    btn._commandGroup = cg;
    _buttons[btn._hash] = btn;
    return hash;
}

bool _PlatformConfiguration::setButtonCommandGroup(HashType hash, CommandGroups cg)
{
    for (auto& kv : _buttons)
        if (kv.second._hash == hash) { kv.second._commandGroup = cg; return true; }
    return false;
}

HashType _PlatformConfiguration::addButtonTab(const std::string& label, const std::string& tab)
{
    if (label == kDefaultLabel && tab == kDefaultTab) return 0;
    HashType hash;
    findOrCreateButton(_buttons, label, tab, hash);
    return hash;
}

bool _PlatformConfiguration::setButtonTab(HashType hash, const std::string& tab)
{
    for (auto& kv : _buttons)
        if (kv.second._hash == hash) { kv.second._tab = tab; kv.second._hash = Button::makeHash(kv.second); return true; }
    return false;
}

HashType _PlatformConfiguration::addButtonCellLocation(const std::string& label, const std::string& tab, int x, int y)
{
    if (label == kDefaultLabel && tab == kDefaultTab && x == -1 && y == -1) return 0;
    HashType hash;
    auto& btn = findOrCreateButton(_buttons, label, tab, hash);
    btn._cellX = x; btn._cellY = y;
    _buttons[btn._hash] = btn;
    return hash;
}

bool _PlatformConfiguration::setButtonCellLocation(HashType hash, int x, int y)
{
    for (auto& kv : _buttons)
        if (kv.second._hash == hash) { kv.second._cellX = x; kv.second._cellY = y; return true; }
    return false;
}

TACSize _PlatformConfiguration::getFormDimension()
{
    if (_formDimension.width > 0 && _formDimension.height > 0)
        return _formDimension;
    return kClassicDimension;
}

void _PlatformConfiguration::setFormDimension(const TACSize& d)
{
    if (d.width > 0 && d.height > 0 && (d.width != kClassicDimension.width || d.height != kClassicDimension.height))
        _formDimension = d;
}

bool _PlatformConfiguration::getResetEnabledState() { return _resetEnabled; }
void _PlatformConfiguration::setResetEnabledState(bool s) { if (s != _resetEnabled) { _dirty = true; _resetEnabled = s; } }

std::string _PlatformConfiguration::getUSBDescriptor() { return _usbDescriptor; }
void _PlatformConfiguration::setUSBDescriptor(const std::string& d) { if (d != _usbDescriptor) { _usbDescriptor = d; _dirty = true; } }

const std::string& _PlatformConfiguration::getAlpacaScript() { return _alpacaScript; }
void _PlatformConfiguration::setAlpacaScript(const std::string& s) { _alpacaScript = s; _dirty = true; }

std::string _PlatformConfiguration::filePath()
{
    if (_platformPath.empty() || _platformFile.empty()) return {};
    return (fs::path(_platformPath) / _platformFile).string();
}

void _PlatformConfiguration::setFilePath(const std::string& fp)
{
    fs::path p(fp);
    std::string absPath = fs::absolute(p).string();
    std::string lower = toLowerStr(absPath);
    if (lower.find("c:/programdata/qualcomm/alpaca") != std::string::npos)
    {
        if ((_platform == ePSOC && _platformId < 255) || (_platform == eFTDI && _platformId < 90000))
        {
            _platformPath = documentsDataPath("TAC Configurations");
            _platformFile = p.filename().string();
            return;
        }
    }
    _platformPath = p.parent_path().string();
    _platformFile = p.filename().string();
}

void _PlatformConfiguration::setSupportedFirmwareVer(const std::vector<uint32_t>& v) { _supportedFirmwareVer = v; }
std::vector<uint32_t> _PlatformConfiguration::supportedFirmwareVer() { return _supportedFirmwareVer; }

bool _PlatformConfiguration::load(const std::string& fp)
{
    std::ifstream file(fp);
    if (!file.is_open())
    {
        _lastError = "Unable to open platform configuration file " + fp;
        return false;
    }

    json doc;
    try { file >> doc; }
    catch (const json::parse_error& e)
    {
        _lastError = std::string("Error parsing configuration file: ") + e.what();
        return false;
    }

    bool result = read(doc);
    if (result) setFilePath(fp);
    return result;
}

void _PlatformConfiguration::save()
{
    _modifyDate = currentDateTimeStr();

    if (_platformPath.empty())
    {
        if (_platform == ePSOC)
            _platformPath = (_platformId < 255) ? documentsDataPath("TAC Configurations") : tacConfigRoot();
        else
            _platformPath = (_platformId < 90000) ? documentsDataPath("TAC Configurations") : tacConfigRoot();
    }

    if (_platformFile.empty())
        _platformFile = makeConfigName(getPlatformString(), _platformId);

    std::string targetPath = (fs::path(_platformPath) / _platformFile).string();

    json doc;
    write(doc);

    std::ofstream file(targetPath);
    if (file.is_open())
    {
        file << doc.dump(4);
        file.close();
        _dirty = false;
        setFilePath(targetPath);
        copyToPineDataPath(targetPath);
    }
}

bool _PlatformConfiguration::read(const json& j)
{
    bool result{false};

    if (j.contains(kName))        _name        = jsonStr(j[kName]);
    if (j.contains(kAuthor))      _author      = jsonStr(j[kAuthor]);
    if (j.contains(kDescription)) _description = jsonStr(j[kDescription]);

    if (j.contains(kPlatformId))
    {
        _platformId = static_cast<PlatformID>(jsonInt(j[kPlatformId]));
        result = true;
    }
    if (j.contains(kPlatformType))
    {
        setPlatform(jsonStr(j[kPlatformType]));
        result = true;
    }
    if (j.contains(kFileVersion))  _fileVersion  = static_cast<uint32_t>(jsonInt(j[kFileVersion]));
    if (j.contains(kPineVersion))  _pineVersion  = static_cast<uint32_t>(jsonInt(j[kPineVersion]));
    if (j.contains(kCreationDate)) _creationDate = jsonStr(j[kCreationDate]);
    if (j.contains(kModifyDate))   _modifyDate   = jsonStr(j[kModifyDate]);
    if (j.contains(kUSBDescriptor))_usbDescriptor= jsonStr(j[kUSBDescriptor]);
    if (j.contains(kResetEnabled)) _resetEnabled = j[kResetEnabled].get<bool>();

    if (j.contains(kFormDimension))
    {
        std::string s = jsonStr(j[kFormDimension]);
        auto pos = s.find(',');
        if (pos != std::string::npos)
        {
            try { _formDimension.width  = std::stoi(s.substr(0, pos)); } catch (...) {}
            try { _formDimension.height = std::stoi(s.substr(pos + 1)); } catch (...) {}
        }
    }

    if (j.contains(kTabs) && j[kTabs].is_array())
    {
        _tabs.clear();
        for (const auto& t : j[kTabs])
        {
            Tab tab;
            if (t.contains(kName))        tab._name        = jsonStr(t[kName]);
            if (t.contains(kUserTab))     tab._userTab     = t[kUserTab].get<bool>();
            if (t.contains(kMoveable))    tab._moveable    = t[kMoveable].get<bool>();
            if (t.contains(kVisible))     tab._visible     = t[kVisible].get<bool>();
            if (t.contains(kConfigurable))tab._configurable= t[kConfigurable].get<bool>();
            if (t.contains(kOrdinal))     tab._ordinal     = jsonInt(t[kOrdinal]);
            tab._hash = Tab::makeHash(tab);
            _tabs.push_back(tab);
        }
    }

    _buttons.clear();
    if (j.contains(kButtons) && j[kButtons].is_array())
    {
        for (const auto& b : j[kButtons])
        {
            Button btn;
            if (b.contains(kName))        btn._label        = jsonStr(b[kName]);
            if (b.contains(kCommand))     btn._command      = jsonStr(b[kCommand]);
            if (b.contains(kCommandGroup))btn._commandGroup = static_cast<CommandGroups>(jsonInt(b[kCommandGroup]));
            if (b.contains(kTab))         btn._tab          = jsonStr(b[kTab]);
            if (b.contains(kTooltip))     btn._toolTip      = jsonStr(b[kTooltip]);
            if (b.contains(kCellLocation))
                strToCellLocation(jsonStr(b[kCellLocation]), btn._cellX, btn._cellY);
            btn._hash = Button::makeHash(btn);
            _buttons[btn._hash] = btn;
        }
    }

    _scriptVariables.clear();
    if (j.contains(kScriptVariables) && j[kScriptVariables].is_array())
    {
        for (const auto& sv : j[kScriptVariables])
        {
            ScriptVariable var;
            if (sv.contains(kName))                   var._name    = jsonStr(sv[kName]);
            if (sv.contains(kDefaultScriptVarLabel))  var._label   = jsonStr(sv[kDefaultScriptVarLabel]);
            if (sv.contains(kTooltip))                var._tooltip = jsonStr(sv[kTooltip]);
            if (sv.contains(kDefaultScriptVarType))   var._type    = static_cast<VariableType>(jsonInt(sv[kDefaultScriptVarType]));
            if (sv.contains(kDefaultScriptDefaultValue))
            {
                try { var._defaultValue = jsonInt(sv[kDefaultScriptDefaultValue]); } catch (...) {}
            }
            if (sv.contains(kCellLocation))
                strToCellLocation(jsonStr(sv[kCellLocation]), var._cellX, var._cellY);
            _scriptVariables[var._name] = var;
        }
    }

    _alpacaScript.clear();
    if (j.contains(kScript)) _alpacaScript = jsonStr(j[kScript]);

    _dirty = false;
    return result;
}

void _PlatformConfiguration::write(json& j)
{
    j[kPlatformId]   = static_cast<int>(_platformId);
    j[kName]         = _name;
    j[kAuthor]       = _author;
    j[kDescription]  = _description;
    j[kFileVersion]  = static_cast<int>(++_fileVersion);
    j[kPineVersion]  = static_cast<int>(_pineVersion);
    j[kCreationDate] = _creationDate;
    j[kModifyDate]   = _modifyDate;
    j[kPlatformType] = debugBoardTypeToString(_platform);
    j[kUSBDescriptor]= _usbDescriptor;
    j[kResetEnabled] = _resetEnabled;
    j[kScript]       = _alpacaScript;

    if (_formDimension.width > 0 && _formDimension.height > 0 &&
        (_formDimension.width != kClassicDimension.width || _formDimension.height != kClassicDimension.height))
        j[kFormDimension] = std::to_string(_formDimension.width) + "," + std::to_string(_formDimension.height);

    json tabList = json::array();
    for (const auto& tab : _tabs)
    {
        json t;
        t[kName]        = tab._name;
        t[kUserTab]     = tab._userTab;
        t[kMoveable]    = tab._moveable;
        t[kVisible]     = tab._visible;
        t[kConfigurable]= tab._configurable;
        t[kOrdinal]     = tab._ordinal;
        tabList.push_back(t);
    }
    j[kTabs] = tabList;

    json btnList = json::array();
    for (const auto& kv : _buttons)
    {
        const auto& btn = kv.second;
        json b;
        b[kName]        = btn._label;
        b[kCommand]     = btn._command;
        b[kCommandGroup]= static_cast<int>(btn._commandGroup);
        b[kTab]         = btn._tab;
        b[kTooltip]     = btn._toolTip;
        b[kCellLocation]= cellLocationToStr(btn._cellX, btn._cellY);
        btnList.push_back(b);
    }
    j[kButtons] = btnList;

    json varList = json::array();
    for (const auto& kv : _scriptVariables)
    {
        const auto& sv = kv.second;
        json v;
        v[kName]                    = sv._name;
        v[kDefaultScriptVarLabel]   = sv._label;
        v[kTooltip]                 = sv._tooltip;
        v[kDefaultScriptVarType]    = static_cast<int>(sv._type);
        v[kCellLocation]            = cellLocationToStr(sv._cellX, sv._cellY);
        if (std::holds_alternative<int>(sv._defaultValue))
            v[kDefaultScriptDefaultValue] = std::get<int>(sv._defaultValue);
        else
            v[kDefaultScriptDefaultValue] = 0;
        varList.push_back(v);
    }
    j[kScriptVariables] = varList;
}

void _PlatformConfiguration::initialize()
{
    initializeDynamicPlatform();

    if (_classicButtons.empty())
    {
        auto addBtn = [&](const std::string& label, const std::string& tip, const std::string& cmd, int x, int y)
        {
            Button btn;
            btn._label        = label;
            btn._tab          = "General";
            btn._cellX        = x;
            btn._cellY        = y;
            btn._commandGroup = eQuickSettingsGroup;
            btn._toolTip      = tip;
            btn._command      = cmd;
            btn._hash         = Button::makeHash(btn);
            _classicButtons[btn._hash] = btn;
        };

        addBtn("Power On",            "Powers on the MTP/Device",                    "powerOn",           0, 0);
        addBtn("Power Off",           "Powers off the MTP/Device",                   "powerOff",          1, 0);
        addBtn("Boot to EDL",         "Boots the device to emergency download",       "bootToEDL",         2, 0);
        addBtn("Boot to Fastboot",    "Boots the device to fastboot",                "bootToFastboot",    0, 1);
        addBtn("Boot to UEFI",        "Boots the device to UEFI Menu",               "bootToUEFI",        1, 1);
        addBtn("Boot to Secondary EDL","Boots the device to secondary emergency download","bootToSecondaryEDL",2, 1);
    }
}

void _PlatformConfiguration::copyToPineDataPath(const std::string& savePath)
{
#ifdef _WIN32
    std::string pinePath = "C:/Program Files (x86)/Qualcomm/Shared/Alpaca";
    fs::create_directories(pinePath);
    std::string fileName = fs::path(savePath).filename().string();
    std::string dest = (fs::path(pinePath) / fileName).string();
    if (fs::exists(dest)) fs::remove(dest);
    fs::copy_file(savePath, dest, fs::copy_options::overwrite_existing);
#else
    (void)savePath;
#endif
}

void _PlatformConfiguration::defaultAlpacaScript()
{
    _alpacaScript = AlpacaScript::defaultScript();
}

void _PlatformConfiguration::defaultScriptVariables()
{
    if (!_scriptVariables.empty()) return;

    auto addVar = [&](const std::string& name, const std::string& label, const std::string& tip, int defVal, int x, int y)
    {
        ScriptVariable v;
        v._name         = name;
        v._label        = label;
        v._tooltip      = tip;
        v._type         = eIntegerType;
        v._defaultValue = defVal;
        v._cellX        = x;
        v._cellY        = y;
        _scriptVariables[name] = v;
    };

    addVar("edl",      "EDL timing (ms)",      "Configurable Boot to EDL timing in milliseconds",      1300, 0, 0);
    addVar("uefi",     "UEFI timing (ms)",     "Configurable Boot to UEFI timing in milliseconds",     8000, 0, 1);
    addVar("fastboot", "Fastboot timing (ms)", "Configurable Boot to fastboot timing in milliseconds", 8000, 1, 0);
}

PlatformConfiguration TACPlatformEntry::getConfiguration()
{
    if (!_platformConfiguration && _platformEntry)
    {
        const std::string& path = _platformEntry->_path;
        if (!path.empty())
        {
            _platformConfiguration = _PlatformConfiguration::openPlatformConfiguration(path);
            AppCore::writeToApplicationLog(
                _platformConfiguration
                    ? "TACPlatformEntry::getConfiguration() Platform " + path + " loaded\n"
                    : "TACPlatformEntry::getConfiguration() Platform " + path + " load failed\n");
        }
        else
        {
            _platformConfiguration = _PlatformConfiguration::createPlatformConfiguration(_platformEntry->_boardtype, 1);
            AppCore::writeToApplicationLog(
                _platformConfiguration
                    ? "TACPlatformEntry::getConfiguration() Platform created\n"
                    : "TACPlatformEntry::getConfiguration() Platform create failed\n");
        }

        // Copy pin sets from the PlatformEntry to the configuration.
        // When no .tcnf file is loaded (configPath empty), the default-created
        // configuration has pinSets=0. The PlatformEntry (from devicelist.json
        // or hardcoded defaults) has the correct pin set assignments.
        if (_platformConfiguration && _platformEntry->_boardtype == eFTDI)
        {
            auto* ftdiConfig = static_cast<_FTDIPlatformConfiguration*>(_platformConfiguration.get());
            for (int i = 0; i < 4; ++i)
            {
                if (_platformEntry->_pinSets[i] != 0)
                    ftdiConfig->setPinSet(i, _platformEntry->_pinSets[i]);
            }
        }
    }
    return _platformConfiguration;
}

void _PlatformConfiguration::cascadeTabDelete(const std::string& /*deleteMe*/) {}
void _PlatformConfiguration::cascadeTabRename(const std::string& /*oldName*/, const std::string& /*newName*/) {}
