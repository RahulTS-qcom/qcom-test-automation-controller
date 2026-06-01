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
			Biswajit Roy (biswroy@qti.qualcomm.com)
*/

#include "TACDev.h"
#include "TACDevCore.h"

#include "TACDefines.h"
#include "AlpacaDevice.h"
#include "FTDIDevice.h"
#include "TACException.h"

// QCommon
#include "version.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>

// ── Debug trace ──────────────────────────────────────────────────────────────
// Uses consolidated TACDebugLog (DebugLog.h in tac-core).
// Enable by setting env var TACDEV_DEBUG=1 before running.
#include "DebugLog.h"
#define TACDEV_DBG(msg) TACDEV_DBG_TAG("TACDev", msg)
// ─────────────────────────────────────────────────────────────────────────────

DevTACCore gDevTACCore;

const std::string kTACDevHandleNotOpen("TAC device is not open. Please reopen the TAC device");
const std::string kTACDevBufferTooSmall("TACDev buffer is too small");
const std::string kTACBadIndex("User provided invalid index");


TAC_RESULT InitializeTACDev()
{
	static std::once_flag initFlag;
	static TAC_RESULT initResult{NO_TAC_ERROR};

	std::call_once(initFlag, []() {
		TACDEV_DBG("Initializing TACDev...");
		if (gDevTACCore.initialize(kAppName, kAppVersion) == false)
		{
			initResult = TACDEV_INIT_FAILED;
			TACDEV_DBG("FAILED to initialize");
		}
		else
		{
			TACDEV_DBG("Initialized OK");
			const char* cfgPath = std::getenv("TACDEV_CONFIG_PATH");
			TACDEV_DBG(std::string("TACDEV_CONFIG_PATH=") + (cfgPath ? cfgPath : "(null)"));
		}
	});

	return initResult;
}

TAC_RESULT _getCommandState
(
	TAC_HANDLE tacHandle,
	const std::string& command,
	bool *state
)
{
	TAC_RESULT result{NO_TAC_ERROR};

	AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
	if (alpacaDevice != nullptr)
	{
		try
		{
			*state = alpacaDevice->getCommandState(command);
		}
		catch (const TACException& e)
		{
			result = e.errorCode();
		}
	}
	else
	{
		result = TACDEV_BAD_TAC_HANDLE;
		gDevTACCore.setLastError(kTACDevHandleNotOpen);
	}

	return result;
}

TAC_RESULT _setCommandState
(
	TAC_HANDLE tacHandle,
	const std::string& command,
	bool state
)
{
	TAC_RESULT result{NO_TAC_ERROR};
	bool valid{false};

	AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
	if (alpacaDevice != nullptr)
	{
		try
		{
			alpacaDevice->setWaitForCompletion();
			valid = alpacaDevice->sendCommand(command, state);

			if (valid == false)
			{
				result = TACDEV_BAD_TAC_HANDLE;
				gDevTACCore.setLastError(kTACDevHandleNotOpen);
			}
		}
		catch (const TACException& e)
		{
			result = e.errorCode();
			gDevTACCore.setLastError(e.getMessage());
		}
	}
	else
	{
		result = TACDEV_BAD_TAC_HANDLE;
		gDevTACCore.setLastError(kTACDevHandleNotOpen);
	}

	return result;
}

TAC_RESULT _quickCommand
(
	TAC_HANDLE tacHandle,
	const std::string& command
)
{
	TAC_RESULT result{NO_TAC_ERROR};

	AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
	if (alpacaDevice != nullptr)
	{
		try
		{
			alpacaDevice->quickCommand(command);
		}
		catch (const TACException& e)
		{
			result = e.errorCode();
			gDevTACCore.setLastError(e.getMessage());
		}
	}
	else
	{
		result = TACDEV_BAD_TAC_HANDLE;
		gDevTACCore.setLastError(kTACDevHandleNotOpen);
	}

	return result;
}

TAC_RESULT GetAlpacaVersion
(
	char* alpacaVersion,
	int bufferSize
)
{
	TAC_RESULT result{NO_TAC_ERROR};

	std::string version{ALPACA_VERSION};

	if (static_cast<int>(version.size()) < bufferSize)
	{
		std::memset(alpacaVersion, 0, bufferSize);
		std::memcpy(alpacaVersion, version.data(), version.size());
	}
	else
	{
		result = static_cast<TAC_RESULT>(version.size());
	}

	return result;
}

TAC_RESULT GetTACVersion(char* tacVersion, int bufferSize)
{
	TAC_RESULT result{NO_TAC_ERROR};

	std::string version{TAC_VERSION};

	if (static_cast<int>(version.size()) < bufferSize)
	{
		std::memset(tacVersion, 0, bufferSize);
		std::memcpy(tacVersion, version.data(), version.size());
	}
	else
	{
		result = static_cast<TAC_RESULT>(version.size());
	}

	return result;
}

TAC_RESULT GetLastTACError
(
	char* lastError,
	int bufferSize
)
{
	TAC_RESULT result{NO_TAC_ERROR};

	std::string lastErrorStr = gDevTACCore.lastError();
	if (static_cast<int>(lastErrorStr.size()) < bufferSize)
	{
		std::memset(lastError, 0, bufferSize);
		std::memcpy(lastError, lastErrorStr.data(), lastErrorStr.size());
	}
	else
	{
		result = TACDEV_BUFFER_TOO_SMALL;
		gDevTACCore.setLastError(kTACDevBufferTooSmall);
	}

	return result;
}

TAC_RESULT GetLoggingState
(
	bool* loggingState
)
{
	*loggingState = gDevTACCore.getLoggingState();

	return NO_TAC_ERROR;
}

TAC_RESULT SetLoggingState(bool loggingState)
{
	gDevTACCore.setLoggingState(loggingState);

	return NO_TAC_ERROR;
}

unsigned long GetDeviceCount
(
	int* deviceCount
)
{
	InitializeTACDev();

	TACDEV_DBG("Called");
	try
	{
		auto result = gDevTACCore.GetDeviceCount(deviceCount);
		TACDEV_DBG("Result=" + std::to_string(result) + " count=" + std::to_string(deviceCount ? *deviceCount : -1));
		return result;
	}
	catch (const TACException& e)
	{
		TACDEV_DBG(std::string("TACException: ") + e.getMessage());
		gDevTACCore.setLastError(std::string("GetDeviceCount: ") + e.getMessage());
		if (deviceCount) *deviceCount = 0;
		return TACDEV_INIT_FAILED;
	}
	catch (const std::exception& e)
	{
		TACDEV_DBG(std::string("std::exception: ") + e.what());
		gDevTACCore.setLastError(std::string("GetDeviceCount: ") + e.what());
		if (deviceCount) *deviceCount = 0;
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		TACDEV_DBG("Unknown exception caught");
		gDevTACCore.setLastError("GetDeviceCount: Unknown exception");
		if (deviceCount) *deviceCount = 0;
		return TACDEV_INIT_FAILED;
	}
}

unsigned long GetPortData
(
	int deviceIndex,
	char* portData,
	int bufferSize
)
{
	try
	{
		int result{0};

		const AlpacaDevices& alpacaDevices = gDevTACCore.GetAlpacaDevices();
		if (deviceIndex < static_cast<int>(alpacaDevices.size()))
		{
			AlpacaDevice alpacaDevice = alpacaDevices.at(deviceIndex);
			std::string portData2;

			portData2 = alpacaDevice->portName();
			portData2 += ";";
			portData2 += alpacaDevice->description();
			portData2 += ";";
			portData2 += alpacaDevice->serialNumber();
			portData2 += ";";
			portData2 += std::to_string(deviceIndex);

			std::memset(portData, 0, bufferSize);
			std::memcpy(portData, portData2.c_str(), (std::min)(static_cast<size_t>(bufferSize - 1), portData2.size()));
			result = static_cast<int>(portData2.size());
		}

		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("GetPortData: ") + e.what());
		return 0;
	}
	catch (...)
	{
		gDevTACCore.setLastError("GetPortData: Unknown exception");
		return 0;
	}
}

TAC_HANDLE OpenHandleByDescription
(
	const char* portName
)
{
	TACDEV_DBG(std::string("portName='") + (portName ? portName : "(null)") + "'");
	try
	{
		TAC_HANDLE result = gDevTACCore.OpenHandleByDescription(portName);
		if (result != 0)
			TACDEV_DBG("Opened OK handle=" + std::to_string(result));
		else
			TACDEV_DBG("FAILED - error: " + gDevTACCore.lastError());
		return result;
	}
	catch (const TACException& e)
	{
		TACDEV_DBG(std::string("TACException: ") + e.getMessage());
		gDevTACCore.setLastError(e.getMessage());
		return 0;
	}
	catch (const std::exception& e)
	{
		TACDEV_DBG(std::string("std::exception: ") + e.what());
		gDevTACCore.setLastError(std::string("OpenHandleByDescription: ") + e.what());
		return 0;
	}
	catch (...)
	{
		TACDEV_DBG("Unknown exception");
		gDevTACCore.setLastError("OpenHandleByDescription: Unknown exception");
		return 0;
	}
}

TAC_RESULT CloseTACHandle
(
	TAC_HANDLE tacHandle
)
{
	try
	{
		return gDevTACCore.CloseTACHandle(tacHandle);
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("CloseTACHandle: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("CloseTACHandle: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}

TAC_RESULT ProgramFTDIDevice
(
	int deviceIndex,
	char* errorBuffer,
	int bufferSize
)
{
	TACDEV_DBG("ProgramFTDIDevice index=" + std::to_string(deviceIndex));
	InitializeTACDev();

	try
	{
		const AlpacaDevices& devices = gDevTACCore.GetAlpacaDevices();
		if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices.size()))
		{
			gDevTACCore.setLastError("Invalid device index");
			return TACDEV_BAD_INDEX;
		}

		AlpacaDevice device = devices.at(deviceIndex);
		if (device->debugBoardType() != eFTDI)
		{
			gDevTACCore.setLastError("Device is not an FTDI board");
			return TACDEV_COMMAND_NOT_FOUND;
		}

		std::string errMsg;
		bool ok = FTDIDevice::programDevice(device, device->platformID(), errMsg);

		if (ok)
		{
			TACDEV_DBG("Programmed OK");
			return NO_TAC_ERROR;
		}
		else
		{
			TACDEV_DBG("Program failed: " + errMsg);
			gDevTACCore.setLastError(errMsg);
			if (errorBuffer && bufferSize > 0)
			{
				size_t len = (std::min)(errMsg.size(), static_cast<size_t>(bufferSize - 1));
				std::memcpy(errorBuffer, errMsg.c_str(), len);
				errorBuffer[len] = '\0';
			}
			return TACDEV_INIT_FAILED;
		}
	}
	catch (const std::exception& e)
	{
		TACDEV_DBG(std::string("Exception: ") + e.what());
		gDevTACCore.setLastError(std::string("ProgramFTDIDevice: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		TACDEV_DBG("Unknown exception");
		gDevTACCore.setLastError("ProgramFTDIDevice: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}

TAC_RESULT GetName
(
	TAC_HANDLE tacHandle,
	char* deviceName,
	int bufferSize
)
{
	try
	{
		TAC_RESULT result{NO_TAC_ERROR};

		AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
		if (alpacaDevice != nullptr)
		{
			std::string name = alpacaDevice->name();
			if (static_cast<int>(name.size()) >= bufferSize)
			{
				result = TACDEV_BUFFER_TOO_SMALL;
				gDevTACCore.setLastError(kTACDevBufferTooSmall);
			}
			else
			{
				std::memset(deviceName, 0, bufferSize);
				std::memcpy(deviceName, name.data(), name.size());
			}
		}
		else
		{
			if (bufferSize > 0) deviceName[0] = '\0';
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}

		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("GetName: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("GetName: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}

TAC_RESULT GetFirmwareVersion
(
	TAC_HANDLE tacHandle,
	char* firmwareVersion,
	int bufferSize
)
{
	try
	{
		TAC_RESULT result{NO_TAC_ERROR};

		AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
		if (alpacaDevice != nullptr)
		{
			std::string firmwareVer = alpacaDevice->firmwareVersion();
			if (static_cast<int>(firmwareVer.size()) >= bufferSize)
			{
				result = TACDEV_BUFFER_TOO_SMALL;
				gDevTACCore.setLastError(kTACDevBufferTooSmall);
			}
			else
			{
				std::memset(firmwareVersion, 0, bufferSize);
				std::memcpy(firmwareVersion, firmwareVer.data(), firmwareVer.size());
			}
		}
		else
		{
			if (bufferSize > 0) firmwareVersion[0] = '\0';
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}

		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("GetFirmwareVersion: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("GetFirmwareVersion: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}


TAC_RESULT GetHardware
(
	TAC_HANDLE tacHandle,
	char* hardware,
	int bufferSize
)
{
	try
	{
		TAC_RESULT result{NO_TAC_ERROR};

		AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
		if (alpacaDevice != nullptr)
		{
			std::string hardwareType = alpacaDevice->debugBoardTypeString();
			if (static_cast<int>(hardwareType.size()) >= bufferSize)
			{
				result = TACDEV_BUFFER_TOO_SMALL;
				gDevTACCore.setLastError(kTACDevBufferTooSmall);
			}
			else
			{
				std::memset(hardware, 0, bufferSize);
				std::memcpy(hardware, hardwareType.data(), hardwareType.size());
			}
		}
		else
		{
			if (bufferSize > 0) hardware[0] = '\0';
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}

		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("GetHardware: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("GetHardware: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}

TAC_RESULT GetHardwareVersion
(
	TAC_HANDLE tacHandle,
	char* hardwareVersion,
	int bufferSize
)
{
	try
	{
		TAC_RESULT result{NO_TAC_ERROR};

		AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
		if (alpacaDevice != nullptr)
		{
			std::string hardwareVersionStr = alpacaDevice->hardwareVersionString();
			if (static_cast<int>(hardwareVersionStr.size()) >= bufferSize)
			{
				result = TACDEV_BUFFER_TOO_SMALL;
				gDevTACCore.setLastError(kTACDevBufferTooSmall);
			}
			else
			{
				std::memset(hardwareVersion, 0, bufferSize);
				std::memcpy(hardwareVersion, hardwareVersionStr.data(), hardwareVersionStr.size());
			}
		}
		else
		{
			if (bufferSize > 0) hardwareVersion[0] = '\0';
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}

		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("GetHardwareVersion: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("GetHardwareVersion: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}

TAC_RESULT GetUUID
(
	TAC_HANDLE tacHandle,
	char* uuid,
	int bufferSize
)
{
	try
	{
		TAC_RESULT result{NO_TAC_ERROR};

		AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
		if (alpacaDevice != nullptr)
		{
			std::string uuidStr = alpacaDevice->uuid();
			if (static_cast<int>(uuidStr.size()) >= bufferSize)
			{
				result = TACDEV_BUFFER_TOO_SMALL;
				gDevTACCore.setLastError(kTACDevBufferTooSmall);
			}
			else
			{
				std::memset(uuid, 0, bufferSize);
				std::memcpy(uuid, uuidStr.data(), uuidStr.size());
			}
		}
		else
		{
			if (bufferSize > 0) uuid[0] = '\0';
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}

		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("GetUUID: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("GetUUID: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}

TAC_ERROR SetExternalPowerControl
(
	TAC_HANDLE tacHandle,
	bool state
)
{
	try
	{
		TAC_RESULT result{NO_TAC_ERROR};

		AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
		if (alpacaDevice != nullptr)
		{
			alpacaDevice->externalPowerControl(state);
		}
		else
		{
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}

		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("SetExternalPowerControl: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("SetExternalPowerControl: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}

TAC_RESULT SetBatteryState
(
	TAC_HANDLE tacHandle,
	bool state
)
{
	return _setCommandState(tacHandle, "battery", state);
}

TAC_RESULT GetBatteryState(TAC_HANDLE tacHandle, bool *state)
{
	return _getCommandState(tacHandle, "battery", state);
}

TAC_RESULT Usb0
(
	TAC_HANDLE tacHandle,
	bool state
)
{
	return _setCommandState(tacHandle, "usb0", state);
}

TAC_RESULT GetUsb0State
(
	TAC_HANDLE tacHandle,
	bool* state
)
{
	return _getCommandState(tacHandle, "usb0", state);
}

TAC_RESULT Usb1
(
	TAC_HANDLE tacHandle,
	bool state
)
{
	return _setCommandState(tacHandle, "usb1", state);
}

TAC_RESULT GetUsb1State
(
	TAC_HANDLE tacHandle,
	bool* state
)
{
	return _getCommandState(tacHandle, "usb1", state);
}

TAC_RESULT PowerKey
(
	TAC_HANDLE tacHandle,
	bool state
)
{
	return _setCommandState(tacHandle, "pkey", state);
}

TAC_RESULT GetPowerKeyState
(
	TAC_HANDLE tacHandle,
	bool* state
)
{
	return _getCommandState(tacHandle, "pkey", state);
}

TAC_RESULT VolumeUp
(
	TAC_HANDLE tacHandle,
	bool state
)
{
	return _setCommandState(tacHandle, "volup", state);
}

TAC_RESULT GetVolumeUpState
(
	TAC_HANDLE tacHandle,
	bool* state
)
{
	return _getCommandState(tacHandle, "volup", state);
}

TAC_RESULT VolumeDown
(
	TAC_HANDLE tacHandle,
	bool state
)
{
	return _setCommandState(tacHandle, "voldn", state);
}

TAC_RESULT GetVolumeDownState
(
	TAC_HANDLE tacHandle,
	bool* state
)
{
	return _getCommandState(tacHandle, "voldn", state);
}

TAC_RESULT DisconnectUIM1
(
	TAC_HANDLE tacHandle,
	bool state
)
{
	return _setCommandState(tacHandle, "uim1", state);
}

TAC_RESULT GetDisconnectUIM1State
(
	TAC_HANDLE tacHandle,
	bool* state
)
{
	return _getCommandState(tacHandle, "uim1", state);
}

TAC_RESULT DisconnectUIM2
(
	TAC_HANDLE tacHandle,
	bool state
)
{
	return _setCommandState(tacHandle, "uim2", state);
}

TAC_RESULT GetDisconnectUIM2State
(
	TAC_HANDLE tacHandle,
	bool* state
)
{
	return _getCommandState(tacHandle, "uim2", state);
}

TAC_RESULT DisconnectSDCard
(
	TAC_HANDLE tacHandle,
	bool state
)
{
	return _setCommandState(tacHandle, "sdcard", state);
}

TAC_RESULT GetDisconnectSDCardState
(
	TAC_HANDLE tacHandle,
	bool* state
)
{
	return _getCommandState(tacHandle, "sdcard", state);
}

TAC_RESULT PrimaryEDL
(
	TAC_HANDLE tacHandle,
	bool state
)
{
	return _setCommandState(tacHandle, "pedl", state);
}

TAC_RESULT GetPrimaryEDLState
(
	TAC_HANDLE tacHandle,
	bool* state
)
{
	return _getCommandState(tacHandle, "pedl", state);
}

TAC_RESULT ForcePSHoldHigh
(
	TAC_HANDLE tacHandle,
	bool state
)
{
	return _setCommandState(tacHandle, "pshold", state);
}

TAC_RESULT GetForcePSHoldHighState
(
	TAC_HANDLE tacHandle,
	bool* state
)
{
	return _getCommandState(tacHandle, "pshold", state);
}

TAC_RESULT SecondaryEDL
(
	TAC_HANDLE tacHandle,
	bool state
)
{
	return _setCommandState(tacHandle, "sedl", state);
}

TAC_RESULT GetSecondaryEDLState
(
	TAC_HANDLE tacHandle,
	bool* state
)
{
	return _getCommandState(tacHandle, "sedl", state);
}

TAC_RESULT SecondaryPmResinN
(
	TAC_HANDLE tacHandle,
	bool state
)
{
	return _setCommandState(tacHandle, "sresn", state);
}

TAC_RESULT GetSecondaryPmResinNState
(
	TAC_HANDLE tacHandle,
	bool* state
)
{
	return _getCommandState(tacHandle, "sresn", state);
}

TAC_RESULT Eud
(
	TAC_HANDLE tacHandle,
	bool state
)
{
	return _setCommandState(tacHandle, "eud", state);
}

TAC_RESULT GetEUDState
(
	TAC_HANDLE tacHandle,
	bool* state
)
{
	return _getCommandState(tacHandle, "eud", state);
}

TAC_RESULT HeadsetDisconnect
(
	TAC_HANDLE tacHandle,
	bool state
)
{
	return _setCommandState(tacHandle, "headset", state);
}

TAC_RESULT GetHeadsetDisconnectState
(
	TAC_HANDLE tacHandle,
	bool* state
)
{
	return _getCommandState(tacHandle, "headset", state);
}

TAC_RESULT SetName
(
	TAC_HANDLE tacHandle,
	const char* newName
)
{
	try
	{
		TAC_RESULT result{NO_TAC_ERROR};

		AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
		if (alpacaDevice != nullptr)
		{
			alpacaDevice->setWaitForCompletion();
			alpacaDevice->setName(std::string(newName));
		}
		else
		{
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}

		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("SetName: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("SetName: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}

TAC_RESULT GetResetCount
(
	TAC_HANDLE tacHandle,
	int* resetCount
)
{
	try
	{
		TAC_RESULT result{NO_TAC_ERROR};

		AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
		if (alpacaDevice != nullptr)
		{
			alpacaDevice->setWaitForCompletion();
			*resetCount = alpacaDevice->getResetCount();
		}
		else
		{
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}

		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("GetResetCount: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("GetResetCount: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}

TAC_RESULT ClearResetCount(TAC_HANDLE tacHandle)
{
	try
	{
		TAC_RESULT result{NO_TAC_ERROR};

		AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
		if (alpacaDevice != nullptr)
		{
			alpacaDevice->setWaitForCompletion();
			alpacaDevice->clearResetCount();
		}
		else
		{
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}

		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("ClearResetCount: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("ClearResetCount: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}

TAC_RESULT PowerOnButton
(
	TAC_HANDLE tacHandle
)
{
	return _quickCommand(tacHandle, "powerOn");
}

TAC_RESULT PowerOffButton
(
	TAC_HANDLE tacHandle
)
{
	return _quickCommand(tacHandle, "powerOff");
}

TAC_RESULT BootToFastBootButton
(
	TAC_HANDLE tacHandle
)
{
	return _quickCommand(tacHandle, "bootToFastboot");
}

TAC_RESULT BootToUEFIMenuButton
(
	TAC_HANDLE tacHandle
)
{
	return _quickCommand(tacHandle, "bootToUEFI");
}

TAC_RESULT BootToEDLButton
(
	TAC_HANDLE tacHandle
)
{
	return _quickCommand(tacHandle, "bootToEDL");
}

TAC_RESULT BootToSecondaryEDLButton
(
	TAC_HANDLE tacHandle
)
{
	return _quickCommand(tacHandle, "bootToSecondaryEDL");
}

TAC_ERROR SetPinState
(
	TAC_HANDLE tacHandle,
	int pin,
	bool state
)
{
	try
	{
		TAC_RESULT result{NO_TAC_ERROR};

		AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
		if (alpacaDevice != nullptr)
		{
			alpacaDevice->setWaitForCompletion();
			alpacaDevice->setPinState(pin, state);
		}
		else
		{
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}

		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("SetPinState: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("SetPinState: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}

TAC_ERROR GetCommandCount
(
	TAC_HANDLE tacHandle,
	unsigned long* commandCount
)
{
	try
	{
		TAC_RESULT result{NO_TAC_ERROR};

		*commandCount = 0;
		AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
		if (alpacaDevice != nullptr)
		{
			*commandCount = alpacaDevice->commandCount();
		}
		else
		{
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}

		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("GetCommandCount: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("GetCommandCount: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}

TAC_ERROR GetCommand
(
	TAC_HANDLE tacHandle,
	unsigned long commandIndex,
	char *commandBuffer,
	int bufferSize
)
{
	try
	{
		TAC_RESULT result{NO_TAC_ERROR};

		AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
		if (alpacaDevice != nullptr)
		{
			TACCommand commandEntry = alpacaDevice->commandEntry(commandIndex);
			if (commandEntry._pin != 0)
			{
				std::string commandData;

				commandData = commandEntry._command;
				commandData += ";";
				commandData += commandEntry._helpText;
				commandData += ";";
				commandData += std::to_string(commandEntry._pin);
				commandData += ";";
				commandData += commandEntry._tabName;
				commandData += ";";
				commandData += commandEntry._groupName;
				commandData += ";";
				commandData += commandEntry._cellLocation;

				std::memset(commandBuffer, 0, bufferSize);
				std::memcpy(commandBuffer, commandData.c_str(), (std::min)(static_cast<size_t>(bufferSize - 1), commandData.size()));
			}
			else
			{
				result = TACDEV_BAD_INDEX;
				gDevTACCore.setLastError(kTACBadIndex);
			}
		}
		else
		{
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}

		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("GetCommand: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("GetCommand: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}

TAC_ERROR GetQuickCommandCount
(
	TAC_HANDLE tacHandle,
	unsigned long* commandCount
)
{
	try
	{
		TAC_RESULT result{NO_TAC_ERROR};

		*commandCount = 0;
		AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
		if (alpacaDevice != nullptr)
		{
			*commandCount = alpacaDevice->quickCommandCount();
		}
		else
		{
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}

		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("GetQuickCommandCount: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("GetQuickCommandCount: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}

TAC_ERROR GetQuickCommand
(
	TAC_HANDLE tacHandle,
	unsigned long commandIndex,
	char* commandBuffer,
	int bufferSize
)
{
	try
	{
		TAC_RESULT result{NO_TAC_ERROR};

		AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
		if (alpacaDevice != nullptr)
		{
			std::string commandData = alpacaDevice->getQuickCommand(commandIndex);
			if (commandData.empty() == false)
			{
				std::memset(commandBuffer, 0, bufferSize);
				std::memcpy(commandBuffer, commandData.c_str(), (std::min)(static_cast<size_t>(bufferSize - 1), commandData.size()));
			}
			else
			{
				result = TACDEV_BAD_INDEX;
				gDevTACCore.setLastError(kTACBadIndex);
			}
		}
		else
		{
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}

		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("GetQuickCommand: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("GetQuickCommand: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}

TAC_ERROR GetCommandState
(
	TAC_HANDLE tacHandle,
	const char *command,
	bool* state
)
{
	TAC_RESULT result{NO_TAC_ERROR};

	AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
	if (alpacaDevice != nullptr)
	{
		try
		{
			*state = alpacaDevice->getCommandState(command);
		}
		catch (const TACException& e)
		{
			(void)e;

			result = TACDEV_COMMAND_NOT_FOUND;
		}
	}
	else
	{
		result = TACDEV_BAD_TAC_HANDLE;
		gDevTACCore.setLastError(kTACDevHandleNotOpen);
	}

	return result;
}

TAC_ERROR SendCommand
(
	TAC_HANDLE tacHandle,
	const char* command,
	bool state
)
{
	TACDEV_DBG(std::string("handle=") + std::to_string(tacHandle) +
		" cmd='" + (command ? command : "(null)") + "' state=" + (state ? "ON" : "OFF"));

	TAC_RESULT result{NO_TAC_ERROR};

	AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
	if (alpacaDevice != nullptr)
	{
		TACDEV_DBG("Device found: '" + alpacaDevice->portName() + "' active=" +
			(alpacaDevice->active() ? "yes" : "no") + " open=" + (alpacaDevice->isOpen() ? "yes" : "no"));

		bool valid;

		try
		{
			valid = alpacaDevice->sendCommand(command, state);
			if (valid == false)
			{
				result = TACDEV_BAD_TAC_HANDLE; // DriveThread is NULL
				gDevTACCore.setLastError(kTACDevHandleNotOpen);
				TACDEV_DBG("sendCommand returned false — drive thread is NULL");
			}
			else
			{
				TACDEV_DBG("sendCommand OK");
			}
		}
		catch (TACException& e)
		{
			result = e.errorCode();
			gDevTACCore.setLastError(e.getMessage());
			TACDEV_DBG(std::string("TACException: code=") + std::to_string(e.errorCode()) + " msg=" + e.getMessage());
		}
		catch (const std::exception& e)
		{
			result = TACDEV_INIT_FAILED;
			gDevTACCore.setLastError(std::string("SendCommand: ") + e.what());
			TACDEV_DBG(std::string("std::exception: ") + e.what());
		}
		catch (...)
		{
			result = TACDEV_INIT_FAILED;
			gDevTACCore.setLastError("SendCommand: Unknown exception");
			TACDEV_DBG("Unknown exception");
		}
	}
	else
	{
		result = TACDEV_BAD_TAC_HANDLE;
		gDevTACCore.setLastError(kTACDevHandleNotOpen);
		TACDEV_DBG("BAD HANDLE — device not found in open devices map");
	}

	return result;
}


TAC_ERROR GetHelpText
(
	TAC_HANDLE tacHandle,
	char* helpBuffer,
	int bufferSize,
	int* actualSize
)
{
	try
	{
		TAC_RESULT result{NO_TAC_ERROR};

		AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
		if (alpacaDevice != nullptr)
		{
			std::string helpText = alpacaDevice->getHelp();
			if (static_cast<int>(helpText.size()) >= bufferSize)
			{
				result = TACDEV_BUFFER_TOO_SMALL;
				gDevTACCore.setLastError(kTACDevBufferTooSmall);
				*actualSize = static_cast<int>(helpText.size());
			}
			else
			{
				std::memset(helpBuffer, 0, bufferSize);
				std::memcpy(helpBuffer, helpText.data(), helpText.size());
			}
		}
		else
		{
			if (bufferSize > 0) helpBuffer[0] = '\0';
			*actualSize = 0;
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}

		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("GetHelpText: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("GetHelpText: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}


TAC_ERROR GetScriptVariableCount(TAC_HANDLE tacHandle, unsigned long *scriptVariableCount)
{
	try
	{
		TAC_RESULT result{NO_TAC_ERROR};
		AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);

		if (alpacaDevice != nullptr)
			*scriptVariableCount = alpacaDevice->scriptVariableCount();
		else
		{
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}

		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("GetScriptVariableCount: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("GetScriptVariableCount: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}

TAC_ERROR GetScriptVariable(TAC_HANDLE tacHandle, unsigned long scriptVariableIndex, char *scriptVariableBuffer, int bufferSize)
{
	try
	{
		TAC_RESULT result{NO_TAC_ERROR};
		AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);

		if (alpacaDevice != nullptr)
		{
			std::string variableData = alpacaDevice->getScriptVariable(scriptVariableIndex);
			if (static_cast<int>(variableData.size()) >= bufferSize)
			{
				result = TACDEV_BUFFER_TOO_SMALL;
				gDevTACCore.setLastError(kTACDevBufferTooSmall);
			}
			else
			{
				std::memset(scriptVariableBuffer, 0, bufferSize);
				std::memcpy(scriptVariableBuffer, variableData.data(), variableData.size());
			}
		}
		else
		{
			if (bufferSize > 0) scriptVariableBuffer[0] = '\0';
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}
		return result;
	}
	catch (const std::exception& e)
	{
		gDevTACCore.setLastError(std::string("GetScriptVariable: ") + e.what());
		return TACDEV_INIT_FAILED;
	}
	catch (...)
	{
		gDevTACCore.setLastError("GetScriptVariable: Unknown exception");
		return TACDEV_INIT_FAILED;
	}
}

TAC_ERROR UpdateScriptVariableValue(TAC_HANDLE tacHandle, const char *scriptVariable, const char* scriptVariableValue)
{
	TAC_RESULT result{NO_TAC_ERROR};
	AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);

	try
	{
		if (alpacaDevice != nullptr)
		{
			alpacaDevice->updateScriptVariableValue(scriptVariable, scriptVariableValue);
		}
		else
		{
			result = TACDEV_BAD_TAC_HANDLE;
			gDevTACCore.setLastError(kTACDevHandleNotOpen);
		}
	}
	catch (TACException& e)
	{
		result = e.errorCode();
		gDevTACCore.setLastError(e.getMessage());
	}

	return result;
}

TAC_ERROR IsCommandQueueClear(TAC_HANDLE tacHandle, bool* status)
{
	TAC_RESULT result{NO_TAC_ERROR};

	AlpacaDevice alpacaDevice = gDevTACCore.getAlpacaDevice(tacHandle);
	if (alpacaDevice != nullptr)
	{
		try
		{
            // if false, queue is clear
            *status = !alpacaDevice->isCommandQueueClear();

			// We're limited by the PIC32CX firmware.
			// The firmware does not actually send us an acknowledgement for the command execution.
			// It receives the commands and processes them later. Hardcoded delay can help some automation use-cases.
			if (alpacaDevice->debugBoardType() == ePIC32CXAuto)
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
		}
		catch (TACException& e)
		{
			result = e.errorCode();
			gDevTACCore.setLastError(e.getMessage());
		}
	}
	else
	{
		result = TACDEV_BAD_TAC_HANDLE;
		gDevTACCore.setLastError(kTACDevHandleNotOpen);
	}

	return result;
}
