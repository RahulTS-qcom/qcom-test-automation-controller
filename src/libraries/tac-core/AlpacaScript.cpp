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

#include "AlpacaScript.h"

#include "AppCore.h"
#include "ConsoleApplicationEnhancements.h"
#include "ScriptVariable.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

AlpacaScript::AlpacaScript()
{
}

std::string AlpacaScript::defaultScript(DebugBoardType /*boardType*/)
{
    std::string scriptPath;

#ifdef DEBUG
#ifdef _WIN32
    scriptPath = "C:\\github\\open-source\\qcom-test-automation-controller\\configurations\\DefaultScript.txt";
#else
    scriptPath = "/local/mnt/workspace/github/qcom-test-automation-controller/configurations/DefaultScript.txt";
#endif
#else
    scriptPath = tacConfigRoot() + "DefaultScript.txt";
#endif

    std::ifstream file(scriptPath);
    if (!file.is_open())
        return {};

    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

// Split a string by delimiter, skipping empty parts
static std::vector<std::string> splitWords(const std::string& line, char delim = ' ')
{
    std::vector<std::string> result;
    std::istringstream ss(line);
    std::string word;
    while (std::getline(ss, word, delim))
        if (!word.empty())
            result.push_back(word);
    return result;
}

static std::string toLower(const std::string& s)
{
    std::string r;
    for (auto c : s) r += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

static std::string trimStr(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    return s.substr(a, b - a + 1);
}

bool AlpacaScript::parseScript(const std::string& alpacaScript, const ScriptVariables& scriptVariables, const TACCommands& tacCommands)
{
    bool result{true};
    std::istringstream stream(alpacaScript);
    std::string line;
    ScriptCommand scriptCommand;

    while (std::getline(stream, line))
    {
        line = trimStr(line);
        auto words = splitWords(line);
        if (words.size() >= 2)
        {
            if (toLower(words[0]) == "def")
            {
                scriptCommand = std::make_shared<_ScriptCommand>();
                scriptCommand->_command = createBaseCommandString(words[1]);
                _scriptCommands[scriptCommand->_command] = scriptCommand;
            }
            else if (TACCommand::contains(words[0], tacCommands))
            {
                TACCommand tacCommand = TACCommand::find(words[0], tacCommands);
                auto commandEntry = std::make_shared<_CommandEntry>();
                commandEntry->_commandAction = _CommandEntry::eSetPin;
                commandEntry->_pinID = tacCommand._pin;
                commandEntry->_action = words[0];
                commandEntry->_arguement = (words[1] == "1");
                scriptCommand->_subCommands.push_back(commandEntry);
            }
            else if (toLower(words[0]) == "logcomment")
            {
                auto commandEntry = std::make_shared<_CommandEntry>();
                commandEntry->_commandAction = _CommandEntry::eLog;
                commandEntry->_action = words[0];
                // Strip "logcomment " prefix
                std::string msg = line;
                std::string prefix = "logcomment ";
                size_t pos = toLower(msg).find(prefix);
                if (pos != std::string::npos) msg = msg.substr(pos + prefix.size());
                commandEntry->_arguement = msg;
                scriptCommand->_subCommands.push_back(commandEntry);
            }
            else if (words[0] == "delay")
            {
                try
                {
                    uint16_t delayValue = static_cast<uint16_t>(std::stoul(words[1]));
                    if (delayValue > 0 && delayValue < 15000)
                    {
                        auto commandEntry = std::make_shared<_CommandEntry>();
                        commandEntry->_commandAction = _CommandEntry::eDelay;
                        commandEntry->_action = words[0];
                        commandEntry->_arguement = delayValue;
                        scriptCommand->_subCommands.push_back(commandEntry);
                    }
                    else
                    {
                        AppCore::writeToApplicationLogLine("Delay argument '" + words[1] + "' is invalid");
                        result = false;
                    }
                }
                catch (...)
                {
                    if (isScriptVariable(scriptVariables, words[1]))
                    {
                        auto commandEntry = std::make_shared<_CommandEntry>();
                        commandEntry->_commandAction = _CommandEntry::eDelay;
                        commandEntry->_action = words[0];
                        commandEntry->_arguement = words[1];
                        scriptCommand->_subCommands.push_back(commandEntry);
                    }
                    else
                    {
                        AppCore::writeToApplicationLogLine("Delay argument '" + words[1] + "' is invalid");
                        result = false;
                    }
                }
            }
            else
            {
                if (words[0].substr(0, 2) != "//")
                {
                    AppCore::writeToApplicationLogLine("Unhandled Command: " + words[0]);
                    result = false;
                }
            }
        }
        else if (words.size() == 1 && scriptCommand)
        {
            auto commandEntry = std::make_shared<_CommandEntry>();
            commandEntry->_commandAction = _CommandEntry::eBaseCommand;
            commandEntry->_action = words[0];
            scriptCommand->_subCommands.push_back(commandEntry);
        }
    }

    return result;
}

CommandEntries AlpacaScript::replaceTokens(const ScriptVariables& scriptVariables, const CommandEntries& commandEntries)
{
    CommandEntries result;

    for (const auto& cmdEntry : commandEntries)
    {
        auto copy = std::make_shared<_CommandEntry>();
        copy->_commandAction = cmdEntry->_commandAction;
        copy->_action        = cmdEntry->_action;
        copy->_arguement     = cmdEntry->_arguement;
        copy->_pinID         = cmdEntry->_pinID;
        result.push_back(copy);
    }

    for (auto& cmdEntry : result)
    {
        if (cmdEntry->_commandAction == _CommandEntry::eDelay)
        {
            if (std::holds_alternative<std::string>(cmdEntry->_arguement))
            {
                std::string varStr = std::get<std::string>(cmdEntry->_arguement);
                if (isScriptVariable(scriptVariables, varStr))
                {
                    std::string key = varStr.substr(1); // strip '$'
                    auto it = scriptVariables.find(key);
                    if (it != scriptVariables.end())
                    {
                        // Extract int from ScriptVariableValue and store as uint16_t delay
                        if (std::holds_alternative<int>(it->second._defaultValue))
                            cmdEntry->_arguement = static_cast<uint16_t>(std::get<int>(it->second._defaultValue));
                    }
                }
            }
        }
        else if (cmdEntry->_commandAction == _CommandEntry::eLog)
        {
            if (std::holds_alternative<std::string>(cmdEntry->_arguement))
            {
                std::string commentStr = std::get<std::string>(cmdEntry->_arguement);
                auto tokens = splitWords(commentStr);
                std::string formatted;
                for (auto& token : tokens)
                {
                    if (isScriptVariable(scriptVariables, token))
                    {
                        std::string key = token.substr(1);
                        auto it = scriptVariables.find(key);
                        if (it != scriptVariables.end())
                        {
                            if (std::holds_alternative<int>(it->second._defaultValue))
                                formatted += std::to_string(std::get<int>(it->second._defaultValue)) + " ";
                        }
                    }
                    else
                        formatted += token + " ";
                }
                cmdEntry->_arguement = formatted;
            }
        }
    }

    return result;
}

std::vector<std::string> AlpacaScript::validateScript(const std::string& alpacaScript, const TACCommands& tacCommands)
{
    std::vector<std::string> result;
    std::istringstream stream(alpacaScript);
    std::string line;
    std::vector<std::string> scriptCmds;
    int lineNumber{0};

    // First pass: collect def names
    while (std::getline(stream, line))
    {
        auto words = splitWords(trimStr(line));
        if (words.size() >= 2 && toLower(words[0]) == "def")
            scriptCmds.push_back(createBaseCommandString(words[1]));
    }

    // Second pass: validate
    stream.clear(); stream.seekg(0);
    while (std::getline(stream, line))
    {
        lineNumber++;
        auto words = splitWords(trimStr(line));
        if (words.size() == 2)
        {
            if (words[0] == "def") continue;
            if (!TACCommand::contains(words[0], tacCommands))
                result.push_back("line number: " + std::to_string(lineNumber) + " Unrecognized command " + words[0]);
        }
        else if (!words.empty() && words[0].substr(0, 2) != "//")
        {
            if (std::find(scriptCmds.begin(), scriptCmds.end(), words[0]) == scriptCmds.end())
                result.push_back("Line number: " + std::to_string(lineNumber) + " Unhandled Command: " + words[0]);
        }
    }

    return result;
}

bool AlpacaScript::hasCommand(const std::string& command)
{
    return _scriptCommands.find(command) != _scriptCommands.end();
}

std::vector<std::string> AlpacaScript::availableCommands()
{
    std::vector<std::string> result;
    for (const auto& kv : _scriptCommands)
        result.push_back(kv.first);
    std::stable_sort(result.begin(), result.end());
    return result;
}

ScriptCommand AlpacaScript::getCommand(const std::string& command)
{
    auto it = _scriptCommands.find(command);
    if (it != _scriptCommands.end())
        return it->second;
    return {};
}

CommandEntries AlpacaScript::getCommandEntries(const std::string& command)
{
    CommandEntries result;
    auto it = _scriptCommands.find(command);
    if (it != _scriptCommands.end())
        processCommandEntries(it->second, result, 1);
    return result;
}

bool AlpacaScript::isScriptVariable(const ScriptVariables& scriptVariables, const std::string& scriptVariable)
{
    if (scriptVariable.size() < 2 || scriptVariable[0] != '$')
        return false;

    std::string token = scriptVariable.substr(1);
    if (scriptVariables.count(token))
        return true;

    AppCore::writeToApplicationLogLine("Script variable '" + scriptVariable + "' is not defined.");
    return false;
}

std::string AlpacaScript::createBaseCommandString(const std::string& baseCommandString)
{
    std::string result;
    for (auto c : baseCommandString)
        if (c != '(' && c != ')')
            result += c;
    return trimStr(result);
}

bool AlpacaScript::processCommandEntries(ScriptCommand scriptCommand, CommandEntries& commandEntries, int level)
{
    bool result{true};
    if (!scriptCommand) return false;

    for (const auto& commandEntry : scriptCommand->_subCommands)
    {
        switch (commandEntry->_commandAction)
        {
        case _CommandEntry::eNotSet:
            break;
        case _CommandEntry::eSetPin:
        case _CommandEntry::eLog:
        case _CommandEntry::eDelay:
            commandEntries.push_back(commandEntry);
            break;
        case _CommandEntry::eBaseCommand:
            if (level < 7)
            {
                ScriptCommand child = getCommand(commandEntry->_action);
                processCommandEntries(child, commandEntries, level + 1);
            }
            else
            {
                AppCore::writeToApplicationLogLine("Recursive Level == 0: " + commandEntry->_action);
                result = false;
            }
            break;
        }
    }
    return result;
}

void AlpacaScript::setVariableValue(const std::string& variableName, int variableValue)
{
    // Update variable default value in all script commands
    (void)variableName; (void)variableValue;
}

void AlpacaScript::setVariableValue(const std::string& variableName, float variableValue)
{
    (void)variableName; (void)variableValue;
}
