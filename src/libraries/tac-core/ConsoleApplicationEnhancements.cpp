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

#include "ConsoleApplicationEnhancements.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>

#ifdef _WIN32
    #include <windows.h>
    #include <shellapi.h>
    #include <shlobj.h>
    #pragma comment(lib, "shell32")
#endif
#ifdef __linux__
    #include <unistd.h>
    #include <pwd.h>
#endif

namespace fs = std::filesystem;

static const std::string kAppName{"QTAC"};

static void ensureDir(const std::string& path)
{
    if (!fs::exists(path))
        fs::create_directories(path);
}

std::string applicationBinPath()
{
    std::string result;
#ifdef _WIN32
    result = "C:/Program Files (x86)/Qualcomm/" + kAppName + "/";
#else
    result = "/opt/qcom/" + kAppName + "/bin/";
#endif
    ensureDir(result);
    return result;
}

std::string applicationDataPath()
{
    // Allow override via TACDEV_CONFIG_PATH environment variable
    const char* envPath = std::getenv("TACDEV_CONFIG_PATH");
    if (envPath && envPath[0] != '\0')
    {
        std::string result = envPath;
        // Ensure trailing separator
        if (result.back() != '/' && result.back() != '\\')
            result += '/';
        ensureDir(result);
        return result;
    }

    std::string result = "../../../../configurations/";
    ensureDir(result);
    return result;
}

std::string documentsDataPath(const std::string& append)
{
    std::string base;
#ifdef _WIN32
    char homePath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, homePath)))
        base = std::string(homePath);
    else
        base = "C:/Users/Public/Documents";
#else
    const char* home = getenv("HOME");
    if (!home) home = "/root";
    base = std::string(home) + "/Documents";
#endif
    std::string result = base + "/" + kAppName;
    if (!append.empty())
        result += "/" + append;
    ensureDir(result);
    return result;
}

std::string defaultGlobalLoggingPath()
{
    std::string result = documentsDataPath("global");
    ensureDir(result);
    return result;
}

std::string defaultLoggingPath(const std::string& appName)
{
    std::string result = documentsDataPath(appName) + "/logs";
    ensureDir(result);
    return result;
}

std::string killOneDrive(const std::string& testPath, const std::string& revertPath)
{
    std::string lower = testPath;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    if (lower.find("onedrive -") != std::string::npos)
        return revertPath;
    return testPath;
}

std::string createFilenameTimeStamp()
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << "_" << std::put_time(&tm, "%Y_%d_%m_%H_%M_%S");
    return ss.str();
}

std::string getModuleFilePath(const std::string& moduleFileName)
{
    std::string result;
#ifdef _WIN32
    char path[MAX_PATH + 1] = {};
    std::string dllName = moduleFileName + ".dll";
    GetModuleFileNameA(GetModuleHandleA(dllName.c_str()), path, sizeof(path));
    result = path;
#endif
    return result;
}

std::string expandPath(const std::string& filePath)
{
    std::string result = filePath;
#ifdef __linux__
    if (!result.empty() && result[0] == '~')
    {
        const char* home = getenv("HOME");
        if (!home) home = "/root";
        result.replace(0, 1, home);
    }
#endif
    return result;
}

std::string tacConfigRoot(bool /*expandThePath*/)
{
    return applicationDataPath();
}

std::string epmConfigRoot()
{
    return applicationDataPath();
}

bool isUserPrivileged()
{
#ifdef _WIN32
    return IsUserAnAdmin() != 0;
#elif defined(__linux__)
    return geteuid() == 0;
#else
    return false;
#endif
}

bool executeBinaryAsAdministrator(const std::string& binary, const std::string& cmdArgs)
{
    bool result{false};
#ifdef _WIN32
    SHELLEXECUTEINFOA shExecInfo{};
    shExecInfo.cbSize       = sizeof(SHELLEXECUTEINFOA);
    shExecInfo.fMask        = SEE_MASK_NOCLOSEPROCESS;
    shExecInfo.lpVerb       = "runas";
    shExecInfo.lpFile       = binary.c_str();
    shExecInfo.lpParameters = cmdArgs.c_str();
    shExecInfo.nShow        = SW_SHOW;

    if (ShellExecuteExA(&shExecInfo))
    {
        WaitForSingleObject(shExecInfo.hProcess, INFINITE);
        DWORD exitCode{0};
        GetExitCodeProcess(shExecInfo.hProcess, &exitCode);
        result = (exitCode == 0);
        CloseHandle(shExecInfo.hProcess);
    }
#else
    // On Linux, use system() as a simple fallback
    std::string cmd = "sudo " + binary + " " + cmdArgs;
    result = (system(cmd.c_str()) == 0);
#endif
    return result;
}
