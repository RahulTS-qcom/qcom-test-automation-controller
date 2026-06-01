#ifndef TACEXCEPTION_H
#define TACEXCEPTION_H

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

#include "QCommonConsoleGlobal.h"

#include <cstdint>
#include <stdexcept>
#include <string>

const uint32_t TAC_BUFFER_TOO_SMALL{1};
const uint32_t TAC_COMMAND_NOT_FOUND{3};
const uint32_t TAC_BAD_INDEX{4};
const uint32_t TAC_DEVICE_INACTIVE{5};
const uint32_t TAC_SCRIPT_VARIABLE_NOT_FOUND{6};

class QCOMMONCONSOLE_EXPORT TACException : public std::exception
{
public:
    TACException(uint32_t errorCode, const std::string& message)
        : _errorCode(errorCode), _message(message) {}
    ~TACException() override = default;

    const char* what() const noexcept override { return _message.c_str(); }
    uint32_t    errorCode() const { return _errorCode; }
    std::string getMessage() const { return _message; }

private:
    std::string  _message;
    uint32_t     _errorCode{0};
};

#endif // TACEXCEPTION_H
