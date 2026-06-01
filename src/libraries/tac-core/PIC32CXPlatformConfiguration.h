// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef PIC32CXPLATFORMCONFIGURATION_H
#define PIC32CXPLATFORMCONFIGURATION_H

#include "CommandGroup.h"
#include "PlatformConfiguration.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

const PlatformID kMaxPIC32CXPlatformId(99999);

struct PIC32CXPinData
{
    PIC32CXPinData() = default;
    PIC32CXPinData(const PIC32CXPinData&) = default;
    PIC32CXPinData(PinID setPin)
    {
        _hash = PIC32CXPinData::makePIC32CXHash(setPin);
    }

    void clear() { *this = PIC32CXPinData(); }

    static HashType makePIC32CXHash(PinID setPin)
    {
        return strHash(std::to_string(setPin));
    }

    HashType        _hash{0};
    PinID           _setPin{static_cast<PinID>(-1)};
    bool            _enabled{false};
    std::string     _pinLabel;
    std::string     _pinTooltip;
    bool            _inverted{false};
    std::string     _pinCommand;
    CommandGroups   _commandGroup{eUnknownCommandGroup};
    int             _cellX{-1};
    int             _cellY{-1};
    std::string     _tabName{"General"};
};

typedef std::map<PinID, PIC32CXPinData>  PIC32CXPinEntries;
typedef std::vector<PIC32CXPinData>      PIC32CXPinList;

class _PIC32CXPlatformConfiguration;

class QCOMMONCONSOLE_EXPORT _PIC32CXPlatformConfiguration : public _PlatformConfiguration
{
public:
    _PIC32CXPlatformConfiguration();
    virtual ~_PIC32CXPlatformConfiguration();

    virtual Pins getPins();

    PIC32CXPinList getAllPins();
    PIC32CXPinList getActivePins();

    bool getPinEnableState(PinID pinId) const;
    void setPinEnableState(HashType hash, bool newState);

    bool getPinInvertedState(PinID pinId) const;
    void setPinInvertedState(HashType hash, bool newState);

    std::string getPinLabel(PinID pinId) const;
    void setPinLabel(HashType hash, const std::string& pinLabel);

    std::string getPinTooltip(PinID pinId) const;
    void setPinTooltip(HashType hash, const std::string& pinTooltip);

    std::string getPinCommand(PinID pinId) const;
    void setPinCommand(HashType hash, const std::string& pinCommand);

    CommandGroups getPinGroup(PinID pinId) const;
    void setPinGroup(HashType hash, CommandGroups pinGroup);

    std::string getTabName(PinID pinId) const;
    void setTabName(HashType hash, const std::string& tabName);

    void getPinCellLocation(PinID pinId, int& x, int& y) const;
    void setPinCellLocation(PinID pinId, int x, int y);

    PinID bitFromSetPin(PinID setPin);

protected:
    virtual void cascadeTabDelete(const std::string& deleteMe);
    virtual void cascadeTabRename(const std::string& oldName, const std::string& newName);

    virtual bool read(const nlohmann::json& parentLevel);
    virtual void write(nlohmann::json& parentLevel);

private:
    void initialize();
    PIC32CXPinEntries _pinEntries;
};
#endif // PIC32CXPLATFORMCONFIGURATION_H
