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
void DateGroupStore::InitDefault()
{
	date_groups.push_back(DateGroup(L"Default"));

	UpdateNumbers();

	signal_date_groups(date_groups);
}

void DateGroupStore::SetDateGroups(const std::vector<DateGroup>& date_groups)
{
	this->date_groups = date_groups;

	UpdateNumbers();

	signal_date_groups(this->date_groups);
}

std::vector<std::wstring> DateGroupStore::GetDateGroupsNames()
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

int DateGroupStore::GetNumber(const std::wstring& name)
{
	auto find_lambda = [&](const DateGroup& compare) { return compare.name == name; };
	auto found = std::find_if(date_groups.cbegin(), date_groups.cend(), find_lambda);
	if (found != date_groups.end())
	{
		return found->number;
	}
	else
	{
		throw std::exception("wstring not found");
	}
}

std::wstring DateGroupStore::GetName(int number)
{
	auto find_lambda = [&](const DateGroup& compare) { return compare.number == number; };
	auto found = std::find_if(date_groups.cbegin(), date_groups.cend(), find_lambda);
	if (found != date_groups.end())
	{
		return found->name;
	}
	else
	{
		throw std::exception("wstring not found");
	}
}

/*int DateGroupStore::CheckAndAdjustGroup(std::vector<DateIntervalBundle>* date_interval_bundles)
{
	for (auto& bundle : *date_interval_bundles)
	{
		if ((bundle.group < date_groups.size()) == false)
		{
			bundle.group = 0;
		}
	}
}*/

int DateGroupStore::GetMaxGroup()
{
	return date_groups.size() - 1;
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


