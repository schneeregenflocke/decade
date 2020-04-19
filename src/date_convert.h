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

#define BOOST_DATE_TIME_NO_LIB
#include <boost\date_time\gregorian\gregorian.hpp>

using date = boost::gregorian::date;

#include <string>
#include <array>
#include <cctype>

struct date_format_descriptor
{
	std::array<size_t, 3> date_order;
	std::array<size_t, 3> delimiters;
	bool shortyear;
};

date_format_descriptor InitDateFormat();

std::string boost_date_to_string(const date& date_variable);
date string_to_boost_date(std::string date_string, const date_format_descriptor& format);