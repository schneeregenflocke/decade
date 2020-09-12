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

#include "../calendar_config.h"

#ifdef WX_PRECOMP
#include <wx/wxprec.h>
#else 
#include <wx/wx.h>
#endif

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/checkbox.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/manager.h>

#include <array>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>

#include <sigslot/signal.hpp>

#include <pugixml.hpp>


class PropertyGridPanel : public wxPropertyGrid
{
public:

	PropertyGridPanel(wxWindow* parent) :
		wxPropertyGrid(parent, -1, wxDefaultPosition, wxDefaultSize, wxPG_SPLITTER_AUTO_CENTER),
		box_sizer(new wxBoxSizer(wxHORIZONTAL))	
	{
		auto sizer_flags = wxSizerFlags().Proportion(1).Expand();
		parent->GetSizer()->Add(box_sizer, sizer_flags);
		box_sizer->Add(this, sizer_flags);

		SetVerticalSpacing(2);

		Append(new wxPropertyCategory("Subrow Proportions", wxPG_LABEL));
		Append(new wxIntProperty("Number Subrows", wxPG_LABEL, 0));
		DisableProperty("Number Subrows");
		
		Append(new wxPropertyCategory("Calendar Range", wxPG_LABEL));
		Append(new wxBoolProperty("Auto", wxPG_LABEL, false));

		Append(new wxIntProperty("Lower Limit", wxPG_LABEL, 0));
		Append(new wxIntProperty("Upper Limit", wxPG_LABEL, 0));
		DisableProperty("Lower Limit");
		DisableProperty("Upper Limit");
		
		RefreshPropertyGrid();
	}

	void RefreshPropertyGrid()
	{
		if (GetProperty("Auto")->GetValue() == true)
		{
			DisableProperty("Lower Limit");
			DisableProperty("Upper Limit");
		}
		if (GetProperty("Auto")->GetValue() == false)
		{
			EnableProperty("Lower Limit");
			EnableProperty("Upper Limit");
		}

		auto number_subrows = GetProperty("Number Subrows")->GetValue().GetLong();

		if (number_subrows > subrow_property_array.size())
		{
			for (size_t index = subrow_property_array.size(); index < number_subrows; ++index)
			{
				subrow_property_array.push_back(new wxFloatProperty(std::string("Subrow ") + std::to_string(index + 1), wxPG_LABEL, 10.0));
				Insert("Calendar Range", subrow_property_array[index]);
			}
		}

		if (number_subrows < subrow_property_array.size())
		{
			for (size_t index = number_subrows; index < subrow_property_array.size();)
			{
				DeleteProperty(subrow_property_array[index]);
				subrow_property_array.pop_back();
			}
		}
	}

	wxBoxSizer* box_sizer;
	std::vector<wxFloatProperty*> subrow_property_array;
};


class CalendarSetupPanel : public wxPanel
{
public:

	CalendarSetupPanel(wxWindow* parent) :
		wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxPanelNameStr),
		property_grid(nullptr)
	{
		wxBoxSizer* vertical_sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(vertical_sizer);

		property_grid = new PropertyGridPanel(this);

		Bind(wxEVT_PG_CHANGED, &CalendarSetupPanel::OnPropertyGridChanging, this);

		ActualizePropertyGridValues();
	}

	void OnPropertyGridChanging(wxPropertyGridEvent& event)
	{
		property_grid->RefreshPropertyGrid();

		//auto current_property = event.GetProperty();

		long number_subrows = property_grid->GetProperty("Number Subrows")->GetValue();
		calendar_config.subrow_proportions.resize(number_subrows);

		for (size_t index = 0; index < number_subrows; ++index)
		{
			const std::string current_property_id = std::string("Subrow ") + std::to_string(index + 1);
			calendar_config.subrow_proportions[index] = static_cast<float>(property_grid->GetPropertyValue(current_property_id.c_str()).GetDouble());
		}

		calendar_config.auto_calendar_range = property_grid->GetProperty("Auto")->GetValue();

		long lower_limit = property_grid->GetProperty("Lower Limit")->GetValue();
		long upper_limit = property_grid->GetProperty("Upper Limit")->GetValue();
		
		calendar_config.SetCalendarRange(lower_limit, upper_limit);

		signal_calendar_config(calendar_config);
	}

	void ActualizePropertyGridValues()
	{
		property_grid->SetPropertyValue("Number Subrows", static_cast<int>(calendar_config.subrow_proportions.size()));

		property_grid->RefreshPropertyGrid();

		for (size_t index = 0; index < calendar_config.subrow_proportions.size(); ++index)
		{
			const std::string current_property_id = std::string("Subrow ") + std::to_string(index + 1);
			property_grid->SetPropertyValue(current_property_id.c_str(), calendar_config.subrow_proportions[index]);
		}
		
		property_grid->SetPropertyValue("Auto", calendar_config.auto_calendar_range);
		property_grid->SetPropertyValue("Lower Limit", calendar_config.GetCalendarRange().first);
		property_grid->SetPropertyValue("Upper Limit", calendar_config.GetCalendarRange().second);
	}
	
	void SendCalendarConfig()
	{
		signal_calendar_config(calendar_config);
	}

	void LoadXML(const pugi::xml_node& node);
	void SaveXML(pugi::xml_node* node);
	
	sigslot::signal<const CalendarConfig&> signal_calendar_config;

private:

	PropertyGridPanel* property_grid;
	CalendarConfig calendar_config;
};
