#ifndef SCRIPTVARIABLE_H
#define SCRIPTVARIABLE_H

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
	Author: Biswajit Roy (biswroy@qti.qualcomm.com)
*/

#include "QCommonConsoleGlobal.h"
#include "StringUtilities.h"

#include <map>
#include <string>
#include <variant>

const std::string kDefaultVariableName   {"<variable name>"};
const std::string kDefaultVariableLabel  {"<label for the variable>"};
const std::string kDefaultVariableTooltip{"<tooltip for the variable>"};

enum VariableType
{
    eUnknownVariableType = 0,
    eIntegerType,
    eBooleanType,
    eFloatType
};

// Replaces QVariant for script variable default values
typedef std::variant<int, bool, float> ScriptVariableValue;

struct QCOMMONCONSOLE_EXPORT ScriptVariable
{
public:
    ScriptVariable() = default;
    ~ScriptVariable() = default;
    ScriptVariable(const ScriptVariable&) = default;

    std::string          _name{kDefaultVariableName};
    std::string          _label{kDefaultVariableLabel};
    std::string          _tooltip{kDefaultVariableTooltip};
    VariableType         _type{eUnknownVariableType};
    ScriptVariableValue  _defaultValue{0};
    int                  _cellX{-1};
    int                  _cellY{-1};
};

typedef std::map<std::string, ScriptVariable> ScriptVariables;

std::string  QCOMMONCONSOLE_EXPORT variableTypeToString(VariableType variableType);
VariableType QCOMMONCONSOLE_EXPORT variableTypeFromString(const std::string& typeString);

#endif // SCRIPTVARIABLE_H
