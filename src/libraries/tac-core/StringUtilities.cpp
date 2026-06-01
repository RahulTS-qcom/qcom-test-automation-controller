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
*/

#include "StringUtilities.h"

#include <algorithm>
#include <cctype>
#include <sstream>

std::string toCamelCase(const std::string& camelCaseMe, char splitChar)
{
    std::string result;
    std::istringstream ss(camelCaseMe);
    std::string part;
    while (std::getline(ss, part, splitChar))
    {
        if (part.empty()) continue;
        part[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(part[0])));
        result += part;
    }
    return result;
}

bool isAlphaNumeric(const std::string& testMe)
{
    for (const auto c : testMe)
    {
        if (std::isalnum(static_cast<unsigned char>(c)) == 0)
            return false;
    }
    return true;
}

std::string fromBool(bool value)
{
    return value ? "true" : "false";
}

HashType strHash(const std::string& hashMe)
{
    return arrayHash(hashMe);
}

HashType arrayHash(const std::string& hashMe)
{
    uint64_t result{0};
    const uint64_t p = 257;
    const uint64_t m = 1000000009ULL;
    uint64_t p_pow = 1;
    for (const auto c : hashMe)
    {
        result = (result + (static_cast<unsigned char>(c) - 'a' + 1) * p_pow) % m;
        p_pow = (p_pow * p) % m;
    }
    return result;
}
