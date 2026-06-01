#ifndef CONSOLEAPPLICATIONENHANCEMENTS_H
#define CONSOLEAPPLICATIONENHANCEMENTS_H
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

// QCommon
#include "QCommonConsoleGlobal.h"

#include <string>

std::string QCOMMONCONSOLE_EXPORT applicationBinPath();
std::string QCOMMONCONSOLE_EXPORT applicationDataPath();
std::string QCOMMONCONSOLE_EXPORT documentsDataPath(const std::string& append);
std::string QCOMMONCONSOLE_EXPORT defaultGlobalLoggingPath();
std::string QCOMMONCONSOLE_EXPORT defaultLoggingPath(const std::string& appName);

std::string QCOMMONCONSOLE_EXPORT killOneDrive(const std::string& testPath, const std::string& revertPath);
std::string QCOMMONCONSOLE_EXPORT createFilenameTimeStamp();

std::string QCOMMONCONSOLE_EXPORT getModuleFilePath(const std::string& moduleFileName);
std::string QCOMMONCONSOLE_EXPORT expandPath(const std::string& filePath);

std::string QCOMMONCONSOLE_EXPORT tacConfigRoot(bool expandPath = true);
std::string QCOMMONCONSOLE_EXPORT epmConfigRoot();

bool QCOMMONCONSOLE_EXPORT isUserPrivileged();
bool QCOMMONCONSOLE_EXPORT executeBinaryAsAdministrator(const std::string& binary, const std::string& cmdArgs);

#endif

