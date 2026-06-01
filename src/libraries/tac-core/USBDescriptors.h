#ifndef USBDESCRIPTORS_H
#define USBDESCRIPTORS_H
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
	Author: Biswajit Roy <biswroy@qti.qualcomm.com>
*/

#include "QCommonConsoleGlobal.h"

#include "DebugBoardType.h"
#include "FTDIPinSet.h"
#include "PlatformID.h"

#include <cstdint>
#include <string>
#include <vector>

const std::string kUSBDescriptorFileName{"devicelist.json"};

struct QCOMMONCONSOLE_EXPORT USBDescriptor
{
    USBDescriptor()
    {
        for (int i = 0; i < kMaxPinSetCount; i++)
            _pinSets[i] = NoOptions;
    }

    PlatformID      _platformID{0};
    DebugBoardType  _debugBoardType{eUnknownDebugBoard};
    std::string     _name;
    std::string     _description;
    std::string     _usbDescriptor;
    uint32_t        _revision{0};
    std::string     _configurationFilePath;
    FTDIPinSets     _pinSets[kMaxPinSetCount];
};

typedef std::vector<USBDescriptor> DescriptorList;

class QCOMMONCONSOLE_EXPORT USBDescriptors
{
public:
    USBDescriptors();
    USBDescriptors(const USBDescriptors&) = delete;
    USBDescriptors& operator=(const USBDescriptors&) = delete;
    ~USBDescriptors() = default;

    std::vector<PlatformID> getPlatforms();

    DescriptorList getDescriptors();
    DescriptorList getDescriptorsByBoardType(DebugBoardType debugBoardType);

    USBDescriptor getPlatformDescriptor(PlatformID platformID);
    PlatformID getPlatformId(const std::string& usbDescriptor);

    void addUSBDescriptor(USBDescriptor usbDescriptor);

    bool load(const std::string& filePath);
    bool save(const std::string& filePath);

    std::string getLastError();

private:
    std::string     _lastError;
    std::string     _modifyDate;
    DescriptorList  _descriptorList;
};

#endif // USBDESCRIPTORS_H
