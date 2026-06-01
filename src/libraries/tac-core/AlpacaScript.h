#ifndef ALPACASCRIPT_H
#define ALPACASCRIPT_H
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

#include "QCommonConsoleGlobal.h"
#include "DebugBoardType.h"
#include "PinID.h"
#include "ScriptVariable.h"
#include "TACCommand.h"

#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

struct _CommandEntry
{
    enum CommandAction
    {
        eNotSet,
        eSetPin,
        eLog,
        eDelay,
        eBaseCommand
    };

    CommandAction  _commandAction{eNotSet};
    PinID          _pinID{static_cast<PinID>(-1)};
    std::string    _action;
    // Replaces QVariant: holds bool (pin state), uint16_t (delay ms), or string (variable/log)
    std::variant<bool, uint16_t, std::string> _arguement{false};
};

typedef std::shared_ptr<_CommandEntry>  CommandEntry;
typedef std::vector<CommandEntry>       CommandEntries;

struct _ScriptCommand
{
    bool isEmpty() const
    {
        return _command.empty() || _subCommands.empty();
    }

    void clear()
    {
        _command.clear();
        _subCommands.clear();
    }

    std::string    _command;
    CommandEntries _subCommands;
};

typedef std::shared_ptr<_ScriptCommand>          ScriptCommand;
typedef std::map<std::string, ScriptCommand>     ScriptCommands;

class QCOMMONCONSOLE_EXPORT AlpacaScript
{
public:
    AlpacaScript();

    uint32_t count() { return static_cast<uint32_t>(_scriptCommands.size()); }

    static std::string defaultScript(DebugBoardType boardType = eFTDI);

    bool parseScript(const std::string& alpacaScript, const ScriptVariables& scriptVariables, const TACCommands& tacCommands);
    CommandEntries replaceTokens(const ScriptVariables& scriptVariables, const CommandEntries& commandEntries);
    std::vector<std::string> validateScript(const std::string& alpacaScript, const TACCommands& tacCommands);

    bool hasCommand(const std::string& command);
    std::vector<std::string> availableCommands();

    ScriptCommand  getCommand(const std::string& command);
    CommandEntries getCommandEntries(const std::string& command);

    void setVariableValue(const std::string& variableName, int variableValue);
    void setVariableValue(const std::string& variableName, float variableValue);

private:
    bool isScriptVariable(const ScriptVariables& scriptVariables, const std::string& scriptVariable);
    std::string createBaseCommandString(const std::string& baseCommandString);
    bool processCommandEntries(ScriptCommand scriptCommand, CommandEntries& commandEntries, int level = 0);

    ScriptCommands _scriptCommands;
};

#endif // ALPACASCRIPT_H
