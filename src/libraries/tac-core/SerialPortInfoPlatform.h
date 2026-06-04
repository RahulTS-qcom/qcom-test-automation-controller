#ifndef SERIALPORTINFOPLATFORM_H
#define SERIALPORTINFOPLATFORM_H
/*
	Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
	SPDX-License-Identifier: BSD-3-Clause
*/

/*
	Internal (non-exported) declaration for the platform-divergent part of
	SerialPortInfo enumeration. The shared enumeration loop lives in
	SerialPortInfo.cpp; the implementation of this helper is selected at build
	time — SerialPortInfo_win.cpp (Windows SetupDI registry) or
	SerialPortInfo_posix.cpp (libserialport sysfs). See CMakeLists.txt.
*/

// libserialport forward declaration — avoids including the full header here.
struct sp_port;

class SerialPortInfo;

// Fill in VID/PID and serial number for a single enumerated port. The port
// name is passed pre-extracted (it may be null for unnamed ports).
void populatePlatformDetails(SerialPortInfo& info, sp_port* port, const char* name);

#endif // SERIALPORTINFOPLATFORM_H
