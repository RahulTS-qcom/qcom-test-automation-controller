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
	Michael Simpson (msimpson@qti.qualcomm.com)
*/

#include "Tabs.h"

HashType Tab::makeHash(const Tab& tab)
{
	return strHash(tab._name + fromBool(tab._userTab) + fromBool(tab._moveable) + fromBool(tab._configurable));
}

Tab Tab::find(HashType hash, Tabs& tabs)
{
	for (const auto& tab : tabs)
		if (tab._hash == hash)
			return tab;
	return {};
}

void Tab::addTab(Tab& tab, Tabs& /*tabs*/)
{
	tab._hash = Tab::makeHash(tab);
}

void Tab::removeTab(HashType hash, Tabs& tabs)
{
	for (const auto& tab : tabs)
		if (tab._hash == hash)
			break;
}

void Tab::updateTabName(HashType hash, std::string& name, Tabs& tabs)
{
	for (auto& tab : tabs)
		if (tab._hash == hash) { tab._name = name; break; }
}

void Tab::setTabVisible(HashType hash, bool visible, Tabs& tabs)
{
	for (auto& tab : tabs)
		if (tab._hash == hash) { tab._visible = visible; break; }
}

std::vector<std::string> Tab::toStringList(Tabs& tabs, bool configurableTabs)
{
	std::vector<std::string> result;

	if (configurableTabs)
		result.push_back("<select a group>");

	for (const auto& tab : tabs)
	{
		if (configurableTabs)
		{
			if (tab._configurable)
				result.push_back(tab._newText.empty() ? tab._name : tab._newText);
		}
		else
		{
			result.push_back(tab._newText.empty() ? tab._name : tab._newText);
		}
	}
	return result;
}


