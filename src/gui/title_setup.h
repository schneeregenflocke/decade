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

#ifdef WX_PRECOMP
#include <wx/wxprec.h>
#else 
#include <wx/wx.h>
#endif

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/clrpicker.h>
#include <wx/colour.h>
#include <wx/slider.h>

#include <array>
#include <algorithm>
#include <memory>
#include <functional>
#include <string>
#include <limits>

#include <sigslot/signal.hpp>

#include <pugixml.hpp>



class TitleSetupPanel : public wxPanel
{
public:

	TitleSetupPanel(wxWindow* parent);

	//float GetFrameHeight();
	//float GetFontSizeRatio();
	//std::wstring GetTitleText();
	//void SetFrameHeight(float value);
	//void SetFontSizeRatio(float value);
	//void SetTitleText(const std::wstring& value);

	void SaveToXML(pugi::xml_node* node);
	void LoadFromXML(const pugi::xml_node& node);

	void SendDefaultValues();

	sigslot::signal<float> signal_frame_height;
	sigslot::signal<float> signal_font_size_ratio;
	sigslot::signal<const std::wstring&> signal_title_text;
	sigslot::signal<const std::array<float, 4>&> signal_text_color;

private:

	float frame_height;
	float font_size_ratio;
	std::wstring title_text;
	std::array<float, 4> text_color;

	void UpdateWidgets();

	wxSpinCtrlDouble* frame_height_ctrl;
	wxSpinCtrlDouble* size_ratio_ctrl;
	wxTextCtrl* title_text_edit;

	void SlotSpinControl(wxSpinDoubleEvent& event);
	void SlotTextControl(wxCommandEvent& event);
	void SlotColorPickerControl(wxColourPickerEvent& event);
	void SlotSliderControl(wxCommandEvent& event);

	const int ID_FRAME_HEIGHT;
	const int ID_SIZE_RATIO;
	const int ID_TEXT;
	const int ID_COLOR_PICKER;
	const int ID_SLIDER;

	float To_RGBA_float(unsigned char value);
};