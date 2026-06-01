#ifndef DEBUGLOG_H
#define DEBUGLOG_H
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996) // getenv deprecation
#endif

#include <cstdlib>
#include <fstream>
#include <mutex>
#include <string>

// Consolidated debug logger for tac-core.
// Writes to "tacdev_debug.log" when TACDEV_DEBUG env var is set.
// Thread-safe, single shared log file across all translation units.

class TACDebugLog
{
public:
    static TACDebugLog& instance()
    {
        static TACDebugLog s_instance;
        return s_instance;
    }

    bool enabled() const { return _enabled; }

    void write(const char* tag, const char* func, const std::string& msg)
    {
        if (!_enabled) return;
        std::lock_guard<std::mutex> lock(_mutex);
        if (_file.is_open())
        {
            _file << "[" << tag << "] " << func << ": " << msg << "\n";
            _file.flush();
        }
    }

    void write(const char* tag, const std::string& msg)
    {
        if (!_enabled) return;
        std::lock_guard<std::mutex> lock(_mutex);
        if (_file.is_open())
        {
            _file << "[" << tag << "] " << msg << "\n";
            _file.flush();
        }
    }

private:
    TACDebugLog()
        : _enabled(std::getenv("TACDEV_DEBUG") != nullptr)
    {
        if (_enabled)
            _file.open("tacdev_debug.log", std::ios::app);
    }

    ~TACDebugLog() = default;
    TACDebugLog(const TACDebugLog&) = delete;
    TACDebugLog& operator=(const TACDebugLog&) = delete;

    bool          _enabled;
    std::mutex    _mutex;
    std::ofstream _file;
};

// Convenience macros — define TAG before including this header, or use TACDEV_DBG_TAG directly
#define TACDEV_DBG_TAG(tag, msg) \
    do { if (TACDebugLog::instance().enabled()) TACDebugLog::instance().write(tag, __FUNCTION__, msg); } while(0)

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // DEBUGLOG_H
