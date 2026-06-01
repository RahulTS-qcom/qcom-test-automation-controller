#ifndef PLATFORMCONFIGURATION_H
#define PLATFORMCONFIGURATION_H
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

/*
	Author: Biswajit Roy (biswroy@qti.qualcomm.com)
			Michael Simpson (msimpson@qti.qualcomm.com)
*/

// QCommonConsole
#include "QCommonConsoleGlobal.h"
#include "Button.h"
#include "PinEntry.h"
#include "PlatformID.h"
#include "ScriptVariable.h"
#include "StringUtilities.h"
#include "Tabs.h"
#include "USBDescriptors.h"

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

const int kDefaultPlatformId{99999};
const int kDefaultFirmwareVersion{15};

// Replaces QSize — used only for form layout dimensions
struct TACSize { int width{560}; int height{725}; };
const TACSize kClassicDimension{560, 725};

typedef int16_t ChipIndex;

class _PlatformConfiguration;

typedef std::shared_ptr<_PlatformConfiguration> PlatformConfiguration;

struct QCOMMONCONSOLE_EXPORT TACPlatformEntry
{
    TACPlatformEntry() = default;

    PlatformEntry         _platformEntry;
    PlatformConfiguration getConfiguration();

protected:
    PlatformConfiguration _platformConfiguration;
};

typedef std::map<PlatformID, TACPlatformEntry> TACPlatformEntries;

class QCOMMONCONSOLE_EXPORT _PlatformConfiguration
{
public:
    _PlatformConfiguration();
    _PlatformConfiguration(const _PlatformConfiguration&) = delete;
    _PlatformConfiguration& operator=(const _PlatformConfiguration&) = delete;
    virtual ~_PlatformConfiguration();

    static void initializeDynamicPlatform();

    static PlatformConfiguration createPlatformConfiguration(DebugBoardType debugBoardType, int chipCount = 1);
    static PlatformConfiguration openPlatformConfiguration(const std::string& filePath);

    static std::vector<PlatformID> platformEntryIds();
    static TACPlatformEntry getEntry(PlatformID platformID);

    static std::string makeConfigName(const std::string& chip, PlatformID platformID);
    static bool parseConfigName(const std::string& fileName, DebugBoardType& debugBoardType, PlatformID& platformID);

    static PlatformID getUSBDescriptor(const std::string& usbDescriptorString);
    static bool containsUSBDescriptor(const std::string& usbDescriptorString);

    static std::string getLastError() { return _lastError; }

    bool dirty() { return _dirty; }

    bool load(const std::string& filePath);
    void save();

    static std::string lastError();

    std::string name()        { return _name; }
    void setName(const std::string& name);

    std::string author()      { return _author; }
    void setAuthor(const std::string& author);

    std::string description() { return _description; }
    void setDescription(const std::string& description);

    std::string creationDate()     { return _creationDate; }
    std::string modificationDate() { return _modifyDate; }

    DebugBoardType getPlatform() { return _platform; }

    std::string getFileVersion();
    uint32_t fileVersion();

    std::string getPineVersion();
    void setPineVersion(uint32_t pineVersion);

    std::string getPlatformString();
    void setPlatform(DebugBoardType platform);
    void setPlatform(const std::string& platform);

    PlatformID getPlatformId();
    void setPlatformID(PlatformID platformId);

    void deleteTabs(Tabs& tabs);
    void updateTabs(Tabs& tabs);
    Tabs getTabs();
    std::vector<std::string> getAllTabs();
    std::vector<std::string> getVisibleTabs();

    void renameTab(const std::string& oldName, const std::string& newName);
    void setTabVisible(const std::string& tabName, bool visible);

    virtual Pins getPins();

    ButtonList getButtons();
    Buttons getButtonsMap();

    ScriptVariables getVariables();
    bool setVariableName(const std::string& variableName);
    void setVariableLabel(const std::string& variableName, const std::string& variableLabel);
    void setVariableTooltip(const std::string& variableName, const std::string& variableTooltip);
    void setVariableType(const std::string& variableName, VariableType variableType);
    bool setVariableDefaultValue(const std::string& variableName, const ScriptVariableValue& variableDefaultValue);
    void setVariableCellLocation(const std::string& variableName, int cellX, int cellY);
    void deleteVariable(const std::string& variableName);

    void deleteButton(HashType hash);

    HashType addButtonLabel(const std::string& buttonLabel, const std::string& tabName);
    bool setButtonLabel(HashType hash, const std::string& buttonLabel);

    HashType addButtonTooltip(const std::string& labelName, const std::string& tabName, const std::string& toolTip);
    bool setButtonTooltip(HashType hash, const std::string& toolTip);

    HashType addButtonCommand(const std::string& labelName, const std::string& tabName, const std::string& command);
    bool setButtonCommand(HashType hash, const std::string& command);

    HashType addButtonCommandGroup(const std::string& labelName, const std::string& tabName, CommandGroups commandGroup);
    bool setButtonCommandGroup(HashType hash, CommandGroups commandGroup);

    HashType addButtonTab(const std::string& labelName, const std::string& tabName);
    bool setButtonTab(HashType hash, const std::string& tabName);

    HashType addButtonCellLocation(const std::string& labelName, const std::string& tabName, int cellX, int cellY);
    bool setButtonCellLocation(HashType hash, int cellX, int cellY);

    TACSize getFormDimension();
    void setFormDimension(const TACSize& formDimension);

    bool resetActive() { return _resetActive; }
    bool getResetEnabledState();
    void setResetEnabledState(bool newState);

    void setUSBDescriptor(const std::string& usbDescriptor);
    std::string getUSBDescriptor();

    const std::string& getAlpacaScript();
    void setAlpacaScript(const std::string& alpacaScript);

    void setFilePath(const std::string& filePath);
    std::string filePath();

    void setSupportedFirmwareVer(const std::vector<uint32_t>& firmwareList);
    std::vector<uint32_t> supportedFirmwareVer();

protected:
    virtual void cascadeTabDelete(const std::string& deleteMe);
    virtual void cascadeTabRename(const std::string& oldName, const std::string& newName);

    virtual bool read(const nlohmann::json& parentLevel);
    virtual void write(nlohmann::json& parentLevel);

    void defaultAlpacaScript();
    void defaultScriptVariables();

    bool                    _dirty{false};
    static std::string      _lastError;
    static bool             _dynamicConfigurationsInitialized;

    std::string             _platformPath;
    std::string             _platformFile;
    PlatformID              _platformId{static_cast<PlatformID>(kDefaultPlatformId)};
    uint32_t                _fileVersion{0};
    uint32_t                _pineVersion{1};
    std::string             _name;
    std::string             _author;
    std::string             _description;
    std::string             _creationDate;
    std::string             _modifyDate;
    DebugBoardType          _platform{eUnknownDebugBoard};
    std::string             _usbDescriptor;
    std::string             _alpacaScript;
    std::vector<uint32_t>   _supportedFirmwareVer;

    Tabs                    _tabs;
    Buttons                 _buttons;
    ScriptVariables         _scriptVariables;
    TACSize                 _formDimension;
    bool                    _resetActive{false};
    bool                    _resetEnabled{false};

    static TACPlatformEntries _tacPlatformEntries;

private:
    static void initialize();
    void copyToPineDataPath(const std::string& savePath);

    static USBDescriptors   _usbDescriptors;
    static Buttons          _classicButtons;
};

#endif // PLATFORMCONFIGURATION_H
