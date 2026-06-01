// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef TACPSOCCODER_H
#define TACPSOCCODER_H

#include "QCommonConsoleGlobal.h"
#include "FrameCoder.h"
#include <string>

const std::string kCommand{"CMD >> "};
const std::string kCommandNotRecognized{"CMD: Command not recognized."};

class QCOMMONCONSOLE_EXPORT TACPSOCCoder : public FrameCoder
{
public:
    TACPSOCCoder();
    virtual ~TACPSOCCoder();
    virtual void reset();
    virtual void decode(const std::string& decodeMe);
    virtual std::string encode(const std::string& encodeMe, const Arguments& arguments);
private:
    TACPSOCCoder(const TACPSOCCoder&) = delete;
    TACPSOCCoder& operator=(const TACPSOCCoder&) = delete;
    std::string _recieveBuffer;
};

#endif // TACPSOCCODER_H
