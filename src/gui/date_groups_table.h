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

#include "../date_utils.h"

#ifdef WX_PRECOMP
#include <wx/wxprec.h>
#else 
#include <wx/wx.h>
#endif

#include <wx/dataview.h>
#include <wx/filedlg.h>
#include <wx/datetime.h>
#include <wx/intl.h>
#include <wx/toolbar.h>

#include <sigslot/signal.hpp>

#include <vector>
#include <string>


class DateGroupsTablePanel : public wxPanel
{
public:

	DateGroupsTablePanel(wxWindow* parent);

	std::wstring GetPanelName();

	//sigslot::signal<const std::vector<date_period>&> signal_table_date_intervals;

private:
	void UpdateButtons();
	void InsertRow(size_t row);
	void RemoveRow(size_t row);

	void OnItemActivated(wxDataViewEvent& event);
	void OnItemEditing(wxDataViewEvent& event);
	void OnSelectionChanged(wxDataViewEvent& event);
	void OnButtonClicked(wxCommandEvent& event);

	wxDataViewListCtrl* data_table;
	wxButton* addRowButton;
	wxButton* deleteRowButton;
};

