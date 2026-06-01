// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

/*
	Author: Michael Simpson (msimpson@qti.qualcomm.com)
*/

#include "PlatformID.h"

#include "AppCore.h"
#include "ConsoleApplicationEnhancements.h"
#include "RangedContainer.h"
#include "USBDescriptors.h"

#include <algorithm>
#include <cctype>

const PlatformID MICRO_EPM_BOARD_ID_SPMV4{1};
const PlatformID MICRO_EPM_BOARD_ID_EPMV4{2};
const PlatformID MICRO_EPM_BOARD_ID_ALPACA{3};
const PlatformID MICRO_EPM_BOARD_ID_MICROEPM_TAC{4};
const PlatformID MICRO_EPM_BOARD_ID_ALPACA_V2{5};
const PlatformID MICRO_EPM_BOARD_ID_ALPACA_V3{6};
const PlatformID MICRO_EPM_BOARD_ID_ALPACA_V3P1{7};
const PlatformID EPM_BOARD_ID_MTP_V3P2{8};
const PlatformID EPM_BOARD_ID_QRD_V1P0{9};
const PlatformID EPM_BOARD_ID_IDP_V1P0{10};
const PlatformID EPM_BOARD_ID_DONGLE_V3P0{11};
const PlatformID EPM_BOARD_ID_MTP_V3P3{12};

PlatformIDs PlatformContainer::_platformIds;

_PlatformEntry::_PlatformEntry(
    PlatformID platformID,
    DebugBoardType boardType,
    const std::string& description,
    const std::string& path,
    const std::string& usbDescriptor)
    : _platformID(platformID)
    , _boardtype(boardType)
    , _description(description)
    , _usbDescriptor(usbDescriptor)
    , _path(path)
{
    _pinSets[0] = _pinSets[1] = _pinSets[2] = _pinSets[3] = NoOptions;
}

void PlatformContainer::initialize()
{
    if (_platformIds.empty())
    {
        auto add = [](PlatformID id, DebugBoardType bt, const std::string& desc) {
            return std::make_shared<_PlatformEntry>(id, bt, desc);
        };

        _platformIds[MICRO_EPM_BOARD_ID_SPMV4]       = add(MICRO_EPM_BOARD_ID_SPMV4,       ePSOC, "SPM V4");
        _platformIds[MICRO_EPM_BOARD_ID_EPMV4]       = add(MICRO_EPM_BOARD_ID_EPMV4,       ePSOC, "EPM V4");
        _platformIds[MICRO_EPM_BOARD_ID_ALPACA]      = add(MICRO_EPM_BOARD_ID_ALPACA,      ePSOC, "Alpaca V1");
        _platformIds[MICRO_EPM_BOARD_ID_MICROEPM_TAC]= add(MICRO_EPM_BOARD_ID_MICROEPM_TAC,ePSOC, "MicroEPM TAC");
        _platformIds[MICRO_EPM_BOARD_ID_ALPACA_V2]   = add(MICRO_EPM_BOARD_ID_ALPACA_V2,   ePSOC, "Alpaca V2");
        _platformIds[MICRO_EPM_BOARD_ID_ALPACA_V3]   = add(MICRO_EPM_BOARD_ID_ALPACA_V3,   ePSOC, "Alpaca V3.0");
        _platformIds[MICRO_EPM_BOARD_ID_ALPACA_V3P1] = add(MICRO_EPM_BOARD_ID_ALPACA_V3P1, ePSOC, "Alpaca V3.1");
        _platformIds[EPM_BOARD_ID_MTP_V3P2]          = add(EPM_BOARD_ID_MTP_V3P2,          ePSOC, "MTP V3.2");
        _platformIds[EPM_BOARD_ID_QRD_V1P0]          = add(EPM_BOARD_ID_QRD_V1P0,          ePSOC, "QRD V1.0");
        _platformIds[EPM_BOARD_ID_IDP_V1P0]          = add(EPM_BOARD_ID_IDP_V1P0,          ePSOC, "IDP V1.0");
        _platformIds[EPM_BOARD_ID_DONGLE_V3P0]       = add(EPM_BOARD_ID_DONGLE_V3P0,       ePSOC, "Dongle V3.0");
        _platformIds[EPM_BOARD_ID_MTP_V3P3]          = add(EPM_BOARD_ID_MTP_V3P3,          ePSOC, "MTP V3.3");

        auto ftdiEntry = std::make_shared<_PlatformEntry>(ALPACA_LITE_ID, eFTDI, "ALPACA LITE (FTDI)");
        ftdiEntry->_usbDescriptor = "ALPACA-LITE MTP DEBUG BOARD";
        ftdiEntry->_pinSets[0] = static_cast<FTDIPinSets>(eC | eD);
        _platformIds[ALPACA_LITE_ID] = ftdiEntry;

        _platformIds[ALPACA_PIC32CX_ID] = std::make_shared<_PlatformEntry>(
            ALPACA_PIC32CX_ID, ePIC32CXAuto, "Default Automotive (PIC32CX)");

        initializeDynamic();
    }
}

PlatformIDList PlatformContainer::getEntries()
{
    PlatformIDList result;
    for (const auto& kv : _platformIds)
        result.push_back(kv.second);
    return result;
}

void PlatformContainer::initializeDynamic()
{
    USBDescriptors usbDescriptors;

    std::string loadPath = tacConfigRoot() + kUSBDescriptorFileName;

    AppCore::writeToApplicationLog("[PlatformContainer::initializeDynamic] Load path: '" + loadPath + "'\n");

    if (usbDescriptors.load(loadPath))
    {
        DescriptorList descriptorList = usbDescriptors.getDescriptors();
        AppCore::writeToApplicationLog("[PlatformContainer::initializeDynamic] Loaded " +
            std::to_string(descriptorList.size()) + " entries from devicelist.json\n");

        for (const auto& descriptor : descriptorList)
        {
            if (_platformIds.find(descriptor._platformID) == _platformIds.end())
            {
                auto platformEntry = std::make_shared<_PlatformEntry>(
                    descriptor._platformID, descriptor._debugBoardType, descriptor._description);

                platformEntry->_usbDescriptor       = descriptor._usbDescriptor;
                platformEntry->_path                = descriptor._configurationFilePath;
                platformEntry->_pinSets[0]          = descriptor._pinSets[0];
                platformEntry->_pinSets[1]          = descriptor._pinSets[1];
                platformEntry->_pinSets[2]          = descriptor._pinSets[2];
                platformEntry->_pinSets[3]          = descriptor._pinSets[3];

                _platformIds[descriptor._platformID] = platformEntry;
            }
        }
    }
    else
    {
        AppCore::writeToApplicationLog("[PlatformContainer::initializeDynamic] FAILED to load: " +
            usbDescriptors.getLastError() + "\n");
    }

    if (AppCore::getAppCore()->appLoggingActive())
    {
        for (const auto& kv : _platformIds)
            AppCore::writeToApplicationLog("   " + std::to_string(kv.second->_platformID) + " " + kv.second->_description + "\n");
    }
}

void PlatformContainer::addEntry(PlatformEntry platformEntry)
{
    _platformIds[platformEntry->_platformID] = platformEntry;
}

std::string PlatformContainer::toString(PlatformID platformID)
{
    auto it = _platformIds.find(platformID);
    if (it != _platformIds.end())
        return it->second->_description;
    return "Unknown Board ID: " + std::to_string(platformID);
}

PlatformID PlatformContainer::fromUSBDescriptor(const std::string& usbDescriptor)
{
    std::string lower;
    for (auto c : usbDescriptor)
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    for (const auto& kv : _platformIds)
    {
        std::string candidate;
        for (auto c : kv.second->_usbDescriptor)
            candidate += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (candidate == lower)
            return kv.second->_platformID;
    }
    return MICRO_EPM_BOARD_ID_UNKNOWN;
}

DebugBoardType PlatformContainer::getDebugBoardType(PlatformID platformID)
{
    auto it = _platformIds.find(platformID);
    if (it != _platformIds.end())
        return it->second->_boardtype;
    return eUnknownDebugBoard;
}

PlatformIDList PlatformContainer::getDebugBoards()
{
    PlatformIDList result;
    for (const auto& kv : _platformIds)
        result.push_back(kv.second);
    return result;
}

PlatformIDList PlatformContainer::getDebugBoardsOfType(DebugBoardType debugBoardType)
{
    PlatformIDList result;
    for (const auto& kv : _platformIds)
        if (kv.second->_boardtype == debugBoardType)
            result.push_back(kv.second);
    return result;
}
