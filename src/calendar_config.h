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
#include <exception>
#include <stdexcept>


class CalendarSpan
{
public:
	CalendarSpan() :
		span(boost::gregorian::date(2000, 1, 1), boost::gregorian::date(2010, 1, 1)),
		valid_id(0)
	{}

	void SetSpan(const int first_year, const int last_year)
	{
		const boost::gregorian::date min_date = boost::gregorian::date(boost::gregorian::min_date_time);
		const boost::gregorian::date max_date = boost::gregorian::date(boost::gregorian::max_date_time);

		auto clamped_lower_year = std::clamp(first_year, static_cast<int>(min_date.year()), static_cast<int>(max_date.year()));
		auto clamped_upper_year = std::clamp(last_year + 1, static_cast<int>(min_date.year()), static_cast<int>(max_date.year()));

		span = boost::gregorian::date_period(boost::gregorian::date(clamped_lower_year, 1, 1), boost::gregorian::date(clamped_upper_year, 1, 1));

		valid_id = CheckAndAdjustDateInterval(&span);
	}

	bool IsValidSpan() const
	{
		bool is_valid = (CheckDateInterval(span.begin(), span.end()) == 1);
		return is_valid;
	}

	int GetSpanLengthYears() const
	{
		if (IsValidSpan() == false)
		{
			throw std::runtime_error("Not valid calendar span!");
		}
		return span.end().year() - span.begin().year();
	}

	std::array<int, 2> GetSpanLimitsYears() const
	{
		return std::array<int, 2>{span.begin().year(), span.last().year()};
	}

	std::array<boost::gregorian::date, 2> GetSpanLimitsDate() const
	{
		return std::array<boost::gregorian::date, 2>{span.begin(), span.last()};
	}

	int GetSpanLengthDays() const
	{
		return span.length().days();
	}

	int GetYear(const int index) const
	{
		int year = span.begin().year() + index;

		if (IsInSpan(year) == false)
		{
			throw std::logic_error("Year not in span!");
		}

		return year;
	}

	bool IsInSpan(const int year) const
	{
		bool is_in_span = false;

		if (year >= span.begin().year() && year <= span.last().year())
		{
			is_in_span = true;
		}
		return is_in_span;
	}

private:
	boost::gregorian::date_period span;
	int valid_id;
};


class CalendarConfig : public CalendarSpan
{
public:
	CalendarConfig() :
		auto_calendar_span(true),
		spacing_proportions({ 25, 100, 50, 100, 50, 100, 25 })	
	{}

	bool auto_calendar_span;
	std::vector<float> spacing_proportions;
	
private:
	
};