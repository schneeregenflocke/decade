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


#include "font_setup.h"

FontSetupPanel::FontSetupPanel(wxWindow* parent) : 
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxPanelNameStr),
	font_directory_enumerated(false)
{
	font_picker = new wxFontPickerCtrl(this, wxID_ANY, wxNullFont, wxDefaultPosition, wxDefaultSize, wxFNTP_DEFAULT_STYLE);
	font_picker->Enable(true);
	//font_picker->SetMinSize(wxSize(300, -1));

	wxBoxSizer* horizontal_sizer0 = new wxBoxSizer(wxHORIZONTAL);
	horizontal_sizer0->Add(font_picker, 1, wxALL | wxEXPAND, 5);

	wxBoxSizer* vertical_sizer = new wxBoxSizer(wxVERTICAL);
	vertical_sizer->Add(horizontal_sizer0, 0, wxEXPAND);

	SetSizer(vertical_sizer);

	Bind(wxEVT_FONTPICKER_CHANGED, &FontSetupPanel::SlotSelectFont, this);

	
}

std::string FontSetupPanel::GetFontFilePath()
{
	return font_file_path;
}

void FontSetupPanel::SetFontFilePath(std::string file_path)
{
	if (std::filesystem::exists(file_path))
	{
		font_file_path = file_path;
		signal_fontpath(file_path);
	}
	else
	{
		std::string w32_default_font_path = "C:/Windows/Fonts/arial.ttf";
		font_file_path = w32_default_font_path;
	}

	signal_fontpath(font_file_path);
}

/*FontSetupPanel::~FontSetupPanel()
{
	//enumerate_thread.join();
}*/

void FontSetupPanel::SendDefaultValues()
{
	std::string w32_default_font_path = "C:/Windows/Fonts/arial.ttf";
	font_file_path = w32_default_font_path;
	signal_fontpath(w32_default_font_path);
}

void FontSetupPanel::SlotSelectFont(wxFontPickerEvent& event)
{
	if (!font_directory_enumerated)
	{
		font_picker->Enable(false);
		enumerate_thread = std::thread(&FontSetupPanel::EnumerateFontDirectory, this);
		//enumerate_thread.detach();
		enumerate_thread.join();
		font_picker->Enable(true);
		font_directory_enumerated = true;
	}
	
	auto fontlookup = PrepareFontName(event.GetFont());

	auto filepath = enum_fonts.GetFilepath(fontlookup);
	if (filepath != std::string("NotFound") && std::filesystem::exists(filepath))
	{
		font_file_path = filepath;
		signal_fontpath(filepath);
	}
	else
	{
		std::cout << "font file not found" << '\n';
	}
}

std::string FontSetupPanel::PrepareFontName(const wxFont& wx_font)
{
	std::string fontlookup = wx_font.GetFaceName();
	if (wx_font.GetWeight() == wxFONTWEIGHT_BOLD)
	{
		fontlookup += std::string(" Bold");
	}
	if (wx_font.GetStyle() == wxFONTSTYLE_ITALIC)
	{
		fontlookup += std::string(" Italic");
	}
	return fontlookup;
}

void FontSetupPanel::EnumerateFontDirectory()
{
	enum_fonts.Enumerate();
}
