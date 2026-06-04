/*
	Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
	SPDX-License-Identifier: BSD-3-Clause

	SerialPortInfo enumeration (platform-independent).

	The enumeration loop below is shared across all platforms. Filling in VID/PID
	and serial number for each port is platform-divergent and is delegated to
	populatePlatformDetails(), implemented in SerialPortInfo_win.cpp (Windows
	SetupDI registry) or SerialPortInfo_posix.cpp (libserialport sysfs). CMake
	selects the correct .cpp at build time.
*/

#include "SerialPortInfo.h"
#include "SerialPortInfoPlatform.h"

#include <libserialport.h>

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

        populatePlatformDetails(info, portList[i], name);

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
