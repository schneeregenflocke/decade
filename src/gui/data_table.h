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


#include "../date_convert.h"
#include "../dates_storage.h"


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

//#define BOOST_DATE_TIME_NO_LIB
//#include <boost/date_time.hpp>

#include <array>
#include <sstream>
#include <locale>
#include <ctime>
#include <iomanip>
#include <cctype>
#include <algorithm>
#include <vector>
#include <utility>
#include <string>



class DataTablePanel : public wxPanel
{
public:

	DataTablePanel(wxWindow* parent, DateIntervals* data_store);
	void InitializeTable();
	const wxEventTypeTag<wxCommandEvent> GetDatesStoreChangedEventTag() const;

private:
	
	void OnItemActivated(wxDataViewEvent& event);
	void OnItemEditing(wxDataViewEvent& event);
	void OnSelectionChanged(wxDataViewEvent& event);
	void OnButtonClicked(wxCommandEvent& event);
	void OnValueChanged(wxDataViewEvent& event);
	
	void UpdateButton();

	void InsertRow(size_t row);
	void RemoveRow(size_t row);

	bool CheckDateInterval(date begin_date, date end_date);
	date ParseDateByCell(int row, int column);
	
	wxDataViewListCtrl* data_view_list_ctrl;
	
	date_format_descriptor dateFormat;

	wxButton* addRowButton;
	wxButton* deleteRowButton;

	DateIntervals* data_store;

	void ScanTable();
	void ScanStore();

	std::vector<size_t> valid_rows;

	const wxEventTypeTag<wxCommandEvent> datesStoreChangedEventTag;
};

