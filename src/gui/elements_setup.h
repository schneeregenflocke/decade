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

#include "../shape_config.h"
#include "../date_group_store.h"


#include "wx_widgets_include.h"

#include <array>
#include <vector>
#include <algorithm>
#include <string>

#include <memory>
#include <functional>

#include <sigslot/signal.hpp>

#include <pugixml.hpp>


inline glm::vec4 to_glm_color(const wxColour& color)
{
	float ratio = 1.f / 255.f;

	float red = static_cast<float>(color.Red()) * ratio;
	float green = static_cast<float>(color.Green()) * ratio;
	float blue = static_cast<float>(color.Blue()) * ratio;
	float alpha = static_cast<float>(color.Alpha()) * ratio;

	return glm::vec4(red, green, blue, alpha);
}


inline wxColour to_wx_color(const glm::vec4& color)
{
	unsigned char red = static_cast<unsigned char>(color.r * 255.f);
	unsigned char green = static_cast<unsigned char>(color.g * 255.f);
	unsigned char blue = static_cast<unsigned char>(color.b * 255.f);
	unsigned char alpha = static_cast<unsigned char>(color.a * 255.f);

	return wxColour(red, green, blue, alpha);
}


class ElementsSetupsPanel : public wxPanel
{
public:
	
	ElementsSetupsPanel(wxWindow* parent) :
		wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxPanelNameStr),
		ID_OUTLINE_VISIBLE(NewControlId()),
		ID_FILLING_VISIBLE(NewControlId()),
		ID_LINE_COLOR_PICKER(NewControlId()),
		ID_FILL_COLOR_PICKER(NewControlId()),
		ID_LINE_ALPHA_SLIDER(NewControlId()),
		ID_FILL_ALPHA_SLIDER(NewControlId())
	{

		InitWidgets();
		InitSizers();

		////////////////////////////////////////

		element_configurations.emplace_back("Page Margin", true, false, 0.2f, glm::vec4(0.f, 0.f, 0.f, 1.f), glm::vec4(1.f, 1.f, 1.f, 0.f));
		element_configurations.emplace_back("Title Frame", true, false, 0.5f, glm::vec4(0.1f, 0.1f, 0.1f, 1.f), glm::vec4(1.f, 1.f, 1.f, 0.f));
		element_configurations.emplace_back("Calendar Labels", true, false, 0.1f, glm::vec4(0.75f, 0.75f, 0.75f, 0.25f), glm::vec4(0.f, 0.f, 0.f, 1.f));
		element_configurations.emplace_back("Day Shapes", true, true, 0.2f, glm::vec4(0.85f, 0.85f, 0.85f, 1.f), glm::vec4(0.f, 0.f, 0.f, 0.f));
		element_configurations.emplace_back("Sunday Shapes", true, true, 0.2f, glm::vec4(0.85f, 0.85f, 0.85f, 1.f), glm::vec4(0.85f, 0.85f, 0.85f, 1.f));
		element_configurations.emplace_back("Months Shapes", true, false, 0.2f, glm::vec4(0.4f, 0.4f, 0.4f, 1.0f), glm::vec4(0.5f, 0.5f, 0.5f, 0.f));
		element_configurations.emplace_back("Years Shapes", false, false, 0.2f, glm::vec4(0.25f, 0.25f, 0.25f, 1.f), glm::vec4(0.25f, 0.25f, 0.25f, 0.f));
		element_configurations.emplace_back("Years Totals", false, true, 0.2f, glm::vec4(0.25f, 0.75f, 0.25f, 1.f), glm::vec4(0.25f, 0.75f, 0.25f, 1.f));

		number_static_elements = element_configurations.size();



		UpdateElementConfigurations();

		////////////////////////////////////////

		Bind(wxEVT_LISTBOX, &ElementsSetupsPanel::SlotSelectListBook, this);
		Bind(wxEVT_CHECKBOX, &ElementsSetupsPanel::SlotOutlineVisible, this, ID_OUTLINE_VISIBLE);
		Bind(wxEVT_CHECKBOX, &ElementsSetupsPanel::SlotFillVisible, this, ID_FILLING_VISIBLE);
		Bind(wxEVT_SPINCTRLDOUBLE, &ElementsSetupsPanel::SlotLineWidth, this);
		Bind(wxEVT_COLOURPICKER_CHANGED, &ElementsSetupsPanel::SlotLineColor, this, ID_LINE_COLOR_PICKER);
		Bind(wxEVT_COLOURPICKER_CHANGED, &ElementsSetupsPanel::SlotFillColor, this, ID_FILL_COLOR_PICKER);
		Bind(wxEVT_SLIDER, &ElementsSetupsPanel::SlotLineColorAlpha, this, ID_LINE_ALPHA_SLIDER);
		Bind(wxEVT_SLIDER, &ElementsSetupsPanel::SlotFillColorAlpha, this, ID_FILL_ALPHA_SLIDER);

		////////////////////////////////////////


	}

	void SendDefaultValues()
	{
		signal_shape_config(element_configurations);
	}

	void ReceiveDateGroups(const std::vector<DateGroup>& argument_date_groups)
	{
		auto current_dynamic_elements = element_configurations.size() - number_static_elements;

		int adjust_number_elements = static_cast<int>(argument_date_groups.size()) - static_cast<int>(current_dynamic_elements);

		auto newsize = element_configurations.size() + adjust_number_elements;
		element_configurations.resize(newsize);

		if (adjust_number_elements > 0)
		{
			auto start_index = argument_date_groups.size() - adjust_number_elements;

			for (size_t index = start_index; index < argument_date_groups.size(); ++index)
			{
				RectangleShapeConfig temporary(std::string("Bar Group ") + std::to_string(index), true, true, 0.5f, glm::vec4(0.25f, 0.25f, 0.75f, 0.75f), glm::vec4(0.25f, 0.25f, 0.75f, 0.35f));
				temporary.RandomColor(0.75f);
				element_configurations[number_static_elements + index] = temporary;
			}
		}

		UpdateElementConfigurations();
	}
	

	void SaveToXML(pugi::xml_node* node)
	{
		auto child_node = node->append_child("elements_setup");

		for (const auto& element : element_configurations)
		{
			auto element_node = child_node.append_child("element");

			element_node.append_attribute("name").set_value(element.Name().c_str());
			element_node.append_attribute("outline_visible").set_value(element.OutlineVisible());
			element_node.append_attribute("fill_visible").set_value(element.FillVisible());
			element_node.append_attribute("linewidth").set_value(element.LineWidth());

			element_node.append_attribute("outline_color_r").set_value(element.OutlineColor().r);
			element_node.append_attribute("outline_color_g").set_value(element.OutlineColor().g);
			element_node.append_attribute("outline_color_b").set_value(element.OutlineColor().b);
			element_node.append_attribute("outline_color_a").set_value(element.OutlineColor().a);

			element_node.append_attribute("fill_color_r").set_value(element.FillColor().r);
			element_node.append_attribute("fill_color_g").set_value(element.FillColor().g);
			element_node.append_attribute("fill_color_b").set_value(element.FillColor().b);
			element_node.append_attribute("fill_color_a").set_value(element.FillColor().a);
		}
	}
	void LoadFromXML(const pugi::xml_node& node)
	{
		auto child_node = node.child("elements_setup");

		for (auto& element_node : child_node.children("element"))
		{
			auto element_name = element_node.attribute("name").as_string();

			auto config_iterator = RectangleShapeConfig::GetShapeConfig(element_name, &element_configurations);

			config_iterator->OutlineVisible(element_node.attribute("outline_visible").as_bool());
			config_iterator->FillVisible(element_node.attribute("fill_visible").as_bool());
			config_iterator->LineWidth(element_node.attribute("linewidth").as_float());

			glm::vec4 outline_color;
			outline_color.r = element_node.attribute("outline_color_r").as_float();
			outline_color.g = element_node.attribute("outline_color_g").as_float();
			outline_color.b = element_node.attribute("outline_color_b").as_float();
			outline_color.a = element_node.attribute("outline_color_a").as_float();

			config_iterator->OutlineColor(outline_color);

			glm::vec4 fill_color;
			fill_color.r = element_node.attribute("fill_color_r").as_float();
			fill_color.g = element_node.attribute("fill_color_g").as_float();
			fill_color.b = element_node.attribute("fill_color_b").as_float();
			fill_color.a = element_node.attribute("fill_color_a").as_float();

			config_iterator->FillColor(fill_color);

			signal_shape_config(element_configurations);
		}
	}

	sigslot::signal<const std::vector<RectangleShapeConfig>&> signal_shape_config;

private:

	void InitWidgets()
	{
		elements_list_box = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE | wxLB_NEEDED_SB, wxDefaultValidator, wxListBoxNameStr);

		outline_visible_ctrl = new wxCheckBox(this, ID_OUTLINE_VISIBLE, L"Outline Visible");
		fill_visible_ctrl = new wxCheckBox(this, ID_FILLING_VISIBLE, L"Fill Visible");

		line_color_picker = new wxColourPickerCtrl(this, ID_LINE_COLOR_PICKER, *wxStockGDI::GetColour(wxStockGDI::COLOUR_BLACK), wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_ALPHA);
		fill_color_picker = new wxColourPickerCtrl(this, ID_FILL_COLOR_PICKER, *wxStockGDI::GetColour(wxStockGDI::COLOUR_BLACK), wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_ALPHA);

		linewidth_ctrl = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, /*16384L*/ wxSP_ARROW_KEYS | wxALIGN_RIGHT);
		linewidth_ctrl->SetRange(0.0, 10.0);
		linewidth_ctrl->SetDigits(2);
		linewidth_ctrl->SetIncrement(0.05);

		wxSize default_label_size(150, -1);
		linewidth_label = new wxStaticText(this, wxID_ANY, "Line Width", wxDefaultPosition, default_label_size);
		linecolor_label = new wxStaticText(this, wxID_ANY, "Line Color", wxDefaultPosition, default_label_size);
		fillcolor_label = new wxStaticText(this, wxID_ANY, "Fill Color", wxDefaultPosition, default_label_size);
		line_transparency_label = new wxStaticText(this, wxID_ANY, "Transparency", wxDefaultPosition, default_label_size);
		fill_transparency_label = new wxStaticText(this, wxID_ANY, "Transparency", wxDefaultPosition, default_label_size);

		line_color_alpha_slider = new wxSlider(this, ID_LINE_ALPHA_SLIDER, 0, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
		fill_color_alpha_slider = new wxSlider(this, ID_FILL_ALPHA_SLIDER, 0, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
	}
	void InitSizers()
	{
		std::array<wxSizerFlags, 3> sizer_flags;
		sizer_flags[0].Proportion(0).Expand();
		sizer_flags[1].Proportion(0).CenterVertical().Border(wxALL, 5);
		sizer_flags[2].Proportion(1).Expand().Border(wxALL, 5);

		wxBoxSizer* vertical_sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(vertical_sizer);


		wxStaticBoxSizer* static_box_sizer_elements = new wxStaticBoxSizer(wxVERTICAL, this, "Elements");
		wxStaticBoxSizer* static_box_sizer_outline = new wxStaticBoxSizer(wxVERTICAL, this, "Outline");
		wxStaticBoxSizer* static_box_sizer_fill = new wxStaticBoxSizer(wxVERTICAL, this, "Fill");

		vertical_sizer->Add(static_box_sizer_elements, sizer_flags[0]);
		vertical_sizer->Add(static_box_sizer_outline, sizer_flags[0]);
		vertical_sizer->Add(static_box_sizer_fill, sizer_flags[0]);


		std::array<wxBoxSizer*, 4> horizontal_sizers_outline;
		for (auto& horizontal_sizer : horizontal_sizers_outline)
		{
			horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);
			static_box_sizer_outline->Add(horizontal_sizer, sizer_flags[0]);
		}

		std::array<wxBoxSizer*, 3> horizontal_sizers_fill;
		for (auto& horizontal_sizer : horizontal_sizers_fill)
		{
			horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);
			static_box_sizer_fill->Add(horizontal_sizer, sizer_flags[0]);
		}

		static_box_sizer_elements->Add(elements_list_box, sizer_flags[2]);

		horizontal_sizers_outline[0]->Add(outline_visible_ctrl, sizer_flags[2]);
		horizontal_sizers_outline[1]->Add(linewidth_label, sizer_flags[1]);
		horizontal_sizers_outline[1]->Add(linewidth_ctrl, sizer_flags[2]);
		horizontal_sizers_outline[2]->Add(linecolor_label, sizer_flags[1]);
		horizontal_sizers_outline[2]->Add(line_color_picker, sizer_flags[2]);
		horizontal_sizers_outline[3]->Add(line_transparency_label, sizer_flags[1]);
		horizontal_sizers_outline[3]->Add(line_color_alpha_slider, sizer_flags[2]);

		horizontal_sizers_fill[0]->Add(fill_visible_ctrl, sizer_flags[2]);
		horizontal_sizers_fill[1]->Add(fillcolor_label, sizer_flags[1]);
		horizontal_sizers_fill[1]->Add(fill_color_picker, sizer_flags[2]);
		horizontal_sizers_fill[2]->Add(fill_transparency_label, sizer_flags[1]);
		horizontal_sizers_fill[2]->Add(fill_color_alpha_slider, sizer_flags[2]);

		vertical_sizer->Layout();
	}

	void UpdateElementConfigurations()
	{
		elements_list_box->Clear();

		for (size_t index = 0; index < element_configurations.size(); ++index)
		{
			elements_list_box->AppendString(element_configurations[index].Name());
		}

		size_t default_selection = 0;
		elements_list_box->Select(default_selection);
		UpdateWidgets(default_selection);

		signal_shape_config(element_configurations);
	}

	void UpdateWidgets(size_t config_index)
	{
		auto& current_element = element_configurations[config_index];

		auto outline_visible = current_element.OutlineVisible();

		outline_visible_ctrl->SetValue(outline_visible);
		linewidth_ctrl->Enable(outline_visible);
		line_color_picker->Enable(outline_visible);
		line_color_alpha_slider->Enable(outline_visible);

		auto fill_visible = current_element.FillVisible();

		fill_visible_ctrl->SetValue(fill_visible);
		fill_color_picker->Enable(fill_visible);
		fill_color_alpha_slider->Enable(fill_visible);

		linewidth_ctrl->SetValue(current_element.LineWidthDisabled());

		line_color_picker->SetColour(to_wx_color(current_element.OutlineColorDisabled()));
		line_color_alpha_slider->SetValue(100 - static_cast<int>(current_element.OutlineColorDisabled().a * 100.f));

		fill_color_picker->SetColour(to_wx_color(current_element.FillColorDisabled()));
		fill_color_alpha_slider->SetValue(100 - static_cast<int>(current_element.FillColorDisabled().a * 100.f));
	}

	void SlotSelectListBook(wxCommandEvent& event)
	{
		size_t selection_index = static_cast<size_t>(event.GetSelection());
		UpdateWidgets(selection_index);
	}
	void SlotOutlineVisible(wxCommandEvent& event)
	{
		auto check_status = event.IsChecked();
		size_t element_selection = static_cast<size_t>(elements_list_box->GetSelection());
		element_configurations[element_selection].OutlineVisible(check_status);
		UpdateWidgets(element_selection);

		signal_shape_config(element_configurations);
	}
	void SlotFillVisible(wxCommandEvent& event)
	{
		auto check_status = event.IsChecked();
		size_t element_selection = static_cast<size_t>(elements_list_box->GetSelection());
		element_configurations[element_selection].FillVisible(check_status);
		UpdateWidgets(element_selection);

		signal_shape_config(element_configurations);
	}
	void SlotLineWidth(wxSpinDoubleEvent& event)
	{
		auto linewidth = event.GetValue();
		size_t element_selection = static_cast<size_t>(elements_list_box->GetSelection());
		element_configurations[element_selection].LineWidth(linewidth);

		signal_shape_config(element_configurations);
	}
	void SlotLineColor(wxColourPickerEvent& event)
	{
		auto color = to_glm_color(event.GetColour());
		size_t element_selection = static_cast<size_t>(elements_list_box->GetSelection());

		auto current_line_color_alpha = element_configurations[element_selection].OutlineColor().a;
		color.a = current_line_color_alpha;

		element_configurations[element_selection].OutlineColor(color);

		signal_shape_config(element_configurations);
	}
	void SlotFillColor(wxColourPickerEvent& event)
	{
		auto color = to_glm_color(event.GetColour());
		size_t element_selection = static_cast<size_t>(elements_list_box->GetSelection());

		auto current_line_color_alpha = element_configurations[element_selection].FillColor().a;
		color.a = current_line_color_alpha;

		element_configurations[element_selection].FillColor(color);

		signal_shape_config(element_configurations);
	}
	void SlotLineColorAlpha(wxCommandEvent& event)
	{
		auto slider_value = static_cast<float>(event.GetInt());
		float alpha_value = 1.f - (slider_value / 100.f);

		size_t element_selection = static_cast<size_t>(elements_list_box->GetSelection());
		auto current_line_color = element_configurations[element_selection].OutlineColor();
		current_line_color.a = alpha_value;
		element_configurations[element_selection].OutlineColor(current_line_color);

		signal_shape_config(element_configurations);
	}
	void SlotFillColorAlpha(wxCommandEvent& event)
	{
		auto slider_value = static_cast<float>(event.GetInt());
		float alpha_value = 1.f - (slider_value / 100.f);

		size_t element_selection = static_cast<size_t>(elements_list_box->GetSelection());
		auto current_line_color = element_configurations[element_selection].FillColor();
		current_line_color.a = alpha_value;
		element_configurations[element_selection].FillColor(current_line_color);

		signal_shape_config(element_configurations);
	}

	wxListBox* elements_list_box;
	wxCheckBox* outline_visible_ctrl;
	wxCheckBox* fill_visible_ctrl;
	wxSpinCtrlDouble* linewidth_ctrl;
	//wxSpinCtrlDouble* line_transparency_ctrl;
	//wxSpinCtrlDouble* fill_transparency_ctrl;
	wxColourPickerCtrl* line_color_picker;
	wxColourPickerCtrl* fill_color_picker;
	wxSlider* line_color_alpha_slider;
	wxSlider* fill_color_alpha_slider;
	wxStaticText* linewidth_label;
	wxStaticText* linecolor_label;
	wxStaticText* fillcolor_label;
	wxStaticText* line_transparency_label;
	wxStaticText* fill_transparency_label;

	std::vector<RectangleShapeConfig> element_configurations;
	size_t number_static_elements;

	const int ID_OUTLINE_VISIBLE;
	const int ID_FILLING_VISIBLE;
	const int ID_LINE_COLOR_PICKER;
	const int ID_FILL_COLOR_PICKER;
	const int ID_LINE_ALPHA_SLIDER;
	const int ID_FILL_ALPHA_SLIDER;
};
