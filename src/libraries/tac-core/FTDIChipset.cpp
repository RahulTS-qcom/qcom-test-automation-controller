// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

// Author: msimpson, biswroy

#include "FTDIChipset.h"

#include "AppCore.h"
#include "PlatformConfiguration.h"
#include "Range.h"
#include "version.h"

// FTDI
#include "ftd2xx.h"

#include "DebugLog.h"

#include <cstring>

#define CHIPSET_DBG(msg) \
    do { if (TACDebugLog::instance().enabled()) TACDebugLog::instance().write("FTDIChipset", msg); } while(0)

FTDIChipsetList _FTDIChipset::_ftdiChipsetList;

_FTDIChipset::_FTDIChipset() {}

std::string _FTDIChipset::ftidStatusToString(unsigned long status)
{
    std::string numberCode = std::to_string(status);
    std::string result = "Status " + numberCode + " not found";

    switch (status)
    {
    case FT_OK: result = "Status Okay"; break;
    case FT_INVALID_HANDLE: result = "Invalid handle: " + numberCode; break;
    case FT_DEVICE_NOT_FOUND: result = "Device not found: " + numberCode; break;
    case FT_DEVICE_NOT_OPENED: result = "Device not opened: " + numberCode; break;
    case FT_IO_ERROR: result = "IO error: " + numberCode; break;
    case FT_INSUFFICIENT_RESOURCES: result = "Insufficient resources: " + numberCode; break;
    case FT_INVALID_PARAMETER: result = "Invalid parameter: " + numberCode; break;
    case FT_INVALID_BAUD_RATE: result = "Invalid baud rate: " + numberCode; break;
    case FT_DEVICE_NOT_OPENED_FOR_ERASE: result = "Device not opened for erase: " + numberCode; break;
    case FT_DEVICE_NOT_OPENED_FOR_WRITE: result = "Device not opened for write: " + numberCode; break;
    case FT_FAILED_TO_WRITE_DEVICE: result = "Failed to write device: " + numberCode; break;
    case FT_EEPROM_READ_FAILED: result = "EEPROM read failed: " + numberCode; break;
    case FT_EEPROM_WRITE_FAILED: result = "EEPROM write failed: " + numberCode; break;
    case FT_EEPROM_ERASE_FAILED: result = "EEPROM erase failed: " + numberCode; break;
    case FT_EEPROM_NOT_PRESENT: result = "EEPROM not present: " + numberCode; break;
    case FT_EEPROM_NOT_PROGRAMMED: result = "EEPROM not programmed: " + numberCode; break;
    case FT_INVALID_ARGS: result = "Invalid Arguments: " + numberCode; break;
    case FT_NOT_SUPPORTED: result = "Not supported: " + numberCode; break;
    case FT_OTHER_ERROR: result = "Other error: " + numberCode; break;
    case FT_DEVICE_LIST_NOT_READY: result = "Device list not ready: " + numberCode; break;
    default: break;
    }

    return result;
}

bool _FTDIChipset::hasDevice(HashType portHash)
{
    for (const auto& device : _FTDIChipset::_ftdiChipsetList)
    {
        if (device->hash() == portHash)
            return true;
    }
    return false;
}

uint32_t _FTDIChipset::getDeviceCount()
{
    for (auto& device : _ftdiChipsetList)
        device->reset();

#ifdef _WIN32
    windowsTraversal();
#else
    linuxTraversal();
#endif

    return static_cast<uint32_t>(_ftdiChipsetList.size());
}

FTDIChipset _FTDIChipset::getDevice(uint32_t deviceIndex)
{
    FTDIChipset result;

    if (deviceIndex < static_cast<uint32_t>(_ftdiChipsetList.size()))
        result = _ftdiChipsetList.at(deviceIndex);

    return result;
}

FTDIChipset _FTDIChipset::getDevice(const std::string& portName)
{
    for (auto& ftdiChipset : _ftdiChipsetList)
    {
        if (ftdiChipset->portName().find(portName) != std::string::npos)
            return ftdiChipset;

        if (ftdiChipset->serialNumber().find(portName) != std::string::npos)
            return ftdiChipset;
    }

    return FTDIChipset();
}

std::string _FTDIChipset::normalizeSerialNumber(const std::string& segmentSerialNumber)
{
    std::string serialNumber{segmentSerialNumber};
    if (!serialNumber.empty())
        serialNumber.pop_back(); // remove trailing segment letter
    return serialNumber;
}

bool _FTDIChipset::open(FTDIPinSets pinsets)
{
    bool result{false};
    uint8_t mask = 0xff;
    uint8_t mode = FT_BITMODE_ASYNC_BITBANG;

    FT_STATUS ftStatus;

    CHIPSET_DBG("open() pinsets=" + std::to_string(static_cast<int>(pinsets)) +
        " A=" + ((pinsets & eA) ? "yes" : "no") + " B=" + ((pinsets & eB) ? "yes" : "no") +
        " C=" + ((pinsets & eC) ? "yes" : "no") + " D=" + ((pinsets & eD) ? "yes" : "no"));
    CHIPSET_DBG("  serialA='" + _aSerialNumber + "' serialB='" + _bSerialNumber +
        "' serialC='" + _cSerialNumber + "' serialD='" + _dSerialNumber + "'");

    AppCore::writeToApplicationLog(std::string("Bus A : ") + ((pinsets & eA) ? "true" : "false") + "\n");
    AppCore::writeToApplicationLog(std::string("Bus B : ") + ((pinsets & eB) ? "true" : "false") + "\n");
    AppCore::writeToApplicationLog(std::string("Bus C : ") + ((pinsets & eC) ? "true" : "false") + "\n");
    AppCore::writeToApplicationLog(std::string("Bus D : ") + ((pinsets & eD) ? "true" : "false") + "\n");

    if (pinsets & eA)
    {
        CHIPSET_DBG("Opening channel A: serial='" + _aSerialNumber + "'");
        ftStatus = FT_OpenEx(const_cast<char*>(_aSerialNumber.c_str()), FT_OPEN_BY_SERIAL_NUMBER, &_aHandle);
        CHIPSET_DBG("  Channel A: " + (ftStatus == FT_OK ? std::string("OK") : "FAILED status=" + _FTDIChipset::ftidStatusToString(ftStatus)));
        if (ftStatus == FT_OK)
        {
            FT_SetBitMode(_aHandle, mask, mode);
            result = true;
        }
        else
        {
            AppCore::writeToApplicationLog("FTDI Port A failure: " + _FTDIChipset::ftidStatusToString(ftStatus) + "\n");
            _aHandle = nullptr;
        }
    }

    if (pinsets & eB)
    {
        CHIPSET_DBG("Opening channel B: serial='" + _bSerialNumber + "'");
        ftStatus = FT_OpenEx(const_cast<char*>(_bSerialNumber.c_str()), FT_OPEN_BY_SERIAL_NUMBER, &_bHandle);
        CHIPSET_DBG("  Channel B: " + (ftStatus == FT_OK ? std::string("OK") : "FAILED status=" + _FTDIChipset::ftidStatusToString(ftStatus)));
        if (ftStatus == FT_OK)
        {
            FT_SetBitMode(_bHandle, mask, mode);
            result = true;
        }
        else
        {
            AppCore::writeToApplicationLog("FTDI Port B failure: " + _FTDIChipset::ftidStatusToString(ftStatus) + "\n");
            _bHandle = nullptr;
        }
    }

    if (pinsets & eC)
    {
        CHIPSET_DBG("Opening channel C: serial='" + _cSerialNumber + "'");
        ftStatus = FT_OpenEx(const_cast<char*>(_cSerialNumber.c_str()), FT_OPEN_BY_SERIAL_NUMBER, &_cHandle);
        CHIPSET_DBG("  Channel C: " + (ftStatus == FT_OK ? std::string("OK") : "FAILED status=" + _FTDIChipset::ftidStatusToString(ftStatus)));
        if (ftStatus == FT_OK)
        {
            FT_SetBitMode(_cHandle, mask, mode);
            result = true;
        }
        else
        {
            AppCore::writeToApplicationLog("FTDI Port C failure: " + _FTDIChipset::ftidStatusToString(ftStatus) + "\n");
            _cHandle = nullptr;
        }
    }

    if (pinsets & eD)
    {
        CHIPSET_DBG("Opening channel D: serial='" + _dSerialNumber + "'");
        ftStatus = FT_OpenEx(const_cast<char*>(_dSerialNumber.c_str()), FT_OPEN_BY_SERIAL_NUMBER, &_dHandle);
        CHIPSET_DBG("  Channel D: " + (ftStatus == FT_OK ? std::string("OK") : "FAILED status=" + _FTDIChipset::ftidStatusToString(ftStatus)));
        if (ftStatus == FT_OK)
        {
            FT_SetBitMode(_dHandle, mask, mode);
            result = true;
        }
        else
        {
            AppCore::writeToApplicationLog("FTDI Port D failure: " + _FTDIChipset::ftidStatusToString(ftStatus) + "\n");
            _dHandle = nullptr;
        }
    }

    CHIPSET_DBG("open() result=" + std::string(result ? "OK" : "FAILED"));
    return result;
}

bool _FTDIChipset::isOpen()
{
    return _aHandle != nullptr || _bHandle != nullptr || _cHandle != nullptr || _dHandle != nullptr;
}

void _FTDIChipset::close()
{
    FT_STATUS ftStatus;

    if (_aHandle != nullptr)
    {
        FT_SetBitMode(_aHandle, 0, 0);
        ftStatus = FT_Close(_aHandle);
        if (ftStatus == FT_OK)
            _aHandle = nullptr;
        else
            AppCore::writeToApplicationLog("FTDI Port A close failure: " + _FTDIChipset::ftidStatusToString(ftStatus) + "\n");
    }

    if (_bHandle != nullptr)
    {
        FT_SetBitMode(_bHandle, 0, 0);
        ftStatus = FT_Close(_bHandle);
        if (ftStatus == FT_OK)
            _bHandle = nullptr;
        else
            AppCore::writeToApplicationLog("FTDI Port B close failure: " + _FTDIChipset::ftidStatusToString(ftStatus) + "\n");
    }

    if (_cHandle != nullptr)
    {
        FT_SetBitMode(_cHandle, 0, 0);
        ftStatus = FT_Close(_cHandle);
        if (ftStatus == FT_OK)
            _cHandle = nullptr;
        else
            AppCore::writeToApplicationLog("FTDI Port C close failure: " + _FTDIChipset::ftidStatusToString(ftStatus) + "\n");
    }

    if (_dHandle != nullptr)
    {
        FT_SetBitMode(_dHandle, 0, 0);
        ftStatus = FT_Close(_dHandle);
        if (ftStatus == FT_OK)
            _dHandle = nullptr;
        else
            AppCore::writeToApplicationLog("FTDI Port D close failure: " + _FTDIChipset::ftidStatusToString(ftStatus) + "\n");
    }
}

PlatformID _FTDIChipset::platformID()
{
    return _platformID;
}

void _FTDIChipset::setPlatformID(PlatformID platformID)
{
    _platformID = platformID;
}

std::string _FTDIChipset::versionString()
{
    return PlatformContainer::toString(_platformID) + " " + firmwareString();
}

std::string _FTDIChipset::firmwareString()
{
    return TAC_LIB_VERSION;
}

HashType _FTDIChipset::hash()
{
    return _hash;
}

std::string _FTDIChipset::serialNumber()
{
    return _serialNumber;
}

bool _FTDIChipset::newDevice()
{
    bool result{_new};
    _new = false;
    return result;
}

std::string _FTDIChipset::aSerialNumber() { return _aSerialNumber; }

void _FTDIChipset::setASerialNumber(const std::string& aSerialNumber)
{
    if (_aSerialNumber.empty())
    {
        setupHash(aSerialNumber);
        _aSerialNumber = aSerialNumber;
    }
}

std::string _FTDIChipset::bSerialNumber() { return _bSerialNumber; }

void _FTDIChipset::setBSerialNumber(const std::string& bSerialNumber)
{
    if (_bSerialNumber.empty())
    {
        setupHash(bSerialNumber);
        _bSerialNumber = bSerialNumber;
    }
}

std::string _FTDIChipset::cSerialNumber() { return _cSerialNumber; }

void _FTDIChipset::setCSerialNumber(const std::string& cSerialNumber)
{
    if (_cSerialNumber.empty())
    {
        setupHash(cSerialNumber);
        _cSerialNumber = cSerialNumber;
    }
}

std::string _FTDIChipset::dSerialNumber() { return _dSerialNumber; }

void _FTDIChipset::setDSerialNumber(const std::string& dSerialNumber)
{
    if (_dSerialNumber.empty())
    {
        setupHash(dSerialNumber);
        _dSerialNumber = dSerialNumber;
    }
}

bool _FTDIChipset::write(uint8_t pin, bool state)
{
    bool result{false};

    void* handle{nullptr};
    CharBit* charBit{nullptr};

    if (pin <= 7) // a
    {
        if (_aHandle != nullptr)
        {
            handle = _aHandle;
            _aPins.set(pin, state);
            charBit = &_aPins;
        }
    }
    else if (pin <= 15) // b
    {
        if (_bHandle != nullptr)
        {
            handle = _bHandle;
            _bPins.set(pin, state);
            charBit = &_bPins;
        }
    }
    else if (pin <= 23) // c
    {
        if (_cHandle != nullptr)
        {
            handle = _cHandle;
            _cPins.set(pin, state);
            charBit = &_cPins;
        }
    }
    else if (pin <= 31) // d
    {
        if (_dHandle != nullptr)
        {
            handle = _dHandle;
            _dPins.set(pin, state);
            charBit = &_dPins;
        }
    }

    if (handle != nullptr && charBit != nullptr)
    {
        DWORD byteWritten;

        FT_STATUS status = FT_Write(handle, charBit->value(), 1, &byteWritten);
        if (status == FT_OK)
        {
            result = true;
        }
        else
        {
            std::string message = "FTDI Write Failed: " + _FTDIChipset::ftidStatusToString(status) + "\n";
            AppCore::writeToApplicationLog(message);
            _lastError = message;
        }
    }

    return result;
}

HashType _FTDIChipset::hash(const std::string& serialNumber)
{
    return ::arrayHash(serialNumber);
}

void _FTDIChipset::setSerialNumber(const std::string& serialNumber)
{
    if (_serialNumber.empty())
    {
        _serialNumber = serialNumber;
        setupPortName();
    }
}

unsigned long long _FTDIChipset::setCustomVIDPID()
{
    unsigned long long ftStatus = !FT_OK;
#ifndef _WIN32
    ftStatus = FT_SetVIDPID(1027, 6552);
    if (ftStatus == FT_OK)
        AppCore::writeToApplicationLog("Configured FTDI to detect VID: 0403 and PID: 1998\n");
#endif
    return ftStatus;
}

void _FTDIChipset::setupHash(const std::string& segmentSerialNumber)
{
    if (_hash == 0)
    {
        std::string serialNumber = _FTDIChipset::normalizeSerialNumber(segmentSerialNumber);
        _hash = hash(serialNumber);
        _serialNumber = serialNumber;
    }
}

void _FTDIChipset::setupPortName()
{
    if (_portName.empty())
    {
        if (!_serialNumber.empty())
        {
            // In non-Qt build, generate a simple VTP name based on hash
            // TODO: persist VTP assignment to JSON config file
            static uint32_t nextPort = 1;
            _portName = "VTP" + std::to_string(nextPort++);
        }
    }
}

PlatformID _FTDIChipset::nameToPlatform(const std::string& deviceName)
{
    return PlatformContainer::fromUSBDescriptor(deviceName);
}

#ifndef _WIN32
void _FTDIChipset::linuxTraversal()
{
    FT_STATUS ftStatus;
    DWORD deviceCount;

    ftStatus = setCustomVIDPID();
    if (ftStatus == FT_OK)
    {
        ftStatus = FT_CreateDeviceInfoList(&deviceCount);
        if (ftStatus == FT_OK && deviceCount > 0)
        {
            FT_DEVICE_LIST_INFO_NODE* devInfoList = new FT_DEVICE_LIST_INFO_NODE[deviceCount];

            ftStatus = FT_GetDeviceInfoList(devInfoList, &deviceCount);
            if (ftStatus == FT_OK)
            {
                FT_HANDLE ftHandleTemp;
                DWORD Flags, ID, Type, LocId;
                char SerialNumber[16];
                char Description[64];

                for (DWORD i = 0; i < deviceCount; i++)
                {
                    ftStatus = FT_GetDeviceInfoDetail(i, &Flags, &Type, &ID, &LocId, SerialNumber, Description, &ftHandleTemp);

                    std::string description(Description);
                    // USB descriptor is description minus the trailing segment letter and space
                    std::string usbDescriptor = description.substr(0, description.length() > 2 ? description.length() - 2 : 0);

                    if (_PlatformConfiguration::containsUSBDescriptor(usbDescriptor) ||
                        usbDescriptor.find("ALPACA-LITE ") == 0)
                    {
                        PlatformID platformID = _PlatformConfiguration::getUSBDescriptor(usbDescriptor);
                        std::string deviceSerialNumber(SerialNumber);
                        std::string serialNumber = _FTDIChipset::normalizeSerialNumber(deviceSerialNumber);

                        if (!serialNumber.empty())
                        {
                            HashType portHash = hash(serialNumber);
                            char segment = description.empty() ? '\0' : description.back();

                            FTDIChipset ftdiChipset;

                            if (hasDevice(portHash))
                            {
                                ftdiChipset = getDevice(serialNumber);
                            }
                            else
                            {
                                ftdiChipset = FTDIChipset(new _FTDIChipset);
                                ftdiChipset->setPlatformID(platformID);
                                _ftdiChipsetList.push_back(ftdiChipset);
                            }

                            ftdiChipset->setSerialNumber(serialNumber);
                            if (ftdiChipset->_usbDescriptor.empty())
                                ftdiChipset->_usbDescriptor = usbDescriptor;

                            switch (segment)
                            {
                            case 'A': case 'a': ftdiChipset->setASerialNumber(deviceSerialNumber); break;
                            case 'B': case 'b': ftdiChipset->setBSerialNumber(deviceSerialNumber); break;
                            case 'C': case 'c': ftdiChipset->setCSerialNumber(deviceSerialNumber); break;
                            case 'D': case 'd': ftdiChipset->setDSerialNumber(deviceSerialNumber); break;
                            }

                            ftdiChipset->_active = true;
                        }
                    }
                }
            }
            delete[] devInfoList;
        }
    }
}

void _FTDIChipset::windowsTraversal() {}
#endif

#ifdef _WIN32
void _FTDIChipset::linuxTraversal() {}

void _FTDIChipset::windowsTraversal()
{
    FT_STATUS ftStatus;
    DWORD deviceCount;

    CHIPSET_DBG("windowsTraversal starting");

    ftStatus = FT_CreateDeviceInfoList(&deviceCount);
    CHIPSET_DBG("FT_CreateDeviceInfoList: status=" + std::to_string(ftStatus) + " count=" + std::to_string(deviceCount));

    if (ftStatus == FT_OK && deviceCount > 0)
    {
        FT_DEVICE_LIST_INFO_NODE* devInfoList = new FT_DEVICE_LIST_INFO_NODE[deviceCount];

        ftStatus = FT_GetDeviceInfoList(devInfoList, &deviceCount);
        if (ftStatus == FT_OK)
        {
            for (DWORD i = 0; i < deviceCount; i++)
            {
                std::string description(devInfoList[i].Description);
                std::string usbDescriptor = description.substr(0, description.length() > 2 ? description.length() - 2 : 0);

                bool containsDesc = _PlatformConfiguration::containsUSBDescriptor(usbDescriptor);
                bool startsWithAlpaca = usbDescriptor.find("ALPACA-LITE ") == 0;

                CHIPSET_DBG("[" + std::to_string(i) + "] desc='" + description + "' usbDesc='" + usbDescriptor +
                    "' containsDesc=" + (containsDesc ? "YES" : "no") + " startsWithAlpaca=" + (startsWithAlpaca ? "YES" : "no"));

                if (containsDesc || startsWithAlpaca)
                {
                    PlatformID platformID = _PlatformConfiguration::getUSBDescriptor(usbDescriptor);
                    std::string deviceSerialNumber(devInfoList[i].SerialNumber);
                    std::string serialNumber = _FTDIChipset::normalizeSerialNumber(deviceSerialNumber);

                    if (!serialNumber.empty())
                    {
                        HashType portHash = hash(serialNumber);
                        char segment = description.empty() ? '\0' : description.back();

                        FTDIChipset ftdiChipset;

                        if (hasDevice(portHash))
                        {
                            ftdiChipset = getDevice(serialNumber);
                        }
                        else
                        {
                            ftdiChipset = FTDIChipset(new _FTDIChipset);
                            ftdiChipset->setPlatformID(platformID);
                            _ftdiChipsetList.push_back(ftdiChipset);
                        }

                        ftdiChipset->setSerialNumber(serialNumber);
                        if (ftdiChipset->_usbDescriptor.empty())
                            ftdiChipset->_usbDescriptor = usbDescriptor;

                        switch (segment)
                        {
                        case 'A': case 'a': ftdiChipset->setASerialNumber(deviceSerialNumber); break;
                        case 'B': case 'b': ftdiChipset->setBSerialNumber(deviceSerialNumber); break;
                        case 'C': case 'c': ftdiChipset->setCSerialNumber(deviceSerialNumber); break;
                        case 'D': case 'd': ftdiChipset->setDSerialNumber(deviceSerialNumber); break;
                        }

                        ftdiChipset->_active = true;

                        if (devInfoList[i].Flags == FT_FLAGS_OPENED)
                            ftdiChipset->_lastError = "QTAC failed to enumerate all connected FTDI devices as they are in use";
                    }
                }
            }
        }
        delete[] devInfoList;
    }
}
#endif
