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

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>

#include <wx/print.h>
#include <wx/printdlg.h>
#include <wx/cmndata.h>
#include <wx/paper.h>

#include <wx/choice.h>
#include <wx/spinctrl.h>

#include <wx/statbox.h>
#include <wx/sizer.h>

#include <memory>
#include <array>

#include <sigslot/signal.hpp>

#include <pugixml.hpp>




class PageSetupPanel : private wxPanel
{
public:
	PageSetupPanel(wxWindow* parent);

	friend class MainWindow;

	void SendDefaultValues();

	void SaveXML(pugi::xml_node* doc);
	void LoadXML(const pugi::xml_node& doc);

	//std::array<float, 2> GetPageSize();
	//std::array<float, 4> GetPageMargins();
	//void SetPageSize(const std::array<float, 2>& value);
	//void SetPageMargins(const std::array<float, 4>& value);

private:

	std::array<float, 2> page_size;
	std::array<float, 4> page_margins;

	void ScanPageSetupDialogData(const wxPageSetupDialogData& dialog_data);
	void ScanData();

	void OnButtonClicked(wxCommandEvent& event);
	void OnCheckboxClicked(wxCommandEvent& event);
	void OnSpinControl(wxSpinDoubleEvent& event);

	const int ID_PAGE_SETUP;
	const int ID_CUSTOM_SIZE;
	const int ID_PAGE_WIDTH;
	const int ID_PAGE_HEIGHT;

	std::unique_ptr<wxPageSetupDialogData> page_setup_dialog_data;

	wxSpinCtrlDouble* page_width_spinctrl;
	wxSpinCtrlDouble* page_height_spinctrl;

	sigslot::signal<const std::array<float, 2>&> signal_page_size;
	sigslot::signal<const std::array<float, 4>&> signal_page_margins;
};
