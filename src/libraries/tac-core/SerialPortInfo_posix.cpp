/*
	Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
	SPDX-License-Identifier: BSD-3-Clause

	SerialPortInfo platform details — POSIX implementation (Linux/macOS).

	On these platforms libserialport reads USB metadata from the OS (sysfs on
	Linux, IOKit on macOS) and reports it through its own portable API, so no
	platform-specific syscalls are needed here.
*/

#include "SerialPortInfo.h"
#include "SerialPortInfoPlatform.h"

#include <libserialport.h>

void populatePlatformDetails(SerialPortInfo& info, sp_port* port, const char* /*name*/)
{
    const char* serial = sp_get_port_usb_serial(port);
    if (serial) info.setSerialNumber(serial);

    int vid = 0, pid = 0;
    if (sp_get_port_transport(port) == SP_TRANSPORT_USB)
    {
        sp_get_port_usb_vid_pid(port, &vid, &pid);
        info.setVendorIdentifier(static_cast<uint16_t>(vid));
        info.setProductIdentifier(static_cast<uint16_t>(pid));
    }
}
