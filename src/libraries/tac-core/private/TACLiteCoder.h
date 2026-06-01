// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef TACLITECODER_H
#define TACLITECODER_H

#include "QCommonConsoleGlobal.h"
#include "FrameCoder.h"
#include <string>

class QCOMMONCONSOLE_EXPORT TACLiteCoder : public FrameCoder
{
public:
    TACLiteCoder();
    virtual ~TACLiteCoder();
    virtual void reset();
    virtual void decode(const std::string& decodeMe);
    virtual std::string encode(const std::string& encodeMe, const Arguments& arguments);
private:
    TACLiteCoder(const TACLiteCoder&) = delete;
    TACLiteCoder& operator=(const TACLiteCoder&) = delete;
    std::string _recieveBuffer;
};

#endif // TACLITECODER_H
