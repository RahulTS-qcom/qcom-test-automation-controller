#ifndef FRAMEPACKAGE_H
#define FRAMEPACKAGE_H
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

// Author: msimpson

class ReceiveInterface;
#include "QCommonConsoleGlobal.h"

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

// Argument type for frame packages — replaces QVariant
typedef std::variant<bool, uint32_t, std::string> FrameArgument;

class QCOMMONCONSOLE_EXPORT _FramePackage
{
public:
    _FramePackage() = default;
    ~_FramePackage() = default;

    _FramePackage(const _FramePackage&) = delete;
    _FramePackage& operator=(const _FramePackage&) = delete;

    FrameArgument getArgument(uint32_t index)
    {
        if (index < _arguments.size())
            return _arguments[index];
        return {};
    }

    std::string                  _lastError;
    std::string                  _request;
    uint32_t                     _requestHash{0};
    std::vector<FrameArgument>   _arguments;
    std::string                  _synonym;
    std::string                  _codedRequest;
    std::vector<std::string>     _responses;
    uint32_t                     _packetID{0};
    std::string                  _comment;

    bool                         _endTransaction{false};
    bool                         _valid{true};
    bool                         _console{false};
    bool                         _shouldStore{false};
    ReceiveInterface*            _recieveInterface{nullptr};
    uint64_t                     _tickcount{0};
    uint32_t                     _delayInMilliSeconds{0};
};

typedef std::shared_ptr<_FramePackage>  FramePackage;
typedef std::vector<FramePackage>       FramePackageList;


#endif // FRAMEPACKAGE_H
