/*
	Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
	SPDX-License-Identifier: BSD-3-Clause

	SerialPortInfo implementation using libserialport + Windows registry fallback.

	libserialport's sp_get_port_transport() does not always detect USB transport
	for ports using the generic Windows usbser.sys driver (shows as "USB Serial
	Device" instead of "USB Serial Port"). In that case, VID/PID comes back as 0.

	The fallback reads VID/PID from the Windows device registry (SetupDI), which
	is exactly what Qt's QSerialPortInfo does internally.
*/

#include "SerialPortInfo.h"

#include <libserialport.h>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#pragma comment(lib, "setupapi.lib")

// Read VID/PID from the Windows registry for a given COM port name.
// Looks at the hardware ID string (e.g., "USB\VID_05C6&PID_9302\...")
static bool readVidPidFromRegistry(const std::string& portName, uint16_t& vid, uint16_t& pid)
{
    vid = 0;
    pid = 0;

    HDEVINFO devInfo = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_COMPORT, nullptr, nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devInfo == INVALID_HANDLE_VALUE)
        return false;

    SP_DEVINFO_DATA devInfoData{};
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(devInfo, i, &devInfoData); ++i)
    {
        // Get the port name from the registry
        HKEY hKey = SetupDiOpenDevRegKey(devInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        if (hKey == INVALID_HANDLE_VALUE)
            continue;

        char portNameBuf[256]{};
        DWORD portNameSize = sizeof(portNameBuf);
        DWORD type = 0;
        LONG res = RegQueryValueExA(hKey, "PortName", nullptr, &type,
            reinterpret_cast<LPBYTE>(portNameBuf), &portNameSize);
        RegCloseKey(hKey);

        if (res != ERROR_SUCCESS || type != REG_SZ)
            continue;

        if (_stricmp(portNameBuf, portName.c_str()) != 0)
            continue;

        // Found our port — now read the Hardware ID to extract VID/PID
        char hwIdBuf[512]{};
        DWORD hwIdSize = sizeof(hwIdBuf);
        if (!SetupDiGetDeviceRegistryPropertyA(devInfo, &devInfoData, SPDRP_HARDWAREID,
            nullptr, reinterpret_cast<PBYTE>(hwIdBuf), hwIdSize, &hwIdSize))
            break;

        // Parse "USB\VID_XXXX&PID_YYYY" from hardware ID
        std::string hwId(hwIdBuf);
        auto vidPos = hwId.find("VID_");
        auto pidPos = hwId.find("PID_");
        if (vidPos != std::string::npos && pidPos != std::string::npos)
        {
            vid = static_cast<uint16_t>(std::stoul(hwId.substr(vidPos + 4, 4), nullptr, 16));
            pid = static_cast<uint16_t>(std::stoul(hwId.substr(pidPos + 4, 4), nullptr, 16));
        }

        // Also try to read serial number if not already known
        break;
    }

    SetupDiDestroyDeviceInfoList(devInfo);
    return vid != 0;
}

// Read serial number from registry for a COM port (fallback when libserialport returns empty)
static std::string readSerialFromRegistry(const std::string& portName)
{
    HDEVINFO devInfo = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_COMPORT, nullptr, nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devInfo == INVALID_HANDLE_VALUE)
        return {};

    SP_DEVINFO_DATA devInfoData{};
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    std::string result;

    for (DWORD i = 0; SetupDiEnumDeviceInfo(devInfo, i, &devInfoData); ++i)
    {
        HKEY hKey = SetupDiOpenDevRegKey(devInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        if (hKey == INVALID_HANDLE_VALUE)
            continue;

        char portNameBuf[256]{};
        DWORD portNameSize = sizeof(portNameBuf);
        DWORD type = 0;
        LONG res = RegQueryValueExA(hKey, "PortName", nullptr, &type,
            reinterpret_cast<LPBYTE>(portNameBuf), &portNameSize);
        RegCloseKey(hKey);

        if (res != ERROR_SUCCESS || _stricmp(portNameBuf, portName.c_str()) != 0)
            continue;

        // Get the device instance ID — often contains the serial number
        char instanceId[256]{};
        if (SetupDiGetDeviceInstanceIdA(devInfo, &devInfoData, instanceId, sizeof(instanceId), nullptr))
        {
            // Instance ID format: "USB\VID_XXXX&PID_YYYY\SERIALNUMBER"
            std::string id(instanceId);
            auto lastSlash = id.rfind('\\');
            if (lastSlash != std::string::npos)
            {
                std::string serial = id.substr(lastSlash + 1);
                // Skip if it looks like a Windows-generated instance ID (contains '&')
                if (serial.find('&') == std::string::npos)
                    result = serial;
            }
        }
        break;
    }

    SetupDiDestroyDeviceInfoList(devInfo);
    return result;
}

#endif // _WIN32

SerialPortInfos SerialPortInfo::availablePorts()
{
    SerialPortInfos result;

    sp_port** portList = nullptr;
    if (sp_list_ports(&portList) != SP_OK || portList == nullptr)
        return result;

    for (int i = 0; portList[i] != nullptr; ++i)
    {
        SerialPortInfo info;
        const char* name = sp_get_port_name(portList[i]);
        const char* desc = sp_get_port_description(portList[i]);
        if (name) info.setPortName(name);
        if (desc) info.setDescription(desc);

#ifdef _WIN32
        // On Windows: always use the SetupDI registry API for VID/PID and serial number.
        // This matches Qt's QSerialPortInfo behavior and works for all USB serial devices
        // regardless of driver type (FTDI, usbser.sys, etc.).
        if (name)
        {
            uint16_t regVid = 0, regPid = 0;
            if (readVidPidFromRegistry(name, regVid, regPid))
            {
                info.setVendorIdentifier(regVid);
                info.setProductIdentifier(regPid);
            }

            std::string regSerial = readSerialFromRegistry(name);
            if (!regSerial.empty())
                info.setSerialNumber(regSerial);
        }
#else
        // On Linux: libserialport reads from sysfs which works correctly
        const char* serial = sp_get_port_usb_serial(portList[i]);
        if (serial) info.setSerialNumber(serial);

        int vid = 0, pid = 0;
        if (sp_get_port_transport(portList[i]) == SP_TRANSPORT_USB)
        {
            sp_get_port_usb_vid_pid(portList[i], &vid, &pid);
            info.setVendorIdentifier(static_cast<uint16_t>(vid));
            info.setProductIdentifier(static_cast<uint16_t>(pid));
        }
#endif

        result.push_back(info);
    }

    sp_free_port_list(portList);
    return result;
}

bool equal(const SerialPortInfos& si1, const SerialPortInfos& si2)
{
    if (si1.size() != si2.size()) return false;
    for (size_t i = 0; i < si1.size(); ++i)
        if (!(si1[i] == si2[i])) return false;
    return true;
}
