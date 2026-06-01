// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

// Author: msimpson, biswroy

#include "QCommonConsole.h"
#include "PlatformID.h"

void InitializeQCommonConsole()
{
    // Q_INIT_RESOURCE not needed for non-Qt build — platform data loaded from JSON files
    PlatformContainer::initialize();
}
