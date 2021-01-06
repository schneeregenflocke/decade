/*
Decade
Copyright (c) 2019-2020 Marco Peyer

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110 - 1301 USA.
*/

#pragma once

#include "../shape_config.h"
#include "../group_store.h"
#include "../casts.h"

#include "wx_widgets_include.h"

#include <array>
#include <vector>
#include <algorithm>
#include <string>

#include <memory>
#include <functional>

#include <sigslot/signal.hpp>



class ElementsSetupsPanel : public wxPanel
{
public:
	
	ElementsSetupsPanel(wxWindow* parent) :
		wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxPanelNameStr)
	{
		InitWidgets();
		InitSizers();

		Bind(wxEVT_LISTBOX, &ElementsSetupsPanel::SlotListBook, this);
		Bind(wxEVT_CHECKBOX, &ElementsSetupsPanel::SlotCheckBox, this);
		Bind(wxEVT_SPINCTRLDOUBLE, &ElementsSetupsPanel::SlotSpinControlDouble, this);
		Bind(wxEVT_COLOURPICKER_CHANGED, &ElementsSetupsPanel::SlotColorPicker, this);
		Bind(wxEVT_SLIDER, &ElementsSetupsPanel::SlotSlider, this);
	}

	void ReceiveShapeConfigurationStorage(const ShapeConfigurationStorage& shape_configuration_storage)
	{
		this->shape_configuration_storage = shape_configuration_storage;

		UpdateConfigurationList();
	}

	void ReceiveDateGroups(const std::vector<DateGroup>& date_groups)
	{
		const size_t number_dynamic_configurations = shape_configuration_storage.size() - shape_configuration_storage.GetNumberPersistentConfigurations();

		int adjust_number_dynamic_configurations = static_cast<int>(date_groups.size()) - static_cast<int>(number_dynamic_configurations);

		auto adjustment = shape_configuration_storage.size() + adjust_number_dynamic_configurations;
		shape_configuration_storage.resize(adjustment);

		if (adjust_number_dynamic_configurations > 0)
		{
			size_t index = date_groups.size() - adjust_number_dynamic_configurations;

			for (; index < date_groups.size(); ++index)
			{
				ShapeConfiguration temporary(std::string("Bar Group ") + std::to_string(index), true, true, 0.5f, glm::vec4(0.25f, 0.25f, 0.75f, 0.75f), glm::vec4(0.25f, 0.25f, 0.75f, 0.35f));
				temporary.RandomColor();
				shape_configuration_storage[shape_configuration_storage.GetNumberPersistentConfigurations() + index] = temporary;
			}	
		}

		UpdateConfigurationList();

		signal_shape_configuration_storage(shape_configuration_storage);
	}
	
	sigslot::signal<const ShapeConfigurationStorage&> signal_shape_configuration_storage;

private:

	void InitWidgets()
	{
		shape_configuration_list_box = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE | wxLB_NEEDED_SB, wxDefaultValidator, wxListBoxNameStr);

		outline_visible_ctrl = new wxCheckBox(this, wxID_ANY, L"Outline Visible");
		fill_visible_ctrl = new wxCheckBox(this, wxID_ANY, L"Fill Visible");

		line_color_picker = new wxColourPickerCtrl(this, wxID_ANY, *wxStockGDI::GetColour(wxStockGDI::COLOUR_BLACK), wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_ALPHA);
		fill_color_picker = new wxColourPickerCtrl(this, wxID_ANY, *wxStockGDI::GetColour(wxStockGDI::COLOUR_BLACK), wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_ALPHA);

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

		line_color_alpha_slider = new wxSlider(this, wxID_ANY, 0, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
		fill_color_alpha_slider = new wxSlider(this, wxID_ANY, 0, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
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

		vertical_sizer->Add(static_box_sizer_elements, sizer_flags[2]);
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

		static_box_sizer_elements->Add(shape_configuration_list_box, sizer_flags[2]);

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

	void UpdateConfigurationList()
	{
		auto selection = shape_configuration_list_box->GetSelection();

		shape_configuration_list_box->Clear();

		for (size_t index = 0; index < shape_configuration_storage.size(); ++index)
		{
			shape_configuration_list_box->AppendString(shape_configuration_storage[index].Name());
		}

		const int number_of_items = shape_configuration_list_box->GetCount();
		selection = std::clamp(selection, 1, number_of_items);
		shape_configuration_list_box->Select(selection);
		UpdateWidgetForSelection(selection);
	}

	void UpdateWidgetForSelection(size_t selection)
	{
		auto& current_configuration = shape_configuration_storage[selection];

		auto outline_visible = current_configuration.OutlineVisible();
		outline_visible_ctrl->SetValue(outline_visible);
		linewidth_ctrl->Enable(outline_visible);
		line_color_picker->Enable(outline_visible);
		line_color_alpha_slider->Enable(outline_visible);

		auto fill_visible = current_configuration.FillVisible();
		fill_visible_ctrl->SetValue(fill_visible);
		fill_color_picker->Enable(fill_visible);
		fill_color_alpha_slider->Enable(fill_visible);

		linewidth_ctrl->SetValue(current_configuration.LineWidthDisabled());

		line_color_picker->SetColour(to_wx_color(current_configuration.OutlineColorDisabled()));
		line_color_alpha_slider->SetValue(100 - static_cast<int>(current_configuration.OutlineColorDisabled().a * 100.f));

		fill_color_picker->SetColour(to_wx_color(current_configuration.FillColorDisabled()));
		fill_color_alpha_slider->SetValue(100 - static_cast<int>(current_configuration.FillColorDisabled().a * 100.f));
	}

	void SlotListBook(wxCommandEvent& event)
	{
		size_t selection_index = static_cast<size_t>(event.GetSelection());
		UpdateWidgetForSelection(selection_index);
	}

	void SlotCheckBox(wxCommandEvent& event)
	{
		auto check_status = event.IsChecked();
		size_t selection = static_cast<size_t>(shape_configuration_list_box->GetSelection());

		if (event.GetEventObject() == outline_visible_ctrl)
		{
			shape_configuration_storage[selection].OutlineVisible(check_status);
		}

		if (event.GetEventObject() == outline_visible_ctrl)
		{
			shape_configuration_storage[selection].FillVisible(check_status);
		}

		UpdateWidgetForSelection(selection);

		signal_shape_configuration_storage(shape_configuration_storage);
	}

	void SlotSpinControlDouble(wxSpinDoubleEvent& event)
	{
		auto line_width = event.GetValue();
		size_t element_selection = static_cast<size_t>(shape_configuration_list_box->GetSelection());
		shape_configuration_storage[element_selection].LineWidth(line_width);

		signal_shape_configuration_storage(shape_configuration_storage);
	}

	void SlotColorPicker(wxColourPickerEvent& event)
	{
		auto color = to_glm_vec4(event.GetColour());
		size_t selection = static_cast<size_t>(shape_configuration_list_box->GetSelection());

		if (event.GetEventObject() == line_color_picker)
		{
			auto current_line_color_alpha = shape_configuration_storage[selection].OutlineColor().a;
			color.a = current_line_color_alpha;
			shape_configuration_storage[selection].OutlineColor(color);
		}

		if (event.GetEventObject() == fill_color_picker)
		{
			auto current_line_color_alpha = shape_configuration_storage[selection].FillColor().a;
			color.a = current_line_color_alpha;
			shape_configuration_storage[selection].FillColor(color);
		}

		signal_shape_configuration_storage(shape_configuration_storage);
	}

	void SlotSlider(wxCommandEvent& event)
	{
		auto slider_value = static_cast<float>(event.GetInt());
		float alpha_value = 1.f - (slider_value / 100.f);

		size_t selection = static_cast<size_t>(shape_configuration_list_box->GetSelection());

		if (event.GetEventObject() == line_color_alpha_slider)
		{
			auto current_line_color = shape_configuration_storage[selection].OutlineColor();
			current_line_color.a = alpha_value;
			shape_configuration_storage[selection].OutlineColor(current_line_color);
		}

		if (event.GetEventObject() == fill_color_alpha_slider)
		{
			auto current_line_color = shape_configuration_storage[selection].FillColor();
			current_line_color.a = alpha_value;
			shape_configuration_storage[selection].FillColor(current_line_color);
		}

		signal_shape_configuration_storage(shape_configuration_storage);
	}

	ShapeConfigurationStorage shape_configuration_storage;

	wxListBox* shape_configuration_list_box;
	wxCheckBox* outline_visible_ctrl;
	wxCheckBox* fill_visible_ctrl;
	wxSpinCtrlDouble* linewidth_ctrl;
	wxColourPickerCtrl* line_color_picker;
	wxColourPickerCtrl* fill_color_picker;
	wxSlider* line_color_alpha_slider;
	wxSlider* fill_color_alpha_slider;
	
	wxStaticText* linewidth_label;
	wxStaticText* linecolor_label;
	wxStaticText* fillcolor_label;
	wxStaticText* line_transparency_label;
	wxStaticText* fill_transparency_label;
};
