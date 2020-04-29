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
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxPanelNameStr)
{

	data_table = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_SINGLE, wxDefaultValidator);
	 
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
	mainSizer->Add(data_table, 1, wxEXPAND | wxALL, 5);
	mainSizer->Add(subSizer, 0, wxEXPAND);
	SetSizer(mainSizer);

	data_table->AppendTextColumn(L"Number", wxDATAVIEW_CELL_INERT);
	data_table->AppendTextColumn(L"From Date", wxDATAVIEW_CELL_EDITABLE);
	data_table->AppendTextColumn(L"To Date", wxDATAVIEW_CELL_EDITABLE);
	data_table->AppendTextColumn(L"Duration", wxDATAVIEW_CELL_INERT);
	data_table->AppendTextColumn(L"Duration to next", wxDATAVIEW_CELL_INERT);

	/*
	wxArrayString test_string;
	test_string.Add(L"choice A");
	test_string.Add(L"choice B");
	wxDataViewChoiceRenderer* choice_renderer = new wxDataViewChoiceRenderer(test_string);
	wxDataViewColumn* test_column = new wxDataViewColumn(L"test column", choice_renderer, 5);
	data_table->AppendColumn(test_column);
	*/
	
	dateFormat = InitDateFormat();
}

void DataTablePanel::OnItemActivated(wxDataViewEvent& event)
{
	if (event.GetItem().IsOk() && event.GetDataViewColumn())
	{
		data_table->EditItem(event.GetItem(), event.GetDataViewColumn());
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
				data_table->SetValue(parsed_string.c_str(), data_table->GetSelectedRow(), event.GetColumn());	
			}
			else
			{
				data_table->SetValue(edited_string.c_str(), data_table->GetSelectedRow(), event.GetColumn());
			}

			ScanTable();
		}		
	}
}

void DataTablePanel::OnSelectionChanged(wxDataViewEvent& event)
{
	UpdateButtons();
}

void DataTablePanel::OnButtonClicked(wxCommandEvent& event)
{
	auto selected_row = data_table->GetSelectedRow();

	if (event.GetId() == wxID_ADD)
	{
		if (selected_row == wxNOT_FOUND)
		{
			selected_row = data_table->GetItemCount();
		}
		else 
		{
			++selected_row;
		}
		
		InsertRow(selected_row);
		data_table->SelectRow(selected_row);
		data_table->EnsureVisible(data_table->RowToItem(selected_row));
		UpdateButtons();
	}

	if (event.GetId() == wxID_DELETE && selected_row != wxNOT_FOUND)
	{
		RemoveRow(selected_row);

		if (data_table->GetItemCount() > 0)
		{
			if (data_table->GetItemCount() == selected_row)
			{
				data_table->SelectRow(selected_row - 1);
			}
			else
			{
				data_table->SelectRow(selected_row);
			}		
		}

		UpdateButtons();
	}
}

void DataTablePanel::UpdateButtons()
{
	if (data_table->GetSelectedRow() == wxNOT_FOUND)
	{
		deleteRowButton->Enable(false);
	}
	else
	{
		deleteRowButton->Enable(true);
	}
}

void DataTablePanel::InsertRow(size_t row)
{
	if (row <= data_table->GetItemCount())
	{
		wxVector<wxVariant> empty_row;
		empty_row.resize(data_table->GetColumnCount());
		data_table->InsertItem(row, empty_row);
	}
}

void DataTablePanel::RemoveRow(size_t row)
{
	if (row <= data_table->GetItemCount())
	{
		data_table->DeleteItem(row);
		ScanTable();
	}
}

void DataTablePanel::SlotUpdateTable(const std::vector<date_period>& date_intervals, const std::vector<date_period>& date_inter_intervals)
{
	UpdateValidRows();
	auto number_overwrite_rows = valid_rows.size() > date_intervals.size() ? date_intervals.size() : valid_rows.size();
	auto number_insert_rows = date_intervals.size() - valid_rows.size();

	for (size_t index = 0; index < valid_rows.size(); ++index)
	{
		data_table->SetValue(std::to_string(index + 1), valid_rows[index], 0);
		data_table->SetValue(boost_date_to_string(date_intervals[index].begin()), valid_rows[index], 1);

		if (date_intervals[index].begin() == date_intervals[index].end())
		{
			data_table->SetValue(L"", valid_rows[index], 2);
		}
		else
		{
			data_table->SetValue(boost_date_to_string(date_intervals[index].end()), valid_rows[index], 2);
		}

		data_table->SetValue(std::to_string(date_intervals[index].length().days()), valid_rows[index], 3);

		if (index < (date_intervals.size() - 1))
		{
			data_table->SetValue(std::to_string(date_inter_intervals[index].length().days()), valid_rows[index], 4);
		}
		else
		{
			data_table->SetValue(L"", valid_rows[index], 4);
		}
	}

	for (size_t index = valid_rows.size(); index < date_intervals.size(); ++index)
	{
		InsertRow(index);

		data_table->SetValue(std::to_string(index + 1), index, 0);
		data_table->SetValue(boost_date_to_string(date_intervals[index].begin()), index, 1);

		if (date_intervals[index].begin() == date_intervals[index].end())
		{
			data_table->SetValue(L"", index, 2);
		}
		else
		{
			data_table->SetValue(boost_date_to_string(date_intervals[index].end()), index, 2);
		}

		data_table->SetValue(std::to_string(date_intervals[index].length().days()), index, 3);

		if (index < (date_intervals.size() - 1))
		{
			data_table->SetValue(std::to_string(date_inter_intervals[index].length().days()), index, 4);
		}
		else
		{
			data_table->SetValue(L"", index, 4);
		}
	}
}

void DataTablePanel::UpdateValidRows()
{
	valid_rows.clear();
	for (size_t row = 0; row < data_table->GetItemCount(); ++row)
	{
		auto begin_date = ParseDateByCell(row, 1);
		auto end_date = ParseDateByCell(row, 2);

		if (CheckDateInterval(begin_date, end_date) > 0)
		{
			valid_rows.push_back(row);
		}
		else
		{
			data_table->SetValue(L"", row, 0);
			data_table->SetValue(L"", row, 3);
			data_table->SetValue(L"", row, 4);
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

		date_period buffer = date_period(begin_date, end_date);
		
		if (CheckAndAdjustDateInterval(&buffer) > 0)
		{
			date_intervals.push_back(buffer);
		}		
	}

	signal_table_date_intervals(date_intervals);
}

date DataTablePanel::ParseDateByCell(int row, int column)
{
	wxVariant cell_value;
	data_table->GetStore()->GetValueByRow(cell_value, row, column);
	return string_to_boost_date(cell_value.GetString().ToStdString(), dateFormat);
}
