/*
Decade
Copyright (c) 2019-2020 Marco Peyer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/gpl-3.0.txt>.
*/

#pragma once

#include <sigslot/signal.hpp>

#include <pugixml.hpp>

#include <string>
#include <vector>
#include <algorithm>


class DateGroup
{
public:

	DateGroup() :
		name(L"no name"),
		number(0),
		exclude(false)
	{}

	DateGroup(std::wstring name) :
		name(name),
		number(0),
		exclude(false)
	{}

	int number;
	std::wstring name;
	bool exclude;

private:
};

class DateGroupStore
{
public:

	void SetDateGroups(const std::vector<DateGroup>& argument_date_groups);

	std::vector<DateGroup> GetDateGroups() const;

	void InitDefault();

	int GetNumber(const std::wstring& name) const;
	std::wstring GetName(int number) const;
	std::vector<std::wstring> GetDateGroupsNames() const;
	int GetGroupMax() const;

	sigslot::signal<const std::vector<DateGroup>&> signal_date_groups;

	void LoadXML(const pugi::xml_node& doc);
	void SaveXML(pugi::xml_node* doc) const;

private:

	void UpdateNumbers();
	std::vector<DateGroup> date_groups;
};
