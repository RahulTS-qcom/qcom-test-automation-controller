#ifndef PSOCDEVICE_H
#define PSOCDEVICE_H
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

/*
	Author: Michael Simpson (msimpson@qti.qualcomm.com)
*/

#include "QCommonConsoleGlobal.h"

#include "AlpacaDevice.h"

class _PSOCPlatformConfiguration;

class QCOMMONCONSOLE_EXPORT PSOCDevice : public _AlpacaDevice
{
public:
    PSOCDevice() = default;
    virtual ~PSOCDevice();

    static uint32_t updateAlpacaDevices();

    virtual bool open();
    virtual void buildMapping();

private:
    _PSOCPlatformConfiguration* _psocPlatformConfiguration{nullptr};
};

#endif // TACDEVICE_H
