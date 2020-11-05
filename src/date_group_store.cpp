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


#include "date_group_store.h"

// call after connecting
void DateGroupStore::SendDefaultValues()
{
	std::vector<DateGroup> temporary_date_groups;

	temporary_date_groups.push_back(DateGroup(L"Default"));

	ReceiveDateGroups(temporary_date_groups);
}

void DateGroupStore::ReceiveDateGroups(const std::vector<DateGroup>& date_groups)
{
	this->date_groups = date_groups;

	UpdateNumbers();

	signal_date_groups(date_groups);
}

std::vector<DateGroup> DateGroupStore::GetDateGroups() const
{
	return date_groups;
}

std::vector<std::wstring> DateGroupStore::GetDateGroupsNames() const
{
	std::vector<std::wstring> name_strings(date_groups.size());
	auto iterator = name_strings.begin();
	for (const auto& date_group : date_groups)
	{
		*iterator = date_group.name;
		++iterator;
	}
	return name_strings;
}

int DateGroupStore::GetNumber(const std::wstring& name) const
{
	auto find_lambda = [&](const DateGroup& compare) { return compare.name == name; };
	auto found = std::find_if(date_groups.cbegin(), date_groups.cend(), find_lambda);
	if (found != date_groups.end())
	{
		return found->number;
	}
	else
	{
		throw std::runtime_error("number not found");
	}
}

std::wstring DateGroupStore::GetName(int number) const
{
	auto find_lambda = [&](const DateGroup& compare) { return compare.number == number; };
	auto found = std::find_if(date_groups.cbegin(), date_groups.cend(), find_lambda);
	if (found != date_groups.end())
	{
		return found->name;
	}
	else
	{
		throw std::runtime_error("wstring not found");
	}
}

int DateGroupStore::GetGroupMax() const
{
	return date_groups.size() - 1;
}

bool DateGroupStore::GetExclude(int number) const
{
	return date_groups[number].exclude;
}

void DateGroupStore::LoadXML(const pugi::xml_node& doc)
{
	std::vector<DateGroup> temporary_date_groups;

	auto base_node = doc.child(L"date_group_store");

	for (auto& node : base_node.children(L"date_group"))
	{
		DateGroup temporary_date_group;

		temporary_date_group.name = node.attribute(L"name").value();
		temporary_date_group.exclude = node.attribute(L"exclude").as_bool();

		temporary_date_groups.push_back(temporary_date_group);
	}

	date_groups.clear();
	ReceiveDateGroups(temporary_date_groups);
}

void DateGroupStore::SaveXML(pugi::xml_node* doc) const
{
	auto base_node = doc->append_child(L"date_group_store");

	for (size_t index = 0; index < date_groups.size(); ++index)
	{
		auto node = base_node.append_child(L"date_group");

		node.append_attribute(L"name").set_value(date_groups[index].name.c_str());
		node.append_attribute(L"exclude").set_value(date_groups[index].exclude);
	}
}

void DateGroupStore::UpdateNumbers()
{
	int number = 0;
	for (auto& date_group : date_groups)
	{
		date_group.number = number;
		++number;
	}
}
