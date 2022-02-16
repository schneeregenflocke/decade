/*
Decade
Copyright (c) 2019-2022 Marco Peyer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/


#pragma once


#include "wx_widgets_include.h"

#include "../packages/title_config.h"

#include <array>
#include <algorithm>
#include <memory>
#include <functional>
#include <string>
#include <limits>

#include <sigslot/signal.hpp>

#include <glm/glm.hpp>


class TitleSetupPanel : public wxPanel
{
public:

	TitleSetupPanel(wxWindow* parent) :
		wxPanel(parent, wxID_ANY)
	{
		wxSizerFlags sizer_flags_0 = wxSizerFlags().Proportion(0).Expand();
		wxSizerFlags sizer_flags_1 = wxSizerFlags().Proportion(0).Expand().Border(wxALL, 5);
		wxSizerFlags sizer_flags_2 = wxSizerFlags().Proportion(1).Expand().Border(wxALL, 5);

		wxBoxSizer* vertical_sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(vertical_sizer);

		std::array<wxBoxSizer*, 5> horizontal_sizers;

		for (auto& sizer : horizontal_sizers)
		{
			sizer = new wxBoxSizer(wxHORIZONTAL);
			vertical_sizer->Add(sizer, sizer_flags_0);
		}

		std::array<wxStaticText*, 5> labels;

		labels[0] = new wxStaticText(this, wxID_ANY, L"Frame Height");
		labels[0]->SetMinSize(wxSize(120, -1));
		horizontal_sizers[0]->Add(labels[0], sizer_flags_1);

		labels[1] = new wxStaticText(this, wxID_ANY, L"Font Size Ratio");
		labels[1]->SetMinSize(wxSize(120, -1));
		horizontal_sizers[1]->Add(labels[1], sizer_flags_1);

		labels[2] = new wxStaticText(this, wxID_ANY, L"Text");
		labels[2]->SetMinSize(wxSize(120, -1));
		horizontal_sizers[2]->Add(labels[2], sizer_flags_1);

		labels[3] = new wxStaticText(this, wxID_ANY, L"Text Color");
		labels[3]->SetMinSize(wxSize(120, -1));
		horizontal_sizers[3]->Add(labels[3], sizer_flags_1);
		labels[3]->Enable(false);

		labels[4] = new wxStaticText(this, wxID_ANY, L"Color Transparency");
		labels[4]->SetMinSize(wxSize(120, -1));
		horizontal_sizers[4]->Add(labels[4], sizer_flags_1);
		labels[4]->Enable(false);

		frame_height_ctrl = new wxSpinCtrlDouble(this);
		frame_height_ctrl->SetDigits(2);
		horizontal_sizers[0]->Add(frame_height_ctrl, sizer_flags_2);

		size_ratio_ctrl = new wxSpinCtrlDouble(this);
		size_ratio_ctrl->SetDigits(2);
		size_ratio_ctrl->SetIncrement(0.05);
		horizontal_sizers[1]->Add(size_ratio_ctrl, sizer_flags_2);

		title_text_edit = new wxTextCtrl(this, wxID_ANY);
		horizontal_sizers[2]->Add(title_text_edit, sizer_flags_2);

		text_color_picker = new wxColourPickerCtrl(this, wxID_ANY, *wxStockGDI::GetColour(wxStockGDI::COLOUR_BLACK),
			wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_ALPHA);
		text_color_picker->Enable(false);
		horizontal_sizers[3]->Add(text_color_picker, sizer_flags_2);

		alpha_slider = new wxSlider(this, wxID_ANY, 255, 0, 255, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
		alpha_slider->Enable(false);
		horizontal_sizers[4]->Add(alpha_slider, sizer_flags_2);

		vertical_sizer->Layout();

		////////////////////////////////////////

		Bind(wxEVT_SPINCTRLDOUBLE, &TitleSetupPanel::SlotSpinControl, this);
		Bind(wxEVT_TEXT, &TitleSetupPanel::SlotTextControl, this);
		Bind(wxEVT_COLOURPICKER_CHANGED, &TitleSetupPanel::SlotColorPickerControl, this);
		Bind(wxEVT_SLIDER, &TitleSetupPanel::SlotSliderControl, this);

		////////////////////////////////////////
	}

	void SendDefaultValues()
	{
		SendTitleConfig();
	}

	void SendTitleConfig()
	{
		signal_title_config(title_config);
	}

	void ReceiveTitleConfig(const TitleConfig& title_config)
	{
		this->title_config = title_config;
		UpdateWidgetForSelection();
	}

	sigslot::signal<const TitleConfig&> signal_title_config;

private:

	TitleConfig title_config;

	wxSpinCtrlDouble* frame_height_ctrl;
	wxSpinCtrlDouble* size_ratio_ctrl;
	
	wxTextCtrl* title_text_edit;

	wxColourPickerCtrl* text_color_picker;
	wxSlider* alpha_slider;

	void UpdateWidgetForSelection()
	{
		frame_height_ctrl->SetValue(static_cast<double>(title_config.frame_height));
		size_ratio_ctrl->SetValue(static_cast<double>(title_config.font_size_ratio));
		
		title_text_edit->ChangeValue(title_config.title_text);
		

		/*wxColour color;
		color.Set(
			glm::floatBitsToUint(title_config.text_color[0]), 
			glm::floatBitsToUint(title_config.text_color[1]), 
			glm::floatBitsToUint(title_config.text_color[2]), 
			glm::floatBitsToUint(title_config.text_color[3])
		);

		text_color_picker->SetColour(color);
		alpha_slider->SetValue(glm::floatBitsToUint(title_config.text_color[3]));*/
	}

	void SlotSpinControl(wxSpinDoubleEvent& event)
	{
		auto float_value = static_cast<float>(event.GetValue());

		if (frame_height_ctrl == event.GetEventObject())
		{
			title_config.frame_height = float_value;

			SendTitleConfig();
		}

		if (size_ratio_ctrl == event.GetEventObject())
		{
			title_config.font_size_ratio = float_value;

			SendTitleConfig();
		}
	}

	void SlotTextControl(wxCommandEvent& event)
	{
		if (title_text_edit == event.GetEventObject())
		{
			title_config.title_text = event.GetString();

			SendTitleConfig();
		}
	}

	void SlotColorPickerControl(wxColourPickerEvent& event)
	{
		//if (event.GetId() == ID_COLOR_PICKER)
		//{
			/*auto color = event.GetColour();

			title_config.text_color[0] = glm::uintBitsToFloat(color.Red());
			title_config.text_color[1] = glm::uintBitsToFloat(color.Green());
			title_config.text_color[2] = glm::uintBitsToFloat(color.Blue());
			title_config.text_color[3] = 1.0f;*/
			//title_config.text_color[3] = glm::uintBitsToFloat(color.Alpha());
		//}

		//SendTitleConfig();
	}

	void SlotSliderControl(wxCommandEvent& event)
	{
		//if (event.GetId() == ID_SLIDER)
		//{
			//title_config.text_color[3] = glm::uintBitsToFloat(event.GetInt());
		//}

		//SendTitleConfig();
	}
};