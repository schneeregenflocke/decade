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

#include "date_utils.h"

#include <vector>
#include <utility>
#include <algorithm>


class CalendarConfig
{
public:
	CalendarConfig() :
		subrow_proportions({25, 100, 50, 100, 50, 100, 25}),
		calendar_range(2010, 2020),
		auto_calendar_range(false)
	{
		min_year = boost::gregorian::date(boost::gregorian::min_date_time).year();
		max_year = boost::gregorian::date(boost::gregorian::max_date_time).year();
	}

	std::vector<float> subrow_proportions;
	bool auto_calendar_range;

	void SetCalendarRange(const int lower_limit, const int upper_limit)
	{
		calendar_range.first = std::clamp(lower_limit, min_year, max_year);
		calendar_range.second = std::clamp(upper_limit, min_year, max_year);
	}

	std::pair<int, int> GetCalendarRange() const
	{
		return calendar_range;
	}

private:
	std::pair<int, int> calendar_range;
	int min_year;
	int max_year;
};