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
	Author: Michael Simpson (msimpson@qti.qualcomm.com)
*/

#include "USBDescriptors.h"

#include "ConsoleApplicationEnhancements.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

// JSON key constants
static const char* kModifyDate    = "modification_date";
static const char* kUSBCatalog    = "catalog";
static const char* kDebugBoardType= "debugBoardType";
static const char* kPlatformId    = "platform_id";
static const char* kName          = "name";
static const char* kDescription   = "description";
static const char* kUSBDescriptor = "usb_descriptor";
static const char* kRevisionNumber= "revision";
static const char* kConfigFilePath= "configPath";
static const char* kChip1BusSet   = "chip1BusSet";
static const char* kChip2BusSet   = "chip2BusSet";
static const char* kChip3BusSet   = "chip3BusSet";
static const char* kChip4BusSet   = "chip4BusSet";

static std::string currentDateTimeString()
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

USBDescriptors::USBDescriptors()
{
    _modifyDate = currentDateTimeString();
}

DescriptorList USBDescriptors::getDescriptors()
{
    return _descriptorList;
}

DescriptorList USBDescriptors::getDescriptorsByBoardType(DebugBoardType debugBoardType)
{
    DescriptorList result;
    for (const auto& d : _descriptorList)
        if (d._debugBoardType == debugBoardType)
            result.push_back(d);
    return result;
}

USBDescriptor USBDescriptors::getPlatformDescriptor(PlatformID platformID)
{
    for (const auto& d : _descriptorList)
        if (d._platformID == platformID)
            return d;
    return {};
}

PlatformID USBDescriptors::getPlatformId(const std::string& usbDescriptor)
{
    std::string lower;
    for (auto c : usbDescriptor)
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    for (const auto& d : _descriptorList)
    {
        std::string candidate;
        for (auto c : d._usbDescriptor)
            candidate += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (candidate == lower)
            return d._platformID;
    }
    return 0;
}

void USBDescriptors::addUSBDescriptor(USBDescriptor usbDescriptor)
{
    _descriptorList.push_back(usbDescriptor);
}

bool USBDescriptors::load(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        _lastError = "Unable to open platform configuration file " + filePath;
        return false;
    }

    json doc;
    try
    {
        file >> doc;
    }
    catch (const json::parse_error& e)
    {
        _lastError = std::string("Error parsing configuration file: ") + e.what();
        return false;
    }

    if (doc.contains(kModifyDate) && doc[kModifyDate].is_string())
        _modifyDate = doc[kModifyDate].get<std::string>();

    if (!doc.contains(kUSBCatalog) || !doc[kUSBCatalog].is_array())
        return true;

    // Helper lambdas for type-safe JSON reads
    auto safeInt = [](const json& v, int def = 0) -> int {
        if (v.is_number()) return v.get<int>();
        if (v.is_string()) { try { return std::stoi(v.get<std::string>()); } catch (...) {} }
        return def;
    };
    auto safeStr = [](const json& v) -> std::string {
        if (v.is_string()) return v.get<std::string>();
        if (v.is_number_integer()) return std::to_string(v.get<int64_t>());
        if (v.is_number_float()) return std::to_string(v.get<double>());
        return "";
    };

    _descriptorList.clear();
    for (const auto& entry : doc[kUSBCatalog])
    {
        if (!entry.contains(kPlatformId)) continue;

        USBDescriptor d;
        d._platformID = safeInt(entry[kPlatformId]);

        if (entry.contains(kDebugBoardType))
            d._debugBoardType = static_cast<DebugBoardType>(safeInt(entry[kDebugBoardType]));

        if (entry.contains(kUSBDescriptor))
            d._usbDescriptor = safeStr(entry[kUSBDescriptor]);

        if (entry.contains(kName))
            d._name = safeStr(entry[kName]);

        if (entry.contains(kDescription))
            d._description = safeStr(entry[kDescription]);

        if (entry.contains(kRevisionNumber))
            d._revision = static_cast<uint32_t>(safeInt(entry[kRevisionNumber]));

        if (entry.contains(kConfigFilePath))
        {
            d._configurationFilePath = safeStr(entry[kConfigFilePath]);
#ifdef __linux__
            d._configurationFilePath = expandPath(d._configurationFilePath);
#endif
        }

        if (d._debugBoardType == eFTDI)
        {
            if (entry.contains(kChip1BusSet)) d._pinSets[0] = static_cast<FTDIPinSet>(safeInt(entry[kChip1BusSet]));
            if (entry.contains(kChip2BusSet)) d._pinSets[1] = static_cast<FTDIPinSet>(safeInt(entry[kChip2BusSet]));
            if (entry.contains(kChip3BusSet)) d._pinSets[2] = static_cast<FTDIPinSet>(safeInt(entry[kChip3BusSet]));
            if (entry.contains(kChip4BusSet)) d._pinSets[3] = static_cast<FTDIPinSet>(safeInt(entry[kChip4BusSet]));
        }

        _descriptorList.push_back(d);
    }

    return true;
}

bool USBDescriptors::save(const std::string& filePath)
{
    json doc;
    doc[kModifyDate] = _modifyDate;

    json catalog = json::array();
    for (const auto& d : _descriptorList)
    {
        json entry;
        entry[kPlatformId]    = static_cast<int>(d._platformID);
        entry[kDebugBoardType]= static_cast<int>(d._debugBoardType);
        entry[kUSBDescriptor] = d._usbDescriptor;
        entry[kName]          = d._name;
        entry[kDescription]   = d._description;
        entry[kRevisionNumber]= static_cast<int>(d._revision);
        entry[kConfigFilePath]= d._configurationFilePath;

        if (d._debugBoardType == eFTDI)
        {
            entry[kChip1BusSet] = static_cast<int>(d._pinSets[0]);
            entry[kChip2BusSet] = static_cast<int>(d._pinSets[1]);
            entry[kChip3BusSet] = static_cast<int>(d._pinSets[2]);
            entry[kChip4BusSet] = static_cast<int>(d._pinSets[3]);
        }
        catalog.push_back(entry);
    }
    doc[kUSBCatalog] = catalog;

    std::ofstream file(filePath);
    if (!file.is_open()) return false;
    file << doc.dump(4);
    return true;
}

std::string USBDescriptors::getLastError()
{
    return _lastError;
}

