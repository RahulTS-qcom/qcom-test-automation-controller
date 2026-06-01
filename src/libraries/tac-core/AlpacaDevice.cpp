// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

/*
	Author: Michael Simpson (msimpson@qti.qualcomm.com)
			Biswajit Roy (biswroy@qti.qualcomm.com)
*/

#include "AlpacaDevice.h"
#include "TACException.h"
#include "FTDIDevice.h"
#include "PSOCDevice.h"
#include "PIC32CXDevice.h"
#include "AppCore.h"
#include "StringUtilities.h"

#include <algorithm>
#include <cctype>
#include <thread>

static const std::string kSendCommandError   {"sendCommand error: Attempted operation on inactive device"};
static const std::string kSetPinError        {"setPinState error: Attempted operation on inactive device"};
static const std::string kQuickCommandError  {"quickCommand error: Attempted operation on inactive device"};
static const std::string kCommandStatusError {"commandQueueStatus error: Attempted operation on inactive device"};

static std::string strToLower(const std::string& s)
{
    std::string r;
    for (auto c : s) r += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

std::mutex    _AlpacaDevice::_mutex;
AlpacaDevices _AlpacaDevice::_alpacaDevices;

_AlpacaDevice::~_AlpacaDevice()
{
    if (isOpen()) close();
}

void _AlpacaDevice::getAlpacaDevices(AlpacaDevices& alpacaDevices, DebugBoardType debugBoardTypeFilter)
{
    alpacaDevices.clear();
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& d : _alpacaDevices)
    {
        if (!d->active()) continue;
        if (debugBoardTypeFilter == eUnknownDebugBoard || d->debugBoardType() == debugBoardTypeFilter)
            alpacaDevices.push_back(d);
    }
}

uint32_t _AlpacaDevice::updateAlpacaDevices()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _alpacaDevices.clear();
    PSOCDevice::updateAlpacaDevices();
    FTDIDevice::updateAlpacaDevices();
    PIC32CXDevice::updateAlpacaDevices();
    return static_cast<uint32_t>(_alpacaDevices.size());
}

AlpacaDevice _AlpacaDevice::findAlpacaDevice(HashType hash)
{
    for (const auto& d : _alpacaDevices)
        if (d->_hash == hash) return d;
    return {};
}

AlpacaDevice _AlpacaDevice::findAlpacaDevice(const std::string& portName)
{
    std::string search = strToLower(portName);
    for (const auto& d : _alpacaDevices)
    {
        if (strToLower(d->_portName).find(search) != std::string::npos) return d;
        if (strToLower(d->_serialNumber).find(search) != std::string::npos) return d;
    }
    return {};
}

AlpacaDevice _AlpacaDevice::findAlpacaDeviceBySerialNumber(const std::string& serialNumber, bool usePartial)
{
    std::string test = strToLower(serialNumber);
    for (const auto& d : _alpacaDevices)
    {
        std::string sn = strToLower(d->_serialNumber);
        if (usePartial ? (test.find(sn) != std::string::npos) : (test == sn))
            return d;
    }
    return {};
}

AlpacaDevice _AlpacaDevice::findAlpacaDeviceByDescription(const std::string& description, bool usePartial)
{
    std::string test = strToLower(description);
    for (const auto& d : _alpacaDevices)
    {
        std::string desc = strToLower(d->description());
        if (usePartial ? (test.find(desc) != std::string::npos) : (test == desc))
            return d;
    }
    return {};
}

AlpacaDevice _AlpacaDevice::findAlpacaDeviceByUSBDescriptor(const std::string& descriptor, bool usePartial)
{
    std::string test = strToLower(descriptor);
    for (const auto& d : _alpacaDevices)
    {
        std::string usbDesc = strToLower(d->_usbDescriptor);
        if (usePartial ? (test.find(usbDesc) != std::string::npos) : (test == usbDesc))
            return d;
    }
    return {};
}

std::string _AlpacaDevice::getLastError()
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::string temp = _lastError;
    _lastError.clear();
    return temp;
}

bool _AlpacaDevice::isOpen()
{
    return _driveThread != nullptr;
}

void _AlpacaDevice::close()
{
    if (_driveThread)
    {
        // If close() is called FROM the drive thread itself (e.g., via
        // onDeviceDisconnected callback → onDeviceDisconnect → close),
        // joining would deadlock.
        //
        // IMPORTANT: In TACLiteDriveThread::run(), onDeviceDisconnected fires
        // at line 253 (discovery failure) and line 346 (end-of-run). In both
        // cases, run() still accesses member variables AFTER the callback
        // (logging at lines 256-258, shutdownLogging at 348). Therefore we
        // CANNOT delete the object while the drive thread is still on the stack.
        //
        // Safe behavior: if called from the drive thread, signal stop and
        // mark inactive but do NOT delete. The thread will exit run() naturally
        // and the next external close() or destructor will clean up.
        bool calledFromDriveThread =
            (std::this_thread::get_id() == _driveThread->threadId());

        if (calledFromDriveThread)
        {
            // Signal the thread to stop. Do NOT delete — run() still has
            // work to do after the callback returns (logging, cleanup).
            // The thread will exit naturally. Mark inactive so the API
            // layer knows this device is no longer usable.
            _driveThread->stopRunning();
            _active = false;
            return;  // Do NOT delete — thread stack still references this object
        }

        // Normal case: called from API thread. Stop + join + reset.
        _driveThread->shutDown();
        _driveThread.reset();
    }
}

void _AlpacaDevice::buildQuickSettings()
{
    if (_alpacaScript.count() == 0)
    {
        const std::string& alpacaScript = _platformConfiguration->getAlpacaScript();
        ScriptVariables scriptVariables = _platformConfiguration->getVariables();
        _alpacaScript.parseScript(alpacaScript, scriptVariables, commandList());
    }
}

uint32_t _AlpacaDevice::commandCount()
{
    commandList();
    return static_cast<uint32_t>(_commandList.size());
}

TACCommand _AlpacaDevice::commandEntry(uint32_t commandIndex)
{
    if (commandIndex < _commandList.size())
        return _commandList[commandIndex];
    return {};
}

TACCommands _AlpacaDevice::commandList()
{
    std::lock_guard<std::mutex> lock(_commandsMutex);
    if (_commandList.empty())
    {
        for (const auto& kv : _commands)
            _commandList.push_back(kv.second);
        std::stable_sort(_commandList.begin(), _commandList.end(),
            [](const TACCommand& a, const TACCommand& b){ return a._command < b._command; });
    }
    return _commandList;
}

uint32_t _AlpacaDevice::quickCommandCount()
{
    if (_quickCommandList.empty())
    {
        ButtonList buttons = _platformConfiguration->getButtons();
        for (const auto& btn : buttons)
        {
            std::string qc = btn._label + ";" + btn._command + ";" + btn._toolTip
                           + ";" + std::to_string(btn._cellX) + "," + std::to_string(btn._cellY);
            _quickCommandList.push_back(qc);
        }
    }
    return static_cast<uint32_t>(_quickCommandList.size());
}

std::string _AlpacaDevice::getQuickCommand(uint32_t index)
{
    if (index < _quickCommandList.size())
        return _quickCommandList[index];
    return {};
}

uint32_t _AlpacaDevice::scriptVariableCount()
{
    return static_cast<uint32_t>(_platformConfiguration->getVariables().size());
}

std::string _AlpacaDevice::getScriptVariable(uint32_t index)
{
    ScriptVariables vars = _platformConfiguration->getVariables();
    std::vector<ScriptVariable> varList;
    for (const auto& kv : vars) varList.push_back(kv.second);

    if (index < varList.size())
    {
        const auto& v = varList[index];
        std::string defVal;
        if (std::holds_alternative<int>(v._defaultValue))
            defVal = std::to_string(std::get<int>(v._defaultValue));

        return v._name + ";" + v._label + ";" + v._tooltip + ";"
             + variableTypeToString(v._type) + ";" + defVal + ";"
             + std::to_string(v._cellX) + "," + std::to_string(v._cellY);
    }
    return {};
}

bool _AlpacaDevice::updateScriptVariableValue(const std::string& scriptVariable, const std::string& value)
{
    ScriptVariables vars = _platformConfiguration->getVariables();
    if (vars.count(scriptVariable))
    {
        try { return _platformConfiguration->setVariableDefaultValue(scriptVariable, std::stoi(value)); }
        catch (...) { return _platformConfiguration->setVariableDefaultValue(scriptVariable, 0); }
    }
    AppCore::writeToApplicationLogLine("[updateScriptVariableValue()]: The variable '" + scriptVariable + "' was not found.");
    throw TACException(TAC_SCRIPT_VARIABLE_NOT_FOUND, "Script variable " + scriptVariable + " not Found.");
}

bool _AlpacaDevice::getCommandState(const std::string& command)
{
    std::lock_guard<std::mutex> lock(_commandsMutex);
    auto it = _commands.find(command);
    if (it != _commands.end()) return it->second._currentState;
    AppCore::writeToApplicationLogLine("getCommandState " + command + " is not found.");
    throw TACException(TAC_COMMAND_NOT_FOUND, "Command " + command + " not Found.");
}

bool _AlpacaDevice::sendCommand(const std::string& command, bool state)
{
    bool result{false};
    if (_driveThread != nullptr)
    {
        PinID pin{static_cast<PinID>(-1)};
        {
            std::lock_guard<std::mutex> lock(_commandsMutex);
            auto it = _commands.find(command);
            if (it != _commands.end())
                pin = it->second._pin;
        }

        if (pin != static_cast<PinID>(-1))
        {
            setPinState(pin, state);
            result = true;
        }
        else
        {
            result = quickCommand(command);
        }

        if (_driveThread != nullptr)
            _driveThread->clearWaitForCompletion();

        if (!result)
        {
            AppCore::writeToApplicationLogLine(command + " is not found in Alpaca Script.");
            throw TACException(TAC_COMMAND_NOT_FOUND, "sendCommand error: Command " + command + " not found.");
        }
        else if (!active())
        {
            AppCore::writeToApplicationLogLine("_AlpacaDevice::sendCommand(" + command + ") failed. Inactive device");
            throw TACException(TAC_DEVICE_INACTIVE, kSendCommandError);
        }
    }
    return result;
}

bool _AlpacaDevice::quickCommand(const std::string& command)
{
    bool result{false};
    if (_driveThread != nullptr)
    {
        if (active())
        {
            if (_alpacaScript.hasCommand(command))
            {
                ScriptVariables vars = _platformConfiguration->getVariables();
                CommandEntries entries = _alpacaScript.getCommandEntries(command);
                CommandEntries substituted = _alpacaScript.replaceTokens(vars, entries);
                _driveThread->sendCommandSequence(substituted);
                result = true;
            }
            else
            {
                AppCore::writeToApplicationLogLine(command + " not found in Alpaca Script.");
                auto cmds = _alpacaScript.availableCommands();
                for (const auto& c : cmds)
                    AppCore::writeToApplicationLogLine("   " + c);
            }
        }
        else
        {
            AppCore::writeToApplicationLogLine("_AlpacaDevice::quickCommand(" + command + ") failed. Inactive device");
            throw TACException(TAC_DEVICE_INACTIVE, kQuickCommandError);
        }
    }
    return result;
}

bool _AlpacaDevice::isCommandQueueClear()
{
    if (_driveThread != nullptr)
    {
        if (active()) return _driveThread->waitForCompletionStatus();
    }
    else
    {
        AppCore::writeToApplicationLogLine("_AlpacaDevice::isCommandQueueClear() failed. Inactive device");
        throw TACException(TAC_DEVICE_INACTIVE, kCommandStatusError);
    }
    return false;
}

std::string _AlpacaDevice::getHelp()
{
    if (_helpText.empty())
    {
        std::string result;
        result += "Name: " + _platformConfiguration->name() + "\n\n";
        result += "Platform: " + _platformConfiguration->getPlatformString() + "\n";
        result += "Author: " + _platformConfiguration->author() + "\n";
        result += "Description: " + _platformConfiguration->description() + "\n";
        result += "Modification Date: " + _platformConfiguration->modificationDate() + "\n";

        Pins pins = _platformConfiguration->getPins();
        if (!pins.empty())
        {
            result += "\nCommands\n\n";
            for (const auto& pin : pins)
            {
                if (pin._cellX != -1 || pin._cellY != -1)
                    result += "\t" + pin._pinLabel + " Command: " + pin._pinCommand
                            + "(" + std::to_string(pin._pin) + "), " + pin._pinTooltip + "\n";
            }
        }

        ButtonList buttons = _platformConfiguration->getButtons();
        if (!buttons.empty())
        {
            result += "\nQuick Commands\n\n";
            for (const auto& btn : buttons)
                result += "\t" + btn._label + " Command: " + btn._command + ", " + btn._toolTip + "\n";
        }

        _helpText = result;
    }
    return _helpText;
}

void _AlpacaDevice::setPinState(PinID pin, bool state)
{
    if (_driveThread != nullptr)
    {
        if (active())
        {
            _driveThread->setPinState(pin, state);
            if (AppCore::getAppCore()->appLoggingActive())
            {
                TACCommand cmd = TACCommand::find(pin, _commandList);
                AppCore::writeToApplicationLogLine("_AlpacaDevice::setPinState(" + std::to_string(pin) + ") Command:" + cmd._command);
            }
        }
        else
        {
            AppCore::writeToApplicationLogLine("_AlpacaDevice::setPinState(" + std::to_string(pin) + ") failed. Inactive device");
            throw TACException(TAC_DEVICE_INACTIVE, kSetPinError);
        }
    }
    else
    {
        AppCore::writeToApplicationLogLine("_AlpacaDevice::setPinState _driveThread is NULL");
    }
}

void _AlpacaDevice::setWaitForCompletion()
{
    if (_driveThread != nullptr) _driveThread->setWaitForCompletion();
}

bool     _AlpacaDevice::active()    { return _active; }
HashType _AlpacaDevice::hash()      { return _hash; }

std::string _AlpacaDevice::description() const
{
    if (_driveThread) return _driveThread->description();
    return _description;
}

std::string _AlpacaDevice::usbDescriptor()  { return _usbDescriptor; }

std::string _AlpacaDevice::serialNumber() const
{
    if (_driveThread) return _driveThread->serialNumber();
    return _serialNumber;
}

PlatformID  _AlpacaDevice::platformID()     { return _platformID; }

std::string _AlpacaDevice::macAddress()
{
    if (_driveThread) return _driveThread->macAddress();
    return {};
}

PlatformConfiguration _AlpacaDevice::platformConfiguration() { return _platformConfiguration; }

DebugBoardType _AlpacaDevice::debugBoardType()
{
    if (_boardType == eUnknownDebugBoard && _driveThread)
        _boardType = _driveThread->debugBoardType();
    return _boardType;
}

std::string _AlpacaDevice::debugBoardTypeString()
{
    if (_driveThread) return _driveThread->debugBoardTypeString();
    return {};
}

std::string _AlpacaDevice::hardwareVersionString()
{
    if (_driveThread) return _driveThread->hardwareVersionString();
    return {};
}

std::string _AlpacaDevice::firmwareVersion()
{
    if (_driveThread) return _driveThread->firmwareVersion();
    return {};
}

uint32_t _AlpacaDevice::majorVersion()   { return _driveThread ? _driveThread->majorVersion()   : 0; }
uint32_t _AlpacaDevice::minorVersion()   { return _driveThread ? _driveThread->minorVersion()   : 0; }
uint32_t _AlpacaDevice::revisionVersion(){ return _driveThread ? _driveThread->revisionVersion(): 0; }

std::string _AlpacaDevice::chipVersion()
{
    switch (_chipVersion)
    {
    case 3:     return "LP038";
    case 4:     return "LP030";
    case 10000: return "FTDI";
    default:    return "None";
    }
}

void _AlpacaDevice::externalPowerControl(bool state)
{
    PinID pin{static_cast<PinID>(-1)};
    {
        std::lock_guard<std::mutex> lock(_commandsMutex);
        auto it = _commands.find("extpower");
        if (it != _commands.end())
            pin = it->second._pin;
    }
    if (pin != static_cast<PinID>(-1))
        setPinState(pin, state);
    else
        throw TACException(TAC_COMMAND_NOT_FOUND, "Command extpower not Found.");
}

std::string _AlpacaDevice::name() const
{
    if (_driveThread) return _driveThread->name();
    return _serialNumber;
}

void _AlpacaDevice::setName(const std::string& newName)
{
    if (_driveThread) _driveThread->setName(newName);
}

TACWindowSize _AlpacaDevice::windowDimension()
{
    if (_platformConfiguration)
    {
        TACSize s = _platformConfiguration->getFormDimension();
        return {s.width, s.height};
    }
    return {560, 750};
}

std::string _AlpacaDevice::portName() const { return _portName; }

void _AlpacaDevice::setPortName(const std::string& portName)
{
    _portName = portName;
    if (_driveThread) _driveThread->setPortName(portName);
}

void _AlpacaDevice::setDescription(const std::string& description)
{
    if (_driveThread) _driveThread->setDescription(description);
}

void _AlpacaDevice::setSerialNumber(const std::string& serialNumber)
{
    _serialNumber = serialNumber;
    if (_driveThread) _driveThread->setSerialNumber(serialNumber);
}

std::string _AlpacaDevice::uuid()
{
    if (_driveThread) return _driveThread->uuid();
    return {};
}

void _AlpacaDevice::on_pinStateChanged(uint64_t pin, bool state)
{
    {
        std::lock_guard<std::mutex> lock(_commandsMutex);
        for (auto& kv : _commands)
        {
            if (kv.second._pin == pin)
            {
                kv.second._currentState = state;
                break;
            }
        }
    }
    // Forward to registered callback (replaces emit pinStateChanged)
    // Fired outside the lock to avoid holding it during user callback execution
    if (onPinStateChanged) onPinStateChanged(pin, state);
}

void _AlpacaDevice::onDeviceDisconnect()
{
    close();
}

int _AlpacaDevice::getResetCount()
{
    if (_driveThread != nullptr) return _driveThread->getResetCount();
    return 0;
}

void _AlpacaDevice::clearResetCount()
{
    if (_driveThread != nullptr) _driveThread->clearResetCount();
}

void _AlpacaDevice::i2CReadRegister(uint32_t addr, uint32_t reg)
{
    if (_driveThread != nullptr) _driveThread->i2CReadRegister(addr, reg);
}

void _AlpacaDevice::i2CWriteRegister(uint32_t addr, uint32_t reg, uint32_t data)
{
    if (_driveThread != nullptr) _driveThread->i2CWriteRegister(addr, reg, data);
}
