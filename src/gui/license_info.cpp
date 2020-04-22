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

#include "license_info.h"


LicenseInformationDialog::LicenseInformationDialog() :
	wxDialog(nullptr, wxID_ANY, L"Open Source Licenses Information", wxDefaultPosition, wxSize(800, 600)/*wxDefaultSize*/,
			wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX )
{
	wxBoxSizer* vertical_sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(vertical_sizer);

	wxBoxSizer* horizontal_sizer = new wxBoxSizer(wxHORIZONTAL);
	vertical_sizer->Add(horizontal_sizer, 1, wxEXPAND);

	license_select_list_box = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE | wxLB_NEEDED_SB);

	horizontal_sizer->Add(license_select_list_box, 0, wxEXPAND | wxALL, 10);

	text_view_ctrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

	horizontal_sizer->Add(text_view_ctrl, 1, wxEXPAND | wxALL, 10);

	Bind(wxEVT_LISTBOX, &LicenseInformationDialog::SlotSelectLicense, this);

	CollectLicenses();

	license_select_list_box->Select(0);
	SelectLicense(collected_licenses.begin()->first);
}

void LicenseInformationDialog::CollectLicenses()
{
	collected_licenses.clear();
	collected_licenses.reserve(10);

	collected_licenses.emplace_back("Decade", LOAD_RESOURCE(LICENSE_txt).toString());
	collected_licenses.emplace_back("Boost", LOAD_RESOURCE(external_provisional_license_collection_Boost_LICENSE_1_0_txt).toString());
	collected_licenses.emplace_back("glm", LOAD_RESOURCE(external_glm_copying_txt).toString());
	collected_licenses.emplace_back("glad", LOAD_RESOURCE(external_glad_LICENSE).toString());
	collected_licenses.emplace_back("wxWidgets", LOAD_RESOURCE(external_provisional_license_collection_wxWidgets_licence_txt).toString());
	collected_licenses.emplace_back("embed_resource", LOAD_RESOURCE(external_provisional_license_collection_embed_resource_LICENSE_txt).toString());
	collected_licenses.emplace_back("csv", LOAD_RESOURCE(external_csv_LICENSE).toString());
	collected_licenses.emplace_back("pugixml", LOAD_RESOURCE(external_pugixml_LICENSE_md).toString());
	collected_licenses.emplace_back("lodepng", LOAD_RESOURCE(external_lodepng_LICENSE).toString());
	collected_licenses.emplace_back("freetype2", LOAD_RESOURCE(external_freetype2_docs_LICENSE_TXT).toString());
	collected_licenses.emplace_back("sigslot", LOAD_RESOURCE(external_sigslot_LICENSE).toString());
	
	for (const auto& license : collected_licenses)
	{
		license_select_list_box->AppendString(license.first);
	}
}

void LicenseInformationDialog::SlotSelectLicense(wxCommandEvent& event)
{
	SelectLicense(event.GetString().ToStdString());
}

void LicenseInformationDialog::SelectLicense(const std::string& map_key)
{
	auto iter = std::find_if(collected_licenses.cbegin(), collected_licenses.cend(), [&](const string_pair& compare) { return compare.first == map_key; });

	text_view_ctrl->Clear();
	*text_view_ctrl << iter->second;
	text_view_ctrl->ShowPosition(0);
}
