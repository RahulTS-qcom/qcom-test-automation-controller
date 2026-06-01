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

#include "TACDevCore.h"
#include "AlpacaDefines.h"

// QCommon
#include "QCommonConsole.h"

bool DevTACCore::initialize(const std::string &appName, const std::string &appVersion)
{
	if (_initialized == false)
	{
		_preferences.setAppName(appName, appVersion);

		AlpacaSharedLibrary::initialize(appName, appVersion, &_preferences);

		InitializeQCommonConsole();

		_initialized = true;
	}

	return _initialized;
}

AlpacaDevice DevTACCore::getAlpacaDevice(TAC_HANDLE tacHandle)
{
	std::lock_guard<std::mutex> lock(_devicesMutex);
	AlpacaDevice alpacaDevice;

	if (licenseIsValid())
	{
		if (_openDevices.find(tacHandle) != _openDevices.end())
		{
			alpacaDevice = _openDevices[tacHandle];

			if (alpacaDevice != nullptr)
				setLastError(alpacaDevice->getLastError());
		}
		else
		{
			setLastError("Bad TAC Handle");
		}
	}
	else
	{
		setLastError("Invalid License");
	}

	return alpacaDevice;
}

TAC_HANDLE DevTACCore::OpenHandleByDescription(const char *portName)
{
	std::lock_guard<std::mutex> lock(_devicesMutex);
	TAC_HANDLE result{kBadHandle};

	if (licenseIsValid() == true)
	{
		for (auto& [tacHandle, openDevice]: _openDevices)
		{
			if (openDevice->portName() == portName)
				return tacHandle;

			if (openDevice->serialNumber().find(portName) != std::string::npos)
				return tacHandle;
		}

		AlpacaDevice alpacaDevice = _AlpacaDevice::findAlpacaDevice(std::string(portName));
		if (alpacaDevice != nullptr)
		{
			AppCore::writeToApplicationLog("[OpenHandleByDescription] Found device: port='" +
				alpacaDevice->portName() + "' desc='" + alpacaDevice->description() +
				"' boardType=" + alpacaDevice->debugBoardTypeString() +
				" platformID=" + std::to_string(static_cast<int>(alpacaDevice->platformID())) +
				" hasConfig=" + (alpacaDevice->platformConfiguration() ? "yes" : "NO") + "\n");

			alpacaDevice->onErrorEvent = [this](const std::string& msg) { onErrorEvent(msg); };

			if (alpacaDevice->open() == true)
			{
				result = alpacaDevice->hash();
				_openDevices[result] = alpacaDevice;
				AppCore::writeToApplicationLog("[OpenHandleByDescription] Opened OK, handle=" +
					std::to_string(result) + "\n");
			}
			else
			{
				std::string lastError = alpacaDevice->getLastError();
				AppCore::writeToApplicationLog("[OpenHandleByDescription] open() FAILED. lastError='" +
					lastError + "'\n");

				if (lastError.empty())
					setLastError(std::string(portName) + " can't be opened.");
				else
					setLastError(lastError);

				alpacaDevice->close();
			}
		}
		else
		{
			AppCore::writeToApplicationLog("[OpenHandleByDescription] Device '" +
				std::string(portName) + "' NOT FOUND in enumerated devices\n");
			setLastError(std::string(portName) + " can't be opened.");
		}
	}
	else
	{
		setLastError("Invalid License");
	}

	return result;
}

TAC_RESULT DevTACCore::CloseTACHandle(TAC_HANDLE tacHandle)
{
	std::lock_guard<std::mutex> lock(_devicesMutex);
	TAC_RESULT result{NO_TAC_ERROR};

	// Inline the lookup here to avoid double-locking _devicesMutex
	AlpacaDevice alpacaDevice;
	if (licenseIsValid())
	{
		auto it = _openDevices.find(tacHandle);
		if (it != _openDevices.end())
			alpacaDevice = it->second;
	}

	if (alpacaDevice != nullptr)
	{
		alpacaDevice->close();
		_openDevices.erase(tacHandle);
	}
	else
	{
		result = TACDEV_BAD_TAC_HANDLE;
	}

	return result;
}

void DevTACCore::onErrorEvent(const std::string &message)
{
	setLastError(message);
}
