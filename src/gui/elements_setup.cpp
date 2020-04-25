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


#include "elements_setup.h"


ElementsSetupsPanel::ElementsSetupsPanel(wxWindow* parent) :
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxPanelNameStr), 
	ID_OUTLINE_VISIBLE(wxNewId()),
	ID_FILLING_VISIBLE(wxNewId()),
	ID_LINE_COLOR_PICKER(wxNewId()),
	ID_FILL_COLOR_PICKER(wxNewId()),
	ID_LINE_ALPHA_SLIDER(wxNewId()),
	ID_FILL_ALPHA_SLIDER(wxNewId())
{


	wxBoxSizer* vertical_sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(vertical_sizer);

	wxBoxSizer* select_list_box_sizer = new wxBoxSizer(wxHORIZONTAL);
	vertical_sizer->Add(select_list_box_sizer, 0, wxEXPAND);

	elements_list_box = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE | wxLB_NEEDED_SB, wxDefaultValidator, wxListBoxNameStr);
	select_list_box_sizer->Add(elements_list_box, 1, wxEXPAND | wxALL, 5);

	Bind(wxEVT_LISTBOX, &ElementsSetupsPanel::SlotSelectListBook, this);

	wxBoxSizer* select_book_sizer = new wxBoxSizer(wxHORIZONTAL);
	vertical_sizer->Add(select_book_sizer, 0, wxEXPAND);
	
	element_configurations.emplace_back(L"Page Margin", true, false, 0.5f, glm::vec4(0.f, 0.f, 0.f, 1.f), glm::vec4(1.f, 1.f, 1.f, 0.f));
	element_configurations.emplace_back(L"Title Frame", true, false, 0.2f, glm::vec4(0.f, 0.f, 0.f, 1.f), glm::vec4(1.f, 1.f, 1.f, 0.f));
	element_configurations.emplace_back(L"Calendar Labels", true, false, 0.2f, glm::vec4(1.f, 1.f, 1.f, 0.f), glm::vec4(0.f, 0.f, 0.f, 1.f));
	element_configurations.emplace_back(L"Day Shapes", true, true, 0.2f, glm::vec4(0.75f, 0.75f, 0.75f, 1.f), glm::vec4(0.f, 0.f, 0.f, 0.f));
	element_configurations.emplace_back(L"Sunday Shapes", true, true, 0.2f, glm::vec4(0.75f, 0.75f, 0.75f, 1.f), glm::vec4(0.75f, 0.75f, 0.75f, 1.f));
	element_configurations.emplace_back(L"Months Shapes", true, true, 0.2f, glm::vec4(0.25f, 0.25f, 0.25f, 1.f), glm::vec4(0.25f, 0.25f, 0.25f, 0.f));
	element_configurations.emplace_back(L"Years Shapes", false, false, 0.2f, glm::vec4(0.25f, 0.25f, 0.25f, 1.f), glm::vec4(0.25f, 0.25f, 0.25f, 0.f));
	element_configurations.emplace_back(L"Bars Shapes", true, true, 0.5f, glm::vec4(0.25f, 0.25f, 0.75f, 0.75f), glm::vec4(0.25f, 0.25f, 0.75f, 0.35f));
	element_configurations.emplace_back(L"Years Totals", false, true, 0.2f, glm::vec4(0.25f, 0.75f, 0.25f, 1.f), glm::vec4(0.25f, 0.75f, 0.25f, 1.f));
	
	////////////////////////////////////////
	////////////////////////////////////////

	for (size_t index = 0; index < element_configurations.size(); ++index)
	{
		elements_list_box->AppendString(element_configurations[index].Name());
	}
	size_t default_selection = 0;
	elements_list_box->Select(default_selection);
	

	////////////////////////////////////////

	
	std::array<wxBoxSizer*, 7> horizontal_sizers;
	for (auto& horizontal_sizer : horizontal_sizers)
	{
		horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);
		vertical_sizer->Add(horizontal_sizer, 0, wxEXPAND);
	}
	
	outline_visible = new wxCheckBox(this, ID_OUTLINE_VISIBLE, L"Outline Visible");
	horizontal_sizers[0]->Add(outline_visible, 0, wxEXPAND | wxALL, 5);

	wxStaticText* linewidth_label = new wxStaticText(this, wxID_ANY, L"Line Width");
	linewidth_label->SetMinSize(wxSize(100, -1));
	horizontal_sizers[1]->Add(linewidth_label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	linewidth_ctrl = new wxSpinCtrlDouble(this, wxID_ANY);
	linewidth_ctrl->SetRange(0.0, 10.0);
	linewidth_ctrl->SetDigits(2);
	linewidth_ctrl->SetIncrement(0.1);
	horizontal_sizers[1]->Add(linewidth_ctrl, 1, wxEXPAND | wxALL, 5);

	wxStaticText* linecolor_label = new wxStaticText(this, wxID_ANY, L"Line Color");
	linecolor_label->SetMinSize(wxSize(100, -1));
	horizontal_sizers[2]->Add(linecolor_label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	line_color_picker = new wxColourPickerCtrl(this, ID_LINE_COLOR_PICKER, *wxStockGDI::GetColour(wxStockGDI::COLOUR_BLACK),
		wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_ALPHA);
	horizontal_sizers[2]->Add(line_color_picker, 1, wxEXPAND | wxALL, 5);

	fill_visible = new wxCheckBox(this, ID_FILLING_VISIBLE, L"Fill Visible");
	horizontal_sizers[4]->Add(fill_visible, 0, wxEXPAND | wxALL, 5);

	wxStaticText* fillcolor_label = new wxStaticText(this, wxID_ANY, L"Fill Color");
	fillcolor_label->SetMinSize(wxSize(100, -1));
	horizontal_sizers[5]->Add(fillcolor_label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	fill_color_picker = new wxColourPickerCtrl(this, ID_FILL_COLOR_PICKER, *wxStockGDI::GetColour(wxStockGDI::COLOUR_BLACK),
		wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_ALPHA);
	horizontal_sizers[5]->Add(fill_color_picker, 1, wxEXPAND | wxALL, 5);

	wxStaticText* line_transparency_label = new wxStaticText(this, wxID_ANY, L"Outline Transparency");
	line_transparency_label->SetMinSize(wxSize(100, -1));
	horizontal_sizers[3]->Add(line_transparency_label, 0,  wxALL, 5);

	line_color_alpha_slider = new wxSlider(this, ID_LINE_ALPHA_SLIDER, 0, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS | wxSL_BOTTOM);
	horizontal_sizers[3]->Add(line_color_alpha_slider, 1, wxEXPAND | wxALL, 5);

	wxStaticText* fill_transparency_label = new wxStaticText(this, wxID_ANY, L"Fill Transparency");
	fill_transparency_label->SetMinSize(wxSize(100, -1));
	horizontal_sizers[6]->Add(fill_transparency_label, 0,  wxALL, 5);

	fill_color_alpha_slider = new wxSlider(this, ID_FILL_ALPHA_SLIDER, 0, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS | wxSL_BOTTOM);
	horizontal_sizers[6]->Add(fill_color_alpha_slider, 1, wxEXPAND | wxALL, 5);

	vertical_sizer->Layout();


	UpdateWidgets(default_selection);

	////////////////////////////////////////

	Bind(wxEVT_CHECKBOX, &ElementsSetupsPanel::SlotOutlineVisible, this, ID_OUTLINE_VISIBLE);
	Bind(wxEVT_CHECKBOX, &ElementsSetupsPanel::SlotFillVisible, this, ID_FILLING_VISIBLE);
	Bind(wxEVT_SPINCTRLDOUBLE, &ElementsSetupsPanel::SlotLineWidth, this);
	Bind(wxEVT_COLOURPICKER_CHANGED, &ElementsSetupsPanel::SlotLineColor, this, ID_LINE_COLOR_PICKER);
	Bind(wxEVT_COLOURPICKER_CHANGED, &ElementsSetupsPanel::SlotFillColor, this, ID_FILL_COLOR_PICKER);
	Bind(wxEVT_SLIDER, &ElementsSetupsPanel::SlotLineColorAlpha, this, ID_LINE_ALPHA_SLIDER); //wxEVT_SLIDER wxEVT_SCROLL_THUMBTRACK
	Bind(wxEVT_SLIDER, &ElementsSetupsPanel::SlotFillColorAlpha, this, ID_FILL_ALPHA_SLIDER); //wxEVT_SLIDER wxEVT_SCROLL_THUMBTRACK

	////////////////////////////////////////


}


void ElementsSetupsPanel::SendDefaultValues()
{
	signal_shape_config(element_configurations);
}

void ElementsSetupsPanel::UpdateWidgets(size_t config_index)
{
	auto& current_element = element_configurations[config_index];

	auto outline_visible_status = current_element.OutlineVisible();
	outline_visible->SetValue(outline_visible_status);

	linewidth_ctrl->Enable(outline_visible_status);
	line_color_picker->Enable(outline_visible_status);
	line_color_alpha_slider->Enable(outline_visible_status);

	auto fill_visible_status = current_element.FillVisible();
	fill_visible->SetValue(fill_visible_status);

	fill_color_picker->Enable(fill_visible_status);
	fill_color_alpha_slider->Enable(fill_visible_status);

	linewidth_ctrl->SetValue(current_element.LineWidth());
	line_color_picker->SetColour(to_wx_color(current_element.OutlineColor()));
	fill_color_picker->SetColour(to_wx_color(current_element.FillColor()));

	line_color_alpha_slider->SetValue(100 - static_cast<int>(current_element.OutlineColor().a * 100.f));
	fill_color_alpha_slider->SetValue(100 - static_cast<int>(current_element.FillColor().a * 100.f));
}

void ElementsSetupsPanel::SlotSelectListBook(wxCommandEvent& event)
{
	size_t selection_index = static_cast<size_t>(event.GetSelection());
	UpdateWidgets(selection_index);
}

void ElementsSetupsPanel::SlotOutlineVisible(wxCommandEvent& event)
{
	auto check_status = event.IsChecked();
	size_t element_selection = static_cast<size_t>(elements_list_box->GetSelection());
	element_configurations[element_selection].OutlineVisible(check_status);
	UpdateWidgets(element_selection);

	signal_shape_config(element_configurations);
}

void ElementsSetupsPanel::SlotFillVisible(wxCommandEvent& event)
{
	auto check_status = event.IsChecked();
	size_t element_selection = static_cast<size_t>(elements_list_box->GetSelection());
	element_configurations[element_selection].FillVisible(check_status);
	UpdateWidgets(element_selection);

	signal_shape_config(element_configurations);
}

void ElementsSetupsPanel::SlotLineWidth(wxSpinDoubleEvent& event)
{
	auto linewidth = event.GetValue();
	size_t element_selection = static_cast<size_t>(elements_list_box->GetSelection());
	element_configurations[element_selection].LineWidth(linewidth);

	signal_shape_config(element_configurations);
}

void ElementsSetupsPanel::SlotLineColor(wxColourPickerEvent& event)
{
	auto color = to_glm_color(event.GetColour());
	size_t element_selection = static_cast<size_t>(elements_list_box->GetSelection());
	
	auto current_line_color_alpha = element_configurations[element_selection].OutlineColor().a;
	color.a = current_line_color_alpha;

	element_configurations[element_selection].OutlineColor(color);

	signal_shape_config(element_configurations);
}

void ElementsSetupsPanel::SlotFillColor(wxColourPickerEvent& event)
{
	auto color = to_glm_color(event.GetColour());
	size_t element_selection = static_cast<size_t>(elements_list_box->GetSelection());

	auto current_line_color_alpha = element_configurations[element_selection].FillColor().a;
	color.a = current_line_color_alpha;

	element_configurations[element_selection].FillColor(color);

	signal_shape_config(element_configurations);
}

void ElementsSetupsPanel::SlotLineColorAlpha(wxCommandEvent& event)
{
	auto slider_value = static_cast<float>(event.GetInt());
	float alpha_value = 1.f - (slider_value / 100.f);

	size_t element_selection = static_cast<size_t>(elements_list_box->GetSelection());
	auto current_line_color = element_configurations[element_selection].OutlineColor();
	current_line_color.a = alpha_value;
	element_configurations[element_selection].OutlineColor(current_line_color);

	signal_shape_config(element_configurations);
}

void ElementsSetupsPanel::SlotFillColorAlpha(wxCommandEvent& event)
{
	auto slider_value = static_cast<float>(event.GetInt());
	float alpha_value = 1.f - (slider_value / 100.f);

	size_t element_selection = static_cast<size_t>(elements_list_box->GetSelection());
	auto current_line_color = element_configurations[element_selection].FillColor();
	current_line_color.a = alpha_value;
	element_configurations[element_selection].FillColor(current_line_color);

	signal_shape_config(element_configurations);
}


void ElementsSetupsPanel::SaveToXML(pugi::xml_node* node)
{
	auto child_node = node->append_child(L"elements_setup");

	for (const auto& element : element_configurations)
	{
		auto element_node = child_node.append_child(L"element");

		element_node.append_attribute(L"name").set_value(element.Name().c_str());
		element_node.append_attribute(L"outline_visible").set_value(element.OutlineVisible());
		element_node.append_attribute(L"fill_visible").set_value(element.FillVisible());
		element_node.append_attribute(L"linewidth").set_value(element.LineWidth());

		element_node.append_attribute(L"outline_color_r").set_value(element.OutlineColor().r);
		element_node.append_attribute(L"outline_color_g").set_value(element.OutlineColor().g);
		element_node.append_attribute(L"outline_color_b").set_value(element.OutlineColor().b);
		element_node.append_attribute(L"outline_color_a").set_value(element.OutlineColor().a);

		element_node.append_attribute(L"fill_color_r").set_value(element.FillColor().r);
		element_node.append_attribute(L"fill_color_g").set_value(element.FillColor().g);
		element_node.append_attribute(L"fill_color_b").set_value(element.FillColor().b);
		element_node.append_attribute(L"fill_color_a").set_value(element.FillColor().a);
	}
}


void ElementsSetupsPanel::LoadFromXML(const pugi::xml_node& node)
{
	auto child_node = node.child(L"elements_setup");

	for (auto& element_node : child_node.children(L"element"))
	{
		auto element_name = element_node.attribute(L"name").as_string();

		auto config_iterator = RectangleShapeConfig::GetShapeConfig(element_name, &element_configurations);

		config_iterator->OutlineVisible(element_node.attribute(L"outline_visible").as_bool());
		config_iterator->FillVisible(element_node.attribute(L"fill_visible").as_bool());
		config_iterator->LineWidth(element_node.attribute(L"linewidth").as_float());

		glm::vec4 outline_color;
		outline_color.r = element_node.attribute(L"outline_color_r").as_float();
		outline_color.g = element_node.attribute(L"outline_color_g").as_float();
		outline_color.b = element_node.attribute(L"outline_color_b").as_float();
		outline_color.a = element_node.attribute(L"outline_color_a").as_float();

		config_iterator->OutlineColor(outline_color);

		glm::vec4 fill_color;
		fill_color.r = element_node.attribute(L"fill_color_r").as_float();
		fill_color.g = element_node.attribute(L"fill_color_g").as_float();
		fill_color.b = element_node.attribute(L"fill_color_b").as_float();
		fill_color.a = element_node.attribute(L"fill_color_a").as_float();

		config_iterator->FillColor(fill_color);

		signal_shape_config(element_configurations);
	}
}
