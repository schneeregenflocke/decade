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


CalendarSetupPanel::CalendarSetupPanel(wxWindow* parent) :
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxPanelNameStr),
	ID_WEIGHT0(wxNewId()),
	ID_WEIGHT1(wxNewId()),
	ID_WEIGHT2(wxNewId()),
	ID_GAP_FACTOR(wxNewId()),
	ID_SPAN_FROM(wxNewId()),
	ID_SPAN_TO(wxNewId()),
	ID_AUTO_SPAN(wxNewId())
{
	wxBoxSizer* vertical_sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(vertical_sizer);

	std::array<wxBoxSizer*, 7> horizontal_sizers;
	for (auto& sizer : horizontal_sizers)
	{
		sizer = new wxBoxSizer(wxHORIZONTAL);
		vertical_sizer->Add(sizer, 0, wxEXPAND);
	}

	wxStaticText* weight0_label = new wxStaticText(this, wxID_ANY, L"Subrow Weight 0");
	weight0_spin_ctrl = new  wxSpinCtrlDouble(this, ID_WEIGHT0);
	weight0_spin_ctrl->SetValue(1.f);
	weight0_spin_ctrl->SetIncrement(0.1f);
	horizontal_sizers[0]->Add(weight0_label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	horizontal_sizers[0]->Add(weight0_spin_ctrl, 1, wxEXPAND | wxALL, 5);

	wxStaticText* weight1_label = new wxStaticText(this, wxID_ANY, L"Subrow Weight 1");
	weight1_spin_ctrl = new  wxSpinCtrlDouble(this, ID_WEIGHT1);
	weight1_spin_ctrl->SetValue(2.f);
	weight1_spin_ctrl->SetIncrement(0.1f);
	horizontal_sizers[1]->Add(weight1_label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	horizontal_sizers[1]->Add(weight1_spin_ctrl, 1, wxEXPAND | wxALL, 5);

	wxStaticText* weight2_label = new wxStaticText(this, wxID_ANY, L"Subrow Weight 2");
	weight2_spin_ctrl = new  wxSpinCtrlDouble(this, ID_WEIGHT2);
	weight2_spin_ctrl->SetValue(2.f);
	weight2_spin_ctrl->SetIncrement(0.1f);
	horizontal_sizers[2]->Add(weight2_label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	horizontal_sizers[2]->Add(weight2_spin_ctrl, 1, wxEXPAND | wxALL, 5);

	wxStaticText* gap_factor_label = new wxStaticText(this, wxID_ANY, L"Gap Factor");
	gap_factor_spin_ctrl = new wxSpinCtrlDouble(this, ID_GAP_FACTOR);
	gap_factor_spin_ctrl->SetValue(0.15f);
	gap_factor_spin_ctrl->SetIncrement(0.01f);
	horizontal_sizers[3]->Add(gap_factor_label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	horizontal_sizers[3]->Add(gap_factor_spin_ctrl, 1, wxEXPAND | wxALL, 5);

	wxStaticText* calendar_span_from_label = new wxStaticText(this, wxID_ANY, L"Calendar From Year");
	calendar_span_from_spin_ctrl = new wxSpinCtrl(this, ID_SPAN_FROM);
	calendar_span_from_spin_ctrl->SetRange(1400, 9999);
	calendar_span_from_spin_ctrl->SetValue(2020);
	//weight2_spin_ctrl->SetIncrement(0.1f);
	horizontal_sizers[4]->Add(calendar_span_from_label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	horizontal_sizers[4]->Add(calendar_span_from_spin_ctrl, 1, wxEXPAND | wxALL, 5);

	wxStaticText* calendar_span_to_label = new wxStaticText(this, wxID_ANY, L"Calendar To Year");
	calendar_span_to_spin_ctrl = new wxSpinCtrl(this, ID_SPAN_TO);
	calendar_span_to_spin_ctrl->SetRange(1400, 9999);
	calendar_span_to_spin_ctrl->SetValue(2030);
	//gap_factor_spin_ctrl->SetIncrement(0.01f);
	horizontal_sizers[5]->Add(calendar_span_to_label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	horizontal_sizers[5]->Add(calendar_span_to_spin_ctrl, 1, wxEXPAND | wxALL, 5);

	auto_span_check_box = new wxCheckBox(this, ID_AUTO_SPAN, L"Auto");
	horizontal_sizers[6]->Add(auto_span_check_box, 1, wxEXPAND | wxALL, 5);
	auto_span_check_box->SetValue(true);

	//////////////////////////////////////////////////

	Bind(wxEVT_SPINCTRLDOUBLE, &CalendarSetupPanel::SlotWeightCtrls, this, ID_WEIGHT0);
	Bind(wxEVT_SPINCTRLDOUBLE, &CalendarSetupPanel::SlotWeightCtrls, this, ID_WEIGHT1);
	Bind(wxEVT_SPINCTRLDOUBLE, &CalendarSetupPanel::SlotWeightCtrls, this, ID_WEIGHT2);
	Bind(wxEVT_SPINCTRLDOUBLE, &CalendarSetupPanel::SlotWeightCtrls, this, ID_GAP_FACTOR);
	Bind(wxEVT_SPINCTRL, &CalendarSetupPanel::SlotSpanSpinCtrls, this, ID_SPAN_FROM);
	Bind(wxEVT_SPINCTRL, &CalendarSetupPanel::SlotSpanSpinCtrls, this, ID_SPAN_TO); 
	Bind(wxEVT_CHECKBOX, &CalendarSetupPanel::SlotAutoSpanCheckBox, this, ID_AUTO_SPAN);

	//////////////////////////////////////////////////

	UpdateControls();
}

void CalendarSetupPanel::SendDefaultValues()
{
	calendar_span_from_spin_ctrl->Enable(!calendar_config.auto_calendar_range);
	calendar_span_to_spin_ctrl->Enable(!calendar_config.auto_calendar_range);

	calendar_config_signal(calendar_config);
}

void CalendarSetupPanel::SaveToXML(pugi::xml_node* node)
{
	auto child_node = node->append_child(L"calendar_setup");

	auto sub_row_weights_node = child_node.append_child(L"sub_row_weights");
	for (const auto& sub_row_weight : calendar_config.sub_row_weights)
	{
		auto weight_node = sub_row_weights_node.append_child(L"sub_row_weight");
		weight_node.append_attribute(L"weight").set_value(sub_row_weight);
	}

	child_node.append_child(L"gap_factor").append_attribute(L"gap_factor").set_value(calendar_config.gap_factor);
	child_node.append_child(L"min_calendar_range").append_attribute(L"min_calendar_range").set_value(calendar_config.min_calendar_range);
	child_node.append_child(L"max_calendar_range").append_attribute(L"max_calendar_range").set_value(calendar_config.max_calendar_range);
	child_node.append_child(L"auto_calendar_range").append_attribute(L"auto_calendar_range").set_value(calendar_config.auto_calendar_range);
}

void CalendarSetupPanel::LoadFromXML(const pugi::xml_node& node)
{
	auto child_node = node.child(L"calendar_setup");

	calendar_config.sub_row_weights.clear();
	auto sub_row_weights_node = child_node.child(L"sub_row_weights");
	for (auto& sub_row_weight : sub_row_weights_node.children(L"sub_row_weight"))
	{
		calendar_config.sub_row_weights.push_back(sub_row_weight.attribute(L"weight").as_float());
	}

	calendar_config.gap_factor = child_node.child(L"gap_factor").attribute(L"gap_factor").as_float();
	calendar_config.min_calendar_range = child_node.child(L"min_calendar_range").attribute(L"min_calendar_range").as_int();
	calendar_config.max_calendar_range = child_node.child(L"max_calendar_range").attribute(L"max_calendar_range").as_int();
	calendar_config.auto_calendar_range = child_node.child(L"auto_calendar_range").attribute(L"auto_calendar_range").as_bool();

	calendar_config_signal(calendar_config);
	UpdateControls();
}

void CalendarSetupPanel::SlotWeightCtrls(wxSpinDoubleEvent& event)
{
	if (event.GetId() == ID_WEIGHT0)
	{
		calendar_config.sub_row_weights[0] = event.GetValue();
	}
	if (event.GetId() == ID_WEIGHT1)
	{
		calendar_config.sub_row_weights[1] = event.GetValue();
	}
	if (event.GetId() == ID_WEIGHT2)
	{
		calendar_config.sub_row_weights[2] = event.GetValue();
	}
	if (event.GetId() == ID_GAP_FACTOR)
	{
		calendar_config.gap_factor = event.GetValue();
	}

	calendar_config_signal(calendar_config);
}

void CalendarSetupPanel::SlotSpanSpinCtrls(wxSpinEvent& event)
{
	if (ID_SPAN_FROM == event.GetId())
	{
		calendar_config.min_calendar_range = event.GetValue();
	}
	if (ID_SPAN_TO == event.GetId())
	{
		calendar_config.max_calendar_range = event.GetValue();
	}

	calendar_config_signal(calendar_config);
}

void CalendarSetupPanel::SlotAutoSpanCheckBox(wxCommandEvent& event)
{
	if (ID_AUTO_SPAN == event.GetId())
	{
		calendar_config.auto_calendar_range = event.IsChecked();

		calendar_span_from_spin_ctrl->Enable(!calendar_config.auto_calendar_range);
		calendar_span_to_spin_ctrl->Enable(!calendar_config.auto_calendar_range);
	}

	calendar_config_signal(calendar_config);
}

void CalendarSetupPanel::UpdateControls()
{
	weight0_spin_ctrl->SetValue(calendar_config.sub_row_weights[0]);
	weight1_spin_ctrl->SetValue(calendar_config.sub_row_weights[1]);
	weight2_spin_ctrl->SetValue(calendar_config.sub_row_weights[2]);
	gap_factor_spin_ctrl->SetValue(calendar_config.gap_factor);
	calendar_span_from_spin_ctrl->SetValue(calendar_config.min_calendar_range);
	calendar_span_to_spin_ctrl->SetValue(calendar_config.max_calendar_range);
	auto_span_check_box->SetValue(calendar_config.auto_calendar_range);

	calendar_span_from_spin_ctrl->Enable(!calendar_config.auto_calendar_range);
	calendar_span_to_spin_ctrl->Enable(!calendar_config.auto_calendar_range);
}
