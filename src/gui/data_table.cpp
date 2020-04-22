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


#include "data_table.h"


DataTablePanel::DataTablePanel(wxWindow* parent) : 
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxPanelNameStr)//,
	//datesStoreChangedEventTag(wxNewEventType())
{

	data_view_list_ctrl = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_SINGLE, wxDefaultValidator);
	 
	Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &DataTablePanel::OnItemActivated, this);
	Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE, &DataTablePanel::OnItemEditing, this);
	Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &DataTablePanel::OnSelectionChanged, this);

	// Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, &DataTablePanel::OnItemEditing, this);
	// Bind(wxEVT_DATAVIEW_ITEM_EDITING_STARTED, &DataTablePanel::OnItemEditing, this);
	// Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &DataTablePanel::OnValueChanged, this);

	addRowButton = new wxButton(this, wxID_ADD, "Add Row");
	deleteRowButton = new wxButton(this, wxID_DELETE, "Delete Row");
	deleteRowButton->Disable();

	Bind(wxEVT_BUTTON, &DataTablePanel::OnButtonClicked, this);
	

	wxBoxSizer* subSizer = new wxBoxSizer(wxHORIZONTAL);
	subSizer->Add(addRowButton, 0, wxALL, 5);
	subSizer->Add(deleteRowButton, 0, wxALL, 5);


	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(data_view_list_ctrl, 1, wxEXPAND | wxALL, 5);
	mainSizer->Add(subSizer, 0, wxEXPAND);
	SetSizer(mainSizer);

	data_view_list_ctrl->AppendTextColumn(L"Number", wxDATAVIEW_CELL_INERT);
	data_view_list_ctrl->AppendTextColumn(L"From Date", wxDATAVIEW_CELL_EDITABLE);
	data_view_list_ctrl->AppendTextColumn(L"To Date", wxDATAVIEW_CELL_EDITABLE);
	data_view_list_ctrl->AppendTextColumn(L"Duration", wxDATAVIEW_CELL_INERT);
	data_view_list_ctrl->AppendTextColumn(L"Duration to next", wxDATAVIEW_CELL_INERT);

	/*
	wxArrayString test_string;
	test_string.Add(L"choice A");
	test_string.Add(L"choice B");
	wxDataViewChoiceRenderer* choice_renderer = new wxDataViewChoiceRenderer(test_string);
	wxDataViewColumn* test_column = new wxDataViewColumn(L"test column", choice_renderer, 5);
	data_view_list_ctrl->AppendColumn(test_column);
	*/
	

	dateFormat = InitDateFormat();
}



void DataTablePanel::OnItemActivated(wxDataViewEvent& event)
{
	if (event.GetItem().IsOk() && event.GetDataViewColumn())
	{
		data_view_list_ctrl->EditItem(event.GetItem(), event.GetDataViewColumn());
	}
}

void DataTablePanel::OnItemEditing(wxDataViewEvent& event)
{
	if (event.GetEventType() == wxEVT_DATAVIEW_ITEM_EDITING_DONE)
	{
		event.Veto();

		if (event.IsEditCancelled() == false)
		{
			auto edited_string = event.GetValue().GetString().ToStdString();
			
			// Check Date
			auto edited_date = string_to_boost_date(edited_string, dateFormat);
			if (edited_date.is_special() == false)
			{
				std::string parsed_string = boost_date_to_string(edited_date);
				data_view_list_ctrl->SetValue(parsed_string.c_str(), data_view_list_ctrl->GetSelectedRow(), event.GetColumn());	
			}
			else
			{
				data_view_list_ctrl->SetValue(edited_string.c_str(), data_view_list_ctrl->GetSelectedRow(), event.GetColumn());
			}

			ScanTable();
		}		
	}
}

void DataTablePanel::OnSelectionChanged(wxDataViewEvent& event)
{
	//std::cout << "selection changed row " << data_view_list_ctrl->GetSelectedRow() << " column " << event.GetColumn() << '\n';
	UpdateButtons();
}

void DataTablePanel::OnButtonClicked(wxCommandEvent& event)
{
	auto selected_row = data_view_list_ctrl->GetSelectedRow();

	if (event.GetId() == wxID_ADD)
	{
		if (selected_row == wxNOT_FOUND)
		{
			selected_row = data_view_list_ctrl->GetItemCount();
		}
		else 
		{
			++selected_row;
		}
		
		InsertRow(selected_row);
		data_view_list_ctrl->SelectRow(selected_row);
		data_view_list_ctrl->EnsureVisible(data_view_list_ctrl->RowToItem(selected_row));
		UpdateButtons();
	}

	if (event.GetId() == wxID_DELETE && selected_row != wxNOT_FOUND)
	{
		RemoveRow(selected_row);

		if (data_view_list_ctrl->GetItemCount() > 0)
		{
			if (data_view_list_ctrl->GetItemCount() == selected_row)
			{
				data_view_list_ctrl->SelectRow(selected_row - 1);
			}
			else
			{
				data_view_list_ctrl->SelectRow(selected_row);
			}		
		}

		UpdateButtons();
	}
}

/*void DataTablePanel::OnValueChanged(wxDataViewEvent& event)
{}*/




/*const wxEventTypeTag<wxCommandEvent> DataTablePanel::GetDatesStoreChangedEventTag() const
{
	return datesStoreChangedEventTag;
}*/


void DataTablePanel::UpdateButtons()
{
	if (data_view_list_ctrl->GetSelectedRow() == wxNOT_FOUND)
	{
		deleteRowButton->Disable();
	}
	else
	{
		deleteRowButton->Enable();
	}
}

void DataTablePanel::InsertRow(size_t row)
{
	if (row <= data_view_list_ctrl->GetItemCount())
	{
		wxVector<wxVariant> empty_row;
		empty_row.resize(data_view_list_ctrl->GetColumnCount());
		data_view_list_ctrl->InsertItem(row, empty_row);
	}
}

void DataTablePanel::RemoveRow(size_t row)
{
	if (row <= data_view_list_ctrl->GetItemCount())
	{
		data_view_list_ctrl->DeleteItem(row);
		ScanTable();
	}
	//ScanStore();
}


/*void DataTablePanel::InitializeTable()
{
	data_view_list_ctrl->DeleteAllItems();

	for (int index = 0; index < data_store->GetDateIntervalsSize(); ++index)
	{
		InsertRow(index);

		data_view_list_ctrl->SetValue(boost_date_to_string(data_store->GetDateIntervalConstRef(index).begin()), index, 1);
		data_view_list_ctrl->SetValue(boost_date_to_string(data_store->GetDateIntervalConstRef(index).end()), index, 2);

		data_view_list_ctrl->SetValue(std::to_string(index + 1), index, 0);

		data_view_list_ctrl->SetValue(std::to_string(data_store->GetDateIntervalConstRef(index).length().days()), index, 3);

		if (index < (data_store->GetDateIntervalsSize() - 1))
		{
			data_view_list_ctrl->SetValue(std::to_string(data_store->GetDateInterIntervalConstRef(index).length().days()), index, 4);
		}
	}

	wxCommandEvent datesStoreChangedEvent(datesStoreChangedEventTag);
	ProcessWindowEvent(datesStoreChangedEvent);
}*/


void DataTablePanel::SlotUpdateTable(const std::vector<date_period>& date_intervals, const std::vector<date_period>& date_inter_intervals)
{
	UpdateValidRows();
	auto number_insert_rows = date_intervals.size() - valid_rows.size();

	for (size_t index = 0; index < valid_rows.size(); ++index)
	{
		data_view_list_ctrl->SetValue(std::to_string(index + 1), valid_rows[index], 0);
		data_view_list_ctrl->SetValue(boost_date_to_string(date_intervals[index].begin()), valid_rows[index], 1);
		data_view_list_ctrl->SetValue(boost_date_to_string(date_intervals[index].end()), valid_rows[index], 2);
		data_view_list_ctrl->SetValue(std::to_string(date_intervals[index].length().days()), valid_rows[index], 3);

		if (index < (date_intervals.size() - 1))
		{
			data_view_list_ctrl->SetValue(std::to_string(date_inter_intervals[index].length().days()), valid_rows[index], 4);
		}
	}

	for (size_t index = valid_rows.size(); index < date_intervals.size(); ++index)
	{
		InsertRow(index);

		data_view_list_ctrl->SetValue(std::to_string(index + 1), index, 0);
		data_view_list_ctrl->SetValue(boost_date_to_string(date_intervals[index].begin()), index, 1);
		data_view_list_ctrl->SetValue(boost_date_to_string(date_intervals[index].end()), index, 2);
		data_view_list_ctrl->SetValue(std::to_string(date_intervals[index].length().days()), index, 3);

		if (index < (date_intervals.size() - 1))
		{
			data_view_list_ctrl->SetValue(std::to_string(date_inter_intervals[index].length().days()), index, 4);
		}
	}
}


/*void DataTablePanel::ScanStore()
{
	for (size_t index = 0; index < valid_rows.size(); ++index)
	{
		data_view_list_ctrl->SetValue(boost_date_to_string(data_store->GetDateIntervalConstRef(index).begin()), valid_rows[index], 1);
		data_view_list_ctrl->SetValue(boost_date_to_string(data_store->GetDateIntervalConstRef(index).end()), valid_rows[index], 2);

		data_view_list_ctrl->SetValue(std::to_string(index + 1), valid_rows[index], 0);

		data_view_list_ctrl->SetValue(std::to_string(data_store->GetDateIntervalConstRef(index).length().days()), valid_rows[index], 3);

		if (index < (data_store->GetDateIntervalsSize() - 1))
		{
			data_view_list_ctrl->SetValue(std::to_string(data_store->GetDateInterIntervalConstRef(index).length().days()), valid_rows[index], 4);
		}
	}
}*/


void DataTablePanel::UpdateValidRows()
{
	valid_rows.clear();
	for (size_t row = 0; row < data_view_list_ctrl->GetItemCount(); ++row)
	{
		auto begin_date = ParseDateByCell(row, 1);
		auto end_date = ParseDateByCell(row, 2);

		if (CheckDateInterval(begin_date, end_date))
		{
			valid_rows.push_back(row);
		}
	}
}


void DataTablePanel::ScanTable()
{
	UpdateValidRows();

	std::vector<date_period> date_intervals;

	for (size_t index = 0; index < valid_rows.size(); ++index)
	{
		auto begin_date = ParseDateByCell(valid_rows[index], 1);
		auto end_date = ParseDateByCell(valid_rows[index], 2);
		
		date_intervals.push_back(date_period(begin_date, end_date));	
	}

	signal_table_date_intervals(date_intervals);

	//data_store->SetDateIntervals(local_date_intervals);
	//wxCommandEvent datesStoreChangedEvent(datesStoreChangedEventTag);
	//ProcessWindowEvent(datesStoreChangedEvent);
}

bool DataTablePanel::CheckDateInterval(date begin_date, date end_date)
{
	bool valid = false;

	if (begin_date.is_special() == false && end_date.is_special() == false)
	{
		date_period period = date_period(begin_date, end_date);

		if (period.is_null() == false)
		{
			valid = true;
		}
	}

	return valid;
}

date DataTablePanel::ParseDateByCell(int row, int column)
{
	wxVariant cell_value;
	data_view_list_ctrl->GetStore()->GetValueByRow(cell_value, row, column);
	return string_to_boost_date(cell_value.GetString().ToStdString(), dateFormat);
}
