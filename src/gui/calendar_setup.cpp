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

#include "calendar_setup.h"


void CalendarSetupPanel::LoadXML(const pugi::xml_node& node)
{
	auto calendar_setup_node = node.child(L"calendar_setup");

	calendar_config.subrow_proportions.clear();

	auto subrow_proportions_node = calendar_setup_node.child(L"spacing_proportions");
	for (auto& proportion : subrow_proportions_node.children(L"proportion"))
	{
		calendar_config.subrow_proportions.push_back(proportion.attribute(L"proportion").as_double());
	}

	calendar_config.auto_calendar_range = calendar_setup_node.child(L"auto_calendar_span").attribute(L"auto_calendar_span").as_bool();

	auto lower_limit = calendar_setup_node.child(L"span_lower_limit").attribute(L"span_lower_limit").as_int();
	auto upper_limit = calendar_setup_node.child(L"span_upper_limit").attribute(L"span_upper_limit").as_int();
	calendar_config.SetCalendarRange(lower_limit, upper_limit);
	
	ActualizePropertyGridValues();

	signal_calendar_config(calendar_config);
}

void CalendarSetupPanel::SaveXML(pugi::xml_node* node) 
{
	auto calendar_setup_node = node->append_child(L"calendar_setup");

	auto subrow_proportions_node = calendar_setup_node.append_child(L"spacing_proportions");
	for (const auto& proportion : calendar_config.subrow_proportions)
	{
		auto proportion_node = subrow_proportions_node.append_child(L"proportion");
		proportion_node.append_attribute(L"proportion").set_value(proportion);
	}

	calendar_setup_node.append_child(L"auto_calendar_span").append_attribute(L"auto_calendar_span").set_value(calendar_config.auto_calendar_range);
	calendar_setup_node.append_child(L"span_lower_limit").append_attribute(L"span_lower_limit").set_value(calendar_config.GetCalendarRange().first);
	calendar_setup_node.append_child(L"span_upper_limit").append_attribute(L"span_upper_limit").set_value(calendar_config.GetCalendarRange().second);
}
