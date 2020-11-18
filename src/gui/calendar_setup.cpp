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
	auto calendar_setup_node = node.child("calendar_setup");

	calendar_config.spacing_proportions.clear();

	auto subrow_proportions_node = calendar_setup_node.child("spacing_proportions");
	for (auto& proportion : subrow_proportions_node.children("proportion"))
	{
		calendar_config.spacing_proportions.push_back(proportion.attribute("proportion").as_double());
	}

	calendar_config.auto_calendar_span = calendar_setup_node.child("auto_calendar_span").attribute("auto_calendar_span").as_bool();

	auto lower_limit = calendar_setup_node.child("span_lower_limit").attribute("span_lower_limit").as_int();
	auto upper_limit = calendar_setup_node.child("span_upper_limit").attribute("span_upper_limit").as_int();
	calendar_config.SetSpan(lower_limit, upper_limit);
	
	ActualizePropertyGridValues();

	signal_calendar_config(calendar_config);
}

void CalendarSetupPanel::SaveXML(pugi::xml_node* node) 
{
	auto calendar_setup_node = node->append_child("calendar_setup");

	auto subrow_proportions_node = calendar_setup_node.append_child("spacing_proportions");
	for (const auto& proportion : calendar_config.spacing_proportions)
	{
		auto proportion_node = subrow_proportions_node.append_child("proportion");
		proportion_node.append_attribute("proportion").set_value(proportion);
	}

	calendar_setup_node.append_child("auto_calendar_span").append_attribute("auto_calendar_span").set_value(calendar_config.auto_calendar_span);
	calendar_setup_node.append_child("span_lower_limit").append_attribute("span_lower_limit").set_value(calendar_config.GetSpanLimitsYears()[0]);
	calendar_setup_node.append_child("span_upper_limit").append_attribute("span_upper_limit").set_value(calendar_config.GetSpanLimitsYears()[1]);
}
