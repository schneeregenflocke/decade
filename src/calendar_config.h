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

#include <vector>

struct CalendarConfig
{
	CalendarConfig()
	{
		sub_row_weights.resize(3);
		sub_row_weights[0] = 1.f;
		sub_row_weights[1] = 2.f;
		sub_row_weights[2] = 2.f;
		gap_factor = 0.15f;
		min_calendar_range = 2020;
		max_calendar_range = 2030;
		auto_calendar_range = true;
	}

	std::vector<float> sub_row_weights;
	float gap_factor;
	int min_calendar_range;
	int max_calendar_range;
	bool auto_calendar_range;
};