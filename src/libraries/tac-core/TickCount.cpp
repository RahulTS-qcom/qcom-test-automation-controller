/*
	Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
	SPDX-License-Identifier: BSD-3-Clause
*/

#include "TickCount.h"

#include <chrono>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

uint64_t tickCount()
{
#ifdef _WIN32
    return static_cast<uint64_t>(::GetTickCount64());
#else
    // POSIX: monotonic clock in milliseconds
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000ULL
         + static_cast<uint64_t>(ts.tv_nsec) / 1000000ULL;
#endif
}

bool sleepUntil(std::function<bool()> test, uint64_t milliseconds)
{
    uint64_t deadline = tickCount() + milliseconds;
    while (tickCount() < deadline)
    {
        if (test()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}
