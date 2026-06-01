#ifndef APPCORE_H
#define APPCORE_H
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
#include "PreferencesBase.h"
#include "ThreadedLog.h"

#include <cstdint>
#include <string>

class QCOMMONCONSOLE_EXPORT AppCore
{
friend class AppCoreDestructor;

public:
    ~AppCore();

    static AppCore* getAppCore();

    std::string appName()    { return _appName; }
    std::string appVersion() { return _appVersion; }

    uint32_t daysSinceInstall();

    PreferencesBase* getPreferences() { return _preferences; }
    void setPreferences(PreferencesBase* preferences);

    void setAppLogging(bool loggingState);
    bool appLoggingActive();
    std::string currentAppLog() { return _appThreadedLog->currentLogPath(); }

    void setRunLogging(bool loggingState);
    bool runLoggingActive();
    std::string currentRunLog() { return _runThreadedLog->currentLogPath(); }

    std::string loggingPath();

    bool checkLicense(const std::string& productID, const std::string& featureID);
    bool isLicenseManagerValid();

    void postStartEvent();
    void postAutomationEvent();
    void postMetric(const std::string& metricID, double metric);

    void writeToAppLog(const std::string& writeMe);

    static void writeToApplicationLog(const std::string& writeMe);
    static void writeToApplicationLogLine(const std::string& writeMe);

    void writeToRunLog(const std::string& writeMe);
    static void writeToRuntimeLog(const std::string& writeMe);

private:
    AppCore();

    static AppCore*   _appCore;

    bool              _internalBuild{false};
    std::string       _appName;
    std::string       _appVersion;

    PreferencesBase*  _preferences{nullptr};
    ThreadedLog       _appThreadedLog;
    ThreadedLog       _runThreadedLog;
};

#endif // APPCORE_H
