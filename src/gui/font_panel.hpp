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


#include "../graphics/font.hpp"
#include "wx_widgets_include.hpp"

#include <sigslot/signal.hpp>

#include <vector>
#include <exception>


class FontPanel
{
public:

	FontPanel(wxWindow* parent)
	{
		wx_panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxPanelNameStr);
		wx_font_picker = new wxFontPickerCtrl(wx_panel, wxID_ANY, wxNullFont, wxDefaultPosition, wxDefaultSize, wxFNTP_FONTDESC_AS_LABEL);

		wxBoxSizer* horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);
		horizontal_sizer->Add(wx_font_picker, 1, wxALL | wxEXPAND, 5);
		wxBoxSizer* vertical_sizer = new wxBoxSizer(wxVERTICAL);
		vertical_sizer->Add(horizontal_sizer, 0, wxEXPAND);
		wx_panel->SetSizer(vertical_sizer);

		wx_panel->Bind(wxEVT_FONTPICKER_CHANGED, &FontPanel::CallbackFontChanged, this);
		wx_font = wx_font_picker->GetFont();
		ProcessFontData();
	}

	std::vector<unsigned char>& GetFontData()
	{
		return font_data;
	}

	wxPanel* GetPanelPtr()
	{
		return wx_panel;
	}
	
	sigslot::signal<const std::vector<unsigned char>&> signal_font_file_path;

private:

	void ProcessFontData()
	{
#ifdef _WIN32
		WXHFONT wxhfont = nullptr;
		wxhfont = wx_font.GetHFONT();

		HDC hdc = nullptr;
		hdc = ::CreateCompatibleDC(nullptr);
		if (hdc != nullptr)
		{
			auto replaced = ::SelectObject(hdc, wxhfont);
			HGDI_ERROR;
			NULLREGION;
			const size_t size = ::GetFontData(hdc, 0, 0, nullptr, 0);
			if (size > 0)
			{
				font_data.resize(size);
				auto check = ::GetFontData(hdc, 0, 0, font_data.data(), size);
				if (check == GDI_ERROR || check != size)
				{
					throw std::runtime_error("GetFontData failed");
				}
			}
			bool delete_succeeded = ::DeleteDC(hdc);
		}
#endif
#ifdef __linux__
#endif
	}

	void CallbackFontChanged(wxFontPickerEvent& event)
	{
		try 
		{
			wx_font = event.GetFont();
			ProcessFontData();
			signal_font_file_path(font_data);
			font_data.clear();
		}
		catch (...)
		{
			std::cout << "Loading Font failed" << '\n';
		}
	}

	wxPanel* wx_panel;
	wxFontPickerCtrl* wx_font_picker;
	wxFont wx_font;
	std::vector<unsigned char> font_data;
};
