#ifndef FTDICHIPSET_H
#define FTDICHIPSET_H
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

#include "CharBit.h"
#include "FTDIPinSet.h"
#include "PlatformID.h"
#include "StringUtilities.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

class _FTDIChipset;

typedef std::shared_ptr<_FTDIChipset>          FTDIChipset;
typedef std::vector<FTDIChipset>               FTDIChipsetList;
typedef std::map<HashType, FTDIChipset>        FTDIChipsetMap;

#include "QCommonConsoleGlobal.h"

class QCOMMONCONSOLE_EXPORT _FTDIChipset
{
public:
    _FTDIChipset(const _FTDIChipset&) = delete;
    _FTDIChipset();

    static std::string ftidStatusToString(unsigned long status);

    static uint32_t   getDeviceCount();
    static FTDIChipset getDevice(uint32_t deviceIndex);
    static FTDIChipset getDevice(const std::string& portName);
    static std::string normalizeSerialNumber(const std::string& segmentSerialNumber);

    bool open(FTDIPinSets pinsets);
    bool isOpen();
    void close();

    PlatformID  platformID();
    void        setPlatformID(PlatformID platformID);

    std::string versionString();
    std::string firmwareString();

    void reset()   { _active = false; _ftdiChipsetList.clear(); }
    bool active()  { return _active; }

    HashType    hash();
    std::string serialNumber();
    bool        newDevice();

    std::string portName()      { return _portName; }
    std::string usbDescriptor() { return _usbDescriptor; }
    std::string lastError()     { return _lastError; }

    std::string aSerialNumber();
    void setASerialNumber(const std::string& s);
    std::string bSerialNumber();
    void setBSerialNumber(const std::string& s);
    std::string cSerialNumber();
    void setCSerialNumber(const std::string& s);
    std::string dSerialNumber();
    void setDSerialNumber(const std::string& s);

    bool write(uint8_t pin, bool state);

private:
    static HashType hash(const std::string& serialNumber);

    void setSerialNumber(const std::string& serialNumber);
    static unsigned long long setCustomVIDPID();

    static bool hasDevice(HashType portHash);
    void setupHash(const std::string& segmentSerialNumber);
    void setupPortName();
    static PlatformID nameToPlatform(const std::string& deviceName);

    static void linuxTraversal();
    static void windowsTraversal();

    HashType    _hash{0};
    PlatformID  _platformID{ALPACA_LITE_ID};
    bool        _new{true};
    bool        _active{false};
    CharBit     _aPins;
    CharBit     _bPins;
    CharBit     _cPins;
    CharBit     _dPins;

    std::string _serialNumber;
    std::string _portName;
    std::string _usbDescriptor;

    std::string _aSerialNumber;
    void*       _aHandle{nullptr};
    std::string _bSerialNumber;
    void*       _bHandle{nullptr};
    std::string _cSerialNumber;
    void*       _cHandle{nullptr};
    std::string _dSerialNumber;
    void*       _dHandle{nullptr};

    std::string _lastError;

    static FTDIChipsetList _ftdiChipsetList;
};


#endif // FTDICHIPSET_H
