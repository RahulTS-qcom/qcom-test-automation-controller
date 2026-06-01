#ifndef TACLiteDriveThread_H
#define TACLiteDriveThread_H
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

#include "QCommonConsoleGlobal.h"

#include "TACLiteCommand.h"
#include "TACDriveThread.h"
#include "TACLiteProtocol.h"
#include "FTDIChipset.h"
#include "ReceiveInterface.h"

#include <cstdint>
#include <string>

class QCOMMONCONSOLE_EXPORT TACLiteDriveThread : public TACDriveThread
{
public:
    TACLiteDriveThread(uint32_t hash);
    ~TACLiteDriveThread();

    void setPinSets(FTDIPinSets pinsets) { _pinsets = pinsets; }

    virtual void run();

    virtual void sendCommand(const std::string& command, bool console = false,
        ReceiveInterface* receiveInterface = nullptr, bool shouldStore = true);

    virtual void externalPowerControl(bool state);

    virtual void setPinState(uint16_t pin, bool state);
    virtual void sendCommandSequence(CommandEntries& commandEntries);

    virtual int  getResetCount();
    virtual void clearResetCount();

    virtual void i2CReadRegister(uint32_t addr, uint32_t reg);
    virtual void i2CWriteRegister(uint32_t addr, uint32_t reg, uint32_t data);

    virtual void setName(const std::string& newName);

    virtual uint32_t send(const std::string& sendMe, const Arguments& arguments, bool console, ReceiveInterface* recieveInterface, bool store = true);
    virtual bool ready();

    virtual void receive(FramePackage& framePackage);

    std::string portDescription();

protected:
    bool openFTDIDevice();

private:
    static bool      _initialized;
    TACLiteProtocol  _tacProtocol;
    bool             _connected{false};
    bool             _readyRead{false};
    FTDIPinSets      _pinsets{static_cast<FTDIPinSets>(eC | eD)};
    FTDIChipset      _ftdiChipset;

    void handleIdle(FramePackage& framePackage);
    void handlePlatformID(FramePackage& framePackage);
    void handleSetName(FramePackage& framePackage);
    void handleSetPin(FramePackage& framePackage);
    void handleUUIDResponse(FramePackage& framePackage);
    void handleVersionResponse(FramePackage& framePackage);

    virtual void setupConnected();
    virtual void setupDiscovery();

    bool getElectrialPinValue(uint32_t kCommandHash, const Arguments& arguments);
};

#endif // TACDRIVETRAIN_H
