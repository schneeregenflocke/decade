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


#include "Resource.h"

#include "wx_widgets_include.h"

#include <string>
#include <vector>
#include <utility>
#include <algorithm>


class LicenseInformationDialog : public wxDialog
{
public:

	LicenseInformationDialog() :
		wxDialog(nullptr, wxID_ANY, L"Open Source Licenses Information", wxDefaultPosition, wxSize(800, 600)/*wxDefaultSize*/,
			wxCAPTION | wxRESIZE_BORDER | wxMAXIMIZE_BOX)
	{

		license_select_list_box = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE | wxLB_NEEDED_SB);

		text_view_ctrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

		wxSizerFlags flags0 = wxSizerFlags().Proportion(1).Expand();
		wxSizerFlags flags1 = wxSizerFlags().Proportion(1).Expand().Border(wxALL, 10);
		wxSizerFlags flags2 = wxSizerFlags().Proportion(0).Expand().Border(wxALL, 10);
		wxSizerFlags flags3 = wxSizerFlags().Proportion(0).Border(wxALL, 10).Right();

		wxBoxSizer* vertical_sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(vertical_sizer);

		wxBoxSizer* horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);
		vertical_sizer->Add(horizontal_sizer, flags0);

		horizontal_sizer->Add(license_select_list_box, flags2);
		horizontal_sizer->Add(text_view_ctrl, flags1);

		wxButton* close_button = new wxButton(this, wxID_CLOSE);

		wxStdDialogButtonSizer* button_sizer = new wxStdDialogButtonSizer();
		button_sizer->AddButton(close_button);
		button_sizer->Realize();

		vertical_sizer->Add(button_sizer, flags3);

		Bind(wxEVT_LISTBOX, &LicenseInformationDialog::SlotSelectLicense, this);
		Bind(wxEVT_BUTTON, &LicenseInformationDialog::CloseDialog, this);

		CollectLicenses();

		license_select_list_box->Select(0);
		SelectLicense(collected_licenses.begin()->first);
	}

private:

	void CollectLicenses()
	{
		collected_licenses.clear();

		collected_licenses.emplace_back("Decade", LOAD_RESOURCE(decade_LICENSE).toString());
		collected_licenses.emplace_back("glm", LOAD_RESOURCE(glm_copying).toString());
		collected_licenses.emplace_back("glad", LOAD_RESOURCE(glad_LICENSE).toString());
		collected_licenses.emplace_back("embed-resource", LOAD_RESOURCE(embed_resource_LICENSE).toString());
		collected_licenses.emplace_back("csv2", LOAD_RESOURCE(csv2_LICENSE).toString());
		collected_licenses.emplace_back("csv2mio", LOAD_RESOURCE(csv2mio_LICENSE).toString());
		collected_licenses.emplace_back("lodepng", LOAD_RESOURCE(lodepng_LICENSE).toString());
		collected_licenses.emplace_back("sigslot", LOAD_RESOURCE(sigslot_LICENSE).toString());
		collected_licenses.emplace_back("boost", LOAD_RESOURCE(boost_copyright).toString());
		collected_licenses.emplace_back("freetype", LOAD_RESOURCE(freetype_copyright).toString());
		collected_licenses.emplace_back("zlib", LOAD_RESOURCE(zlib_copyright).toString());
		collected_licenses.emplace_back("libpng", LOAD_RESOURCE(libpng_copyright).toString());
		collected_licenses.emplace_back("wxwidgets", LOAD_RESOURCE(wxwidgets_licence).toString());

		for (const auto& license : collected_licenses)
		{
			license_select_list_box->AppendString(license.first);
		}
	}

	void SlotSelectLicense(wxCommandEvent& event)
	{
		SelectLicense(event.GetString().ToStdString());
	}
	void CloseDialog(wxCommandEvent& event)
	{
		EndModal(0);
	}
	void SelectLicense(const std::string& map_key)
	{
		auto iter = std::find_if(collected_licenses.cbegin(), collected_licenses.cend(), [&](const string_pair& compare) { return compare.first == map_key; });

		text_view_ctrl->Clear();
		*text_view_ctrl << iter->second;
		text_view_ctrl->ShowPosition(0);
	}

	wxListBox* license_select_list_box;
	wxTextCtrl* text_view_ctrl;

	typedef std::pair<std::string, std::string> string_pair;

	std::vector< std::pair<std::string, std::string> > collected_licenses;
};