#ifndef PSOCPLATFORMCONFIGURATION_H
#define PSOCPLATFORMCONFIGURATION_H
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

/*
	Author: Biswajit Roy (biswroy@qti.qualcomm.com)
*/

#include "PlatformConfiguration.h"
#include "CommandGroup.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

const PlatformID kMaxPSOCPlatformId(255);

struct PSOCPinData
{
    PSOCPinData() = default;
    PSOCPinData(const PSOCPinData&) = default;
    PSOCPinData(PinID pin) { _pin = pin; _hash = _pin; }

    void clear() { *this = PSOCPinData(); }

    PinID           _pin{0};
    HashType        _hash{0};
    bool            _enabled{true};
    std::string     _pinLabel;
    std::string     _pinTooltip;
    bool            _initialValue{false};
    int             _initializationPriority{0};
    bool            _inverted{false};
    std::string     _pinCommand;
    CommandGroups   _commandGroup{eUnknownCommandGroup};
    std::string     _classicAction;
    int             _cellX{-1};
    int             _cellY{-1};
    std::string     _tabName;
};

typedef std::map<PinID, PSOCPinData>  PSOCPinEntries;
typedef std::vector<PSOCPinData>      PSOCPinList;

class _PSOCPlatformConfiguration;

class QCOMMONCONSOLE_EXPORT _PSOCPlatformConfiguration : public _PlatformConfiguration
{
public:
    _PSOCPlatformConfiguration();
    virtual ~_PSOCPlatformConfiguration();

    virtual Pins getPins();

    PSOCPinList getAllPins();
    PSOCPinList getActivePins();

    bool getPinEnableState(PinID pinId) const;
    void setPinEnableState(PinID pinId, bool newState);

    bool getInitialPinValue(PinID pinId) const;
    void setInitialPinValue(PinID pinId, bool newState);

    uint64_t getPinInitializationPriority(PinID pinId) const;
    void setPinInitializationPriority(PinID pinId, int priority);

    bool getPinInvertedState(PinID pinId) const;
    void setPinInvertedState(PinID pinId, bool newState);

    std::string getPinLabel(PinID pinId) const;
    void setPinLabel(PinID pinId, const std::string& pinLabel);

    std::string getPinTooltip(PinID pinId) const;
    void setPinTooltip(PinID pinId, const std::string& pinTooltip);

    std::string getPinCommand(PinID pinId) const;
    void setPinCommand(PinID pinId, const std::string& pinCommand);

    CommandGroups getPinGroup(PinID pinId) const;
    void setPinGroup(PinID pinId, CommandGroups commandGroup);

    std::string getTabName(PinID pinId) const;
    void setTabName(PinID pinId, const std::string& tabName);

    std::string getClassicAction(PinID pinId) const;
    void setClassicAction(PinID pinId, const std::string& classicAction);

    void getPinCellLocation(PinID pinId, int& x, int& y) const;
    void setPinCellLocation(PinID pinId, int x, int y);

protected:
    virtual void cascadeTabDelete(const std::string& deleteMe);
    virtual void cascadeTabRename(const std::string& oldName, const std::string& newName);

    virtual bool read(const nlohmann::json& parentLevel);
    virtual void write(nlohmann::json& parentLevel);

private:
    PSOCPinEntries  _pinEntries;

    static void initialize();
    static PSOCPinEntries _classicActions;
};

#endif // PSOCPLATFORMCONFIGURATION_H
