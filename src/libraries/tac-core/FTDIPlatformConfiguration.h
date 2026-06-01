#ifndef FTDIPLATFORMCONFIGURATION_H
#define FTDIPLATFORMCONFIGURATION_H
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

#include "QCommonConsoleGlobal.h"

#include "PlatformConfiguration.h"
#include "CommandGroup.h"
#include "StringUtilities.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

const int kMaxPinIndex{7};
const int kPinsPerBus{8};

typedef char Bus;   // replaces QChar — single ASCII letter: 'A','B','C','D'

enum FTDIBusFunction
{
    eBusFunctionUnknown = 0,
    eBusFunctionVCP,
    eBusFunctionD2XX,
    eBusFunctionI2C
};

struct FTDIBusData
{
    FTDIBusData() = default;
    FTDIBusData(const FTDIBusData&) = default;
    FTDIBusData(ChipIndex chipIndex, Bus bus, FTDIBusFunction busFunction)
    {
        _hash = FTDIBusData::makeFTDIHash(chipIndex, bus);
        _chipIndex = chipIndex;
        _bus = bus;
        _busFunction = busFunction;
    }

    void clear() { *this = FTDIBusData(); }

    static HashType makeFTDIHash(ChipIndex chipIndex, Bus bus)
    {
        return strHash(std::to_string(chipIndex) + bus);
    }

    static std::string toString(FTDIBusFunction busFunction)
    {
        switch (busFunction)
        {
        case eBusFunctionVCP:  return "VCP";
        case eBusFunctionD2XX: return "D2XX";
        case eBusFunctionI2C:  return "I2C";
        default:               return {};
        }
    }

    static FTDIBusFunction fromString(const std::string& busFunction)
    {
        std::string upper;
        for (auto c : busFunction)
            upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (upper == "VCP")  return eBusFunctionVCP;
        if (upper == "D2XX") return eBusFunctionD2XX;
        if (upper == "I2C")  return eBusFunctionI2C;
        return eBusFunctionUnknown;
    }

    HashType        _hash{0};
    ChipIndex       _chipIndex{1};
    Bus             _bus{0};
    FTDIBusFunction _busFunction{eBusFunctionUnknown};
};

typedef std::map<HashType, FTDIBusData> FTDIBusFunctions;

struct FTDIPinData
{
    FTDIPinData() = default;
    FTDIPinData(const FTDIPinData&) = default;
    FTDIPinData(ChipIndex chipIndex, Bus bus, PinID pin)
    {
        _hash = FTDIPinData::makeFTDIHash(chipIndex, bus, pin);
        _chipIndex = chipIndex;
        _bus = bus;
        _chipPin = pin;
    }

    void clear() { *this = FTDIPinData(); }

    static HashType makeFTDIHash(ChipIndex chipIndex, Bus bus, PinID pin)
    {
        return strHash(std::to_string(chipIndex) + bus + std::to_string(pin));
    }

    HashType        _hash{0};
    ChipIndex       _chipIndex{1};
    Bus             _bus{0};
    PinID           _chipPin{static_cast<PinID>(-1)};
    PinID           _setPin{static_cast<PinID>(-1)};
    bool            _enabled{false};
    bool            _input{false};
    std::string     _pinLabel;
    std::string     _pinTooltip;
    bool            _initialValue{false};
    int             _initializationPriority{-1};
    bool            _inverted{false};
    std::string     _pinCommand;
    CommandGroups   _commandGroup{eUnknownCommandGroup};
    int             _cellX{-1};
    int             _cellY{-1};
    std::string     _tabName{"General"};
};

typedef std::map<HashType, FTDIPinData>  FTDIPinEntries;
typedef std::vector<FTDIPinData>         FTDIPinList;

class QCOMMONCONSOLE_EXPORT _FTDIPlatformConfiguration : public _PlatformConfiguration
{
public:
    _FTDIPlatformConfiguration() = delete;
    _FTDIPlatformConfiguration(uint16_t chipCount);
    void initialize(uint16_t chipCount);
    virtual ~_FTDIPlatformConfiguration();

    FTDIPinData getPinData(ChipIndex chipIndex, Bus bus, PinID pin);

    FTDIPinList getAllPins() const;
    FTDIPinList getActivePins() const;
    FTDIPinList getActivePins(ChipIndex chipIndex, Bus bus) const;

    int getChipCount();
    virtual Pins getPins();

    bool getPinEnableState(ChipIndex chipIndex, Bus busName, PinID pinId) const;
    void setPinEnableState(HashType hash, bool newState);
    void setPinEnableState(ChipIndex chipIndex, Bus busName, PinID pinId, bool newState);

    bool getPinInputState(ChipIndex chipIndex, Bus busName, PinID pinId) const;
    void setPinInputState(HashType hash, bool newState);
    void setPinInputState(ChipIndex chipIndex, Bus busName, PinID pinId, bool newState);

    bool getInitialPinValue(ChipIndex chipIndex, Bus busName, PinID pinId) const;
    void setInitialPinValue(HashType hash, bool newState);
    void setInitialPinValue(ChipIndex chipIndex, Bus busName, PinID pinId, bool newState);

    int getPinInitializationPriority(ChipIndex chipIndex, Bus busName, PinID pinId) const;
    void setPinInitializationPriority(HashType hash, int priority);
    void setPinInitializationPriority(ChipIndex chipIndex, Bus busName, PinID pinId, int priority);

    bool getPinInvertedState(ChipIndex chipIndex, Bus busName, PinID pinId) const;
    void setPinInvertedState(HashType hash, bool newState);
    void setPinInvertedState(ChipIndex chipIndex, Bus busName, PinID pinId, bool newState);

    std::string getPinLabel(ChipIndex chipIndex, Bus busName, PinID pinId) const;
    void setPinLabel(HashType hash, const std::string& pinLabel);
    void setPinLabel(ChipIndex chipIndex, Bus busName, PinID pinId, const std::string& pinLabel);

    std::string getPinTooltip(ChipIndex chipIndex, Bus busName, PinID pinId) const;
    void setPinTooltip(HashType hash, const std::string& pinTooltip);
    void setPinTooltip(ChipIndex chipIndex, Bus busName, PinID pinId, const std::string& pinTooltip);

    std::string getPinCommand(ChipIndex chipIndex, Bus busName, PinID pinId) const;
    void setPinCommand(HashType hash, const std::string& pinCommand);
    void setPinCommand(ChipIndex chipIndex, Bus busName, PinID pinId, const std::string& pinCommand);

    CommandGroups getPinGroup(ChipIndex chipIndex, Bus busName, PinID pinId) const;
    void setPinGroup(HashType hash, CommandGroups pinGroup);
    void setPinGroup(ChipIndex chipIndex, Bus busName, PinID pinId, CommandGroups pinGroup);

    std::string getTabName(ChipIndex chipIndex, Bus busName, PinID pinId) const;
    void setTabName(HashType hash, const std::string& tabName);
    void setTabName(ChipIndex chipIndex, Bus busName, PinID pinId, const std::string& tabName);

    void getPinCellLocation(ChipIndex chipIndex, Bus busName, PinID pinId, int& x, int& y) const;
    void setPinCellLocation(HashType hash, int x, int y);
    void setPinCellLocation(ChipIndex chipIndex, Bus busName, PinID pinId, int x, int y);

    FTDIBusData getBusFunction(ChipIndex chipIndex, Bus busName) const;
    void setBusFunction(HashType hash, FTDIBusFunction busFunction);
    void setBusFunction(ChipIndex chipIndex, Bus busName, FTDIBusFunction busFunction);

    FTDIPinSets getPinSet(ChipIndex chipIndex);
    void setPinSet(ChipIndex chipIndex, FTDIPinSets pinSet);

protected:
    virtual void cascadeTabDelete(const std::string& deleteMe);
    virtual void cascadeTabRename(const std::string& oldName, const std::string& newName);

    virtual bool read(const nlohmann::json& parentLevel);
    virtual void write(nlohmann::json& parentLevel);

private:
    PinID getSetPinIndex(int chipIndex, Bus bus, PinID pinId);

    int              _chipCount{1};
    FTDIPinEntries   _pinEntries;
    FTDIBusFunctions _busFunctions;
    FTDIPinSets      _pinSets[kMaxPinSetCount]{};   // per-chip pin set flags
    std::string      _usbDescriptorString;
};

#endif // FTDIPLATFORMCONFIGURATION_H
