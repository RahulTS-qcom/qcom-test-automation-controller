#ifndef TACLITEPROTOCOL_H
#define TACLITEPROTOCOL_H
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

#include "QCommonConsoleGlobal.h"

class TACDriveThread;

#include "ProtocolInterface.h"
#include "ReceiveInterface.h"
#include "SendInterface.h"

#include <cstdint>
#include <string>

class QCOMMONCONSOLE_EXPORT TACLiteProtocol :
    public ProtocolInterface,
    public ReceiveInterface
{
public:
    TACLiteProtocol();
    virtual ~TACLiteProtocol();

    void setTACDriveTrain(TACDriveThread* tacDriveTrain);

    uint32_t sendCommand(const std::string& command, const Arguments& arguments, bool console = false,
        ReceiveInterface* receiveInterface = nullptr, bool shouldStore = true);
    void endTransaction(ReceiveInterface* receiveInterface = nullptr);
    void sendHelpCommand();

    virtual void receive(FramePackage& framePackage);
    virtual void idle();

protected:
    virtual void frameComplete(const std::string& completedFrame);
    virtual void badFrame(const std::string& completedFrame);

private:
    TACDriveThread*  _tacDriveTrain{nullptr};
    uint64_t         _tickCount{0};
    std::string      _currentCommand;

    void triggerElapsed();
    uint32_t queueCommand(const std::string& command);
};

#endif // TACLITEPROTOCOL_H
