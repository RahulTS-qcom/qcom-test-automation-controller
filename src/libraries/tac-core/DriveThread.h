#ifndef DRIVETHREAD_H
#define DRIVETHREAD_H
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

#include "ProtocolInterface.h"
#include "SendInterface.h"
#include "QCommonConsoleGlobal.h"

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <thread>

class QCOMMONCONSOLE_EXPORT DriveThread : public SendInterface
{
public:
    DriveThread();
    virtual ~DriveThread();

    bool weAreRunning()
    {
        std::lock_guard<std::mutex> lock(_runningMutex);
        return _running;
    }

    // Start the drive thread
    void start()
    {
        _thread = std::thread([this]{ run(); });
    }

    // Returns true if the thread is joinable (running or finished but not yet joined)
    bool isRunning() const { return _thread.joinable(); }

    void shutDown();

    // Returns the thread's ID (for self-join detection)
    std::thread::id threadId() const { return _thread.get_id(); }

    // Signal the thread to stop (does NOT join — call shutDown() to also join)
    bool stopRunning()
    {
        std::lock_guard<std::mutex> lock(_runningMutex);
        bool result = _running;
        _running = false;
        return result;
    }

    // Detach the thread so the destructor won't join.
    // Use ONLY when close() is called from within the drive thread itself
    // (self-join avoidance). The thread must already be in its exit path.
    void detachThread()
    {
        if (_thread.joinable())
            _thread.detach();
    }

    std::string name() const { return _driveTrainName; }
    int id() const { return _driveTrainID; }

    std::string lastErrorMessage()
    {
        std::lock_guard<std::mutex> lock(_runningMutex);
        std::string result = _lastErrorMessage;
        _lastErrorMessage.clear();
        return result;
    }

    void setProtocolInterface(ProtocolInterface* protocolInterface);

    virtual void run() = 0;

    // SendInterface
    virtual uint32_t send(const std::string& sendMe, const Arguments& arguments, bool console, ReceiveInterface* recieveInterface, bool store = true) = 0;
    virtual void addDelay(uint32_t delayInMilliSeconds, ReceiveInterface* recieveInterface);
    virtual void addLogComment(const std::string& comment);
    virtual void addEndTransaction(ReceiveInterface* receiveInterface);
    virtual bool ready() = 0;

protected:
    void startRunning()
    {
        std::lock_guard<std::mutex> lock(_runningMutex);
        _running = true;
    }

    static std::atomic<int> _driveTrainIDs;

    int                 _driveTrainID{0};
    ProtocolInterface*  _protocolInterface{nullptr};
    std::string         _driveTrainName{"<unnamed>"};
    std::string         _lastErrorMessage;
    std::thread         _thread;

private:
    std::mutex          _runningMutex;
    bool                _running{false};
};

typedef std::map<int, DriveThread*> DriveTrains;

#endif // DRIVETHREAD_H
