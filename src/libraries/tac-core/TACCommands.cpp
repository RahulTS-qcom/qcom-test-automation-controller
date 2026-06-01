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

#include "TACCommands.h"
#include "TACCommandHashes.h"
#include "StringUtilities.h"

#include <algorithm>
#include <cctype>
#include <map>

static std::map<HashType, oldCommandEntry>  gCommandMap;
static std::map<std::string, HashType>      gAliasMap;

static void InitializeCommandMap();

static std::string CommandToCompressedCommand(const std::string& compressMe)
{
	std::string result;
	for (auto c : compressMe)
	{
		if (c != ' ')
			result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	return result;
}

static oldCommandEntry makeCommandEntry(
	HashType hash,
	const std::string& longCommand,
	const std::string& alias,
	const std::string& helpText,
	bool setter = false,
	TACArgType argType = TACArgType::None)
{
	oldCommandEntry e;
	e._longCommand        = longCommand;
	e._compressedCommand  = CommandToCompressedCommand(longCommand);
	e._hash               = hash;
	e._alias              = alias;
	e._helpText           = helpText;
	e._setter             = setter;
	e._argType            = argType;
	gAliasMap[alias]      = hash;
	return e;
}

HashType AddCommandToEntries(const std::string& commandToAdd, const std::string& alias, const std::string& helpText)
{
	if (gCommandMap.empty())
		InitializeCommandMap();

	std::string compressed = CommandToCompressedCommand(commandToAdd);
	HashType result = CommandStringToHash(commandToAdd);
	if (result == 0)
	{
		HashType hash = arrayHash(compressed);
		gCommandMap[hash] = makeCommandEntry(hash, commandToAdd, alias, helpText);
	}
	return result;
}

HashType CommandStringToHash(const std::string& commandString)
{
	if (gCommandMap.empty())
		InitializeCommandMap();

	auto it = gAliasMap.find(commandString);
	if (it != gAliasMap.end())
		return it->second;

	std::string compressed = CommandToCompressedCommand(commandString);
	HashType hash = arrayHash(compressed);
	if (gCommandMap.count(hash))
		return hash;

	return 0;
}

oldCommandEntry CommandHashToCommandEntry(HashType commandHash)
{
	if (gCommandMap.empty())
		InitializeCommandMap();

	auto it = gCommandMap.find(commandHash);
	if (it != gCommandMap.end())
		return it->second;

	return {};
}

void AddHelpTextEntry(std::string& helpText, HashType commandHash)
{
	auto entry = CommandHashToCommandEntry(commandHash);
	if (!entry._helpText.empty())
		helpText += entry._helpText + "\n";
}

static void InitializeCommandMap()
{
	gCommandMap[kVersionCommandHash]         = makeCommandEntry(kVersionCommandHash,         kVersionCommand,              "version",      "Returns the QTAC board's version information");
	gCommandMap[kGetNameCommandHash]         = makeCommandEntry(kGetNameCommandHash,         kGetNameCommand,              "getname",      "Returns the name of the QTAC board");
	gCommandMap[kSetNameCommandHash]         = makeCommandEntry(kSetNameCommandHash,         kSetNameCommand,              "setname",      "Sets the name of the QTAC board ex. setname foobar", true, TACArgType::String);
	gCommandMap[kGetUUIDCommandHash]         = makeCommandEntry(kGetUUIDCommandHash,         kGetUUIDCommand,              "uuid",         "Returns the UUID of the QTAC board");
	gCommandMap[kGetPlatformIDCommandHash]   = makeCommandEntry(kGetPlatformIDCommandHash,   kGetPlatformIDCommand,        "platid",       "Returns the Platform ID of the QTAC board");
	gCommandMap[kGetResetCountCommandHash]   = makeCommandEntry(kGetResetCountCommandHash,   kGetResetCountCommand,        "resetcnt",     "Returns the Reset Count");
	gCommandMap[kClearResetCountCommandHash] = makeCommandEntry(kClearResetCountCommandHash, kClearResetCountCommand,      "clrresetcnt",  "Clears the Reset Count");
	gCommandMap[kI2CReadRegisterCommandHash] = makeCommandEntry(kI2CReadRegisterCommandHash, kI2CReadRegisterCommand,      "rdreg",        "Reads an I2C register");
	gCommandMap[kI2CReadRegisterValueCommandHash] = makeCommandEntry(kI2CReadRegisterValueCommandHash, kI2CReadRegisterValueCommand, "", "Internal, not visible");
	gCommandMap[kI2CWriteRegisterCommandHash]= makeCommandEntry(kI2CWriteRegisterCommandHash,kI2CWriteRegisterCommand,    "wrreg",        "Writes an I2C register");
	gCommandMap[kSetPinCommandHash]          = makeCommandEntry(kSetPinCommandHash,          kSetPinCommand,               "setpin",       "setpin nn on|off - Toggles a hardware pin nn, nn being the pin number");
	gCommandMap[kPIC32CXClearBufferHash]     = makeCommandEntry(kPIC32CXClearBufferHash,     kPIC32CXClearBufferCommand,   "\n",           "\n - Clear buffer stream");
	gCommandMap[kPIC32CXVersionCommandHash]  = makeCommandEntry(kPIC32CXVersionCommandHash,  kPIC32CXPlatformIDCommand,    "*IDN?",        "*IDN? - Requests debug board to identify itself");
	gCommandMap[kPIC32CXSetPinCommandHash]   = makeCommandEntry(kPIC32CXSetPinCommandHash,   kPIC32CXSetPinCommand,        "CONF:DIG:ON",  "CONF:DIG:ON on|off (@pin)- Toggles a hardware pin nn, nn being the pin number");
}
