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


#ifdef WX_PRECOMP
#include <wx/wxprec.h>
#else 
#include <wx/wx.h>
#endif

#include <wx/panel.h>
#include <wx/window.h>
#include <wx/sizer.h>
#include <wx/listbox.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/clrpicker.h>
#include <wx/slider.h>
#include <wx/statbox.h>

#include <array>
#include <vector>
#include <algorithm>
#include <string>

#include <memory>
#include <functional>

#include <sigslot/signal.hpp>

#include <pugixml.hpp>


class ElementsSetupsPanel : public wxPanel
{
public:
	
	ElementsSetupsPanel(wxWindow* parent);

	void SendDefaultValues();

	void ReceiveDateGroups(const std::vector<DateGroup>& argument_date_groups);
	

	void SaveToXML(pugi::xml_node* node);
	void LoadFromXML(const pugi::xml_node& node);

	sigslot::signal<const std::vector<RectangleShapeConfig>&> signal_shape_config;

private:

	void InitWidgets();
	void InitSizers();

	void UpdateElementConfigurations();

	void UpdateWidgets(size_t config_index);

	void SlotSelectListBook(wxCommandEvent& event);
	void SlotOutlineVisible(wxCommandEvent& event);
	void SlotFillVisible(wxCommandEvent& event);
	void SlotLineWidth(wxSpinDoubleEvent& event);
	void SlotLineColor(wxColourPickerEvent& event);
	void SlotFillColor(wxColourPickerEvent& event);
	void SlotLineColorAlpha(wxCommandEvent& event);
	void SlotFillColorAlpha(wxCommandEvent& event);

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
