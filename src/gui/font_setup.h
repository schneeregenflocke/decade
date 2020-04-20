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

#include "../graphics/font.h"

#ifdef WX_PRECOMP
#include <wx/wxprec.h>
#else 
#include <wx/wx.h>
#endif

#include <wx/fontpicker.h>
#include <wx/font.h>

#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <filesystem>

#include <boost/signals2.hpp>


class FontSetupPanel : public wxPanel
{
public:

	FontSetupPanel(wxWindow* parent);
	//~FontSetupPanel();

	std::string GetFontFilePath();
	void SetFontFilePath(std::string file_path);

	void SendDefaultValues();

	template<typename T, typename U>
	void ConnectSignalFontPath(T memfunptr, U objectptr);

private:

	void SlotSelectFont(wxFontPickerEvent& event);
	std::string PrepareFontName(const wxFont& wx_font);
	void EnumerateFontDirectory();

	bool font_directory_enumerated;

	std::string default_font_path;
	std::string font_file_path;
	wxFontPickerCtrl* font_picker;
	EnumerateFont enum_fonts;
	boost::signals2::signal<void(const std::string&)> signal_fontpath;
	std::thread enumerate_thread;
};

template<typename T, typename U>
inline void FontSetupPanel::ConnectSignalFontPath(T memfunptr, U objectptr)
{
	signal_fontpath.connect(std::bind(memfunptr, objectptr, std::placeholders::_1));
}
