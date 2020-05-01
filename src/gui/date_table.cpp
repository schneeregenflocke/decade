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


#include "date_table.h"


DateTablePanel::DateTablePanel(wxWindow* parent) : 
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxPanelNameStr)
{

	table_widget = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_SINGLE, wxDefaultValidator);
	 
	addRowButton = new wxButton(this, wxID_ADD, "Add Row");
	deleteRowButton = new wxButton(this, wxID_DELETE, "Delete Row");
	deleteRowButton->Disable();


	Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &DateTablePanel::OnItemActivated, this);
	Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE, &DateTablePanel::OnItemEditing, this);
	Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &DateTablePanel::OnSelectionChanged, this);

	// Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, &DateTablePanel::OnItemEditing, this);
	// Bind(wxEVT_DATAVIEW_ITEM_EDITING_STARTED, &DateTablePanel::OnItemEditing, this);
	// Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &DateTablePanel::OnValueChanged, this);

	Bind(wxEVT_BUTTON, &DateTablePanel::OnButtonClicked, this);
	

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(mainSizer);

	wxSizerFlags sizer_flags0;
	sizer_flags0.Proportion(1).Expand().Border(wxALL, 0);

	wxBoxSizer* table_sizer = new wxBoxSizer(wxHORIZONTAL);
	table_sizer->Add(table_widget, sizer_flags0);

	wxSizerFlags sizer_flags1;
	sizer_flags1.Proportion(0).Border(wxALL, 5);
	
	wxBoxSizer* buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
	buttons_sizer->Add(addRowButton, sizer_flags1);
	buttons_sizer->Add(deleteRowButton, sizer_flags1);

	mainSizer->Add(table_sizer, 1, wxEXPAND);
	mainSizer->Add(buttons_sizer, 0, wxEXPAND);

	Layout();
	
	SetupColumns();


	dateFormat = InitDateFormat();
}

void DateTablePanel::SetupColumns()
{
	table_widget->ClearColumns();

	wxDataViewChoiceRenderer* group_choice_renderer = new wxDataViewChoiceRenderer(group_choices);
	wxDataViewColumn* group_column = new wxDataViewColumn(L"Group", group_choice_renderer, 3);

	table_widget->AppendTextColumn(L"From Date", wxDATAVIEW_CELL_EDITABLE);
	table_widget->AppendTextColumn(L"To Date", wxDATAVIEW_CELL_EDITABLE);
	table_widget->AppendTextColumn(L"Number", wxDATAVIEW_CELL_INERT);

	table_widget->InsertColumn(3, group_column);

	table_widget->AppendTextColumn(L"Group Number", wxDATAVIEW_CELL_INERT);
	table_widget->AppendTextColumn(L"Duration", wxDATAVIEW_CELL_INERT);
	table_widget->AppendTextColumn(L"Duration to next", wxDATAVIEW_CELL_INERT);
	table_widget->AppendTextColumn(L"Comment", wxDATAVIEW_CELL_EDITABLE);

	std::cout << table_widget->GetColumnCount() << " " << table_widget->GetModel()->GetColumnCount() << '\n';
}

void DateTablePanel::UpdateGroups(const std::vector<DateGroup>& date_groups)
{
	group_choices.clear();
	for (const auto& date_group : date_groups)
	{
		group_choices.push_back(date_group.name);
	}

	SetupColumns();
}


void DateTablePanel::UpdateTable(const std::vector<DateIntervalBundle>& date_interval_bundles)
{
	UpdateValidRows();

	int change_row_number = date_interval_bundles.size() - valid_rows.size();

	if (change_row_number > 0)
	{
		for (int index = 0; index < change_row_number; ++index)
		{
			InsertRow(index);
			valid_rows.push_back(index);
		}
	}
	if (change_row_number < 0)
	{
		for (int index = change_row_number; index < 0; ++index)
		{
			RemoveRow(valid_rows.back());
			valid_rows.pop_back();
		}
	}

	for (size_t index = 0; index < valid_rows.size(); ++index)
	{
		table_widget->SetValue(boost_date_to_string(date_interval_bundles[index].date_interval.begin()), valid_rows[index], 0);

		if (date_interval_bundles[index].date_interval.begin() == date_interval_bundles[index].date_interval.end())
		{
			table_widget->SetValue(L"", valid_rows[index], 1);
		}
		else
		{
			table_widget->SetValue(boost_date_to_string(date_interval_bundles[index].date_interval.end()), valid_rows[index], 1);
		}

		table_widget->SetValue(std::to_string(index + 1), valid_rows[index], 2);

		table_widget->SetValue(std::to_string(date_interval_bundles[index].date_interval.length().days()), valid_rows[index], 5);

		if (index < (date_interval_bundles.size() - 1))
		{
			table_widget->SetValue(std::to_string(date_interval_bundles[index].date_inter_interval.length().days()), valid_rows[index], 6);
		}
		else
		{
			table_widget->SetValue(L"", valid_rows[index], 6);
		}
	}
}




void DateTablePanel::OnItemActivated(wxDataViewEvent& event)
{
	if (event.GetItem().IsOk() && event.GetDataViewColumn())
	{
		table_widget->EditItem(event.GetItem(), event.GetDataViewColumn());
	}
}

void DateTablePanel::OnItemEditing(wxDataViewEvent& event)
{
	if (event.GetEventType() == wxEVT_DATAVIEW_ITEM_EDITING_DONE && (event.GetColumn() == 0 || event.GetColumn() == 1))
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
				table_widget->SetValue(parsed_string.c_str(), table_widget->GetSelectedRow(), event.GetColumn());	
			}
			else
			{
				table_widget->SetValue(edited_string.c_str(), table_widget->GetSelectedRow(), event.GetColumn());
			}

			ScanTable();
		}		
	}
}

void DateTablePanel::OnSelectionChanged(wxDataViewEvent& event)
{
	UpdateButtons();
}

void DateTablePanel::OnButtonClicked(wxCommandEvent& event)
{
	auto selected_row = table_widget->GetSelectedRow();

	if (event.GetId() == wxID_ADD)
	{
		if (selected_row == wxNOT_FOUND)
		{
			selected_row = table_widget->GetItemCount();
		}
		else 
		{
			++selected_row;
		}
		
		InsertRow(selected_row);
		table_widget->SelectRow(selected_row);
		table_widget->EnsureVisible(table_widget->RowToItem(selected_row));
		UpdateButtons();
	}

	if (event.GetId() == wxID_DELETE && selected_row != wxNOT_FOUND)
	{
		RemoveRow(selected_row);

		if (table_widget->GetItemCount() > 0)
		{
			if (table_widget->GetItemCount() == selected_row)
			{
				table_widget->SelectRow(selected_row - 1);
			}
			else
			{
				table_widget->SelectRow(selected_row);
			}		
		}

		UpdateButtons();
	}
}

void DateTablePanel::UpdateButtons()
{
	if (table_widget->GetSelectedRow() == wxNOT_FOUND)
	{
		deleteRowButton->Enable(false);
	}
	else
	{
		deleteRowButton->Enable(true);
	}
}

void DateTablePanel::InsertRow(size_t row)
{
	if (row <= table_widget->GetItemCount())
	{
		wxVector<wxVariant> empty_row;
		empty_row.resize(table_widget->GetColumnCount());
		table_widget->InsertItem(row, empty_row);
	}
}

void DateTablePanel::RemoveRow(size_t row)
{
	if (row <= table_widget->GetItemCount())
	{
		table_widget->DeleteItem(row);
		ScanTable();
	}
}



std::wstring DateTablePanel::GetPanelName()
{
	return std::wstring(L"Date Table");
}

void DateTablePanel::UpdateValidRows()
{
	valid_rows.clear();
	for (size_t row = 0; row < table_widget->GetItemCount(); ++row)
	{
		auto begin_date = ParseDateByCell(row, 0);
		auto end_date = ParseDateByCell(row, 1);

		if (CheckDateInterval(begin_date, end_date) > 0)
		{
			valid_rows.push_back(row);
		}
		else
		{
			table_widget->SetValue(L"", row, 2);
			table_widget->SetValue(L"", row, 5);
			table_widget->SetValue(L"", row, 6);
		}
	}
}

void DateTablePanel::ScanTable()
{
	UpdateValidRows();

	std::vector<DateIntervalBundle> date_intervals;

	for (size_t index = 0; index < valid_rows.size(); ++index)
	{
		auto begin_date = ParseDateByCell(valid_rows[index], 0);
		auto end_date = ParseDateByCell(valid_rows[index], 1);

		date_period buffer = date_period(begin_date, end_date);
		
		if (CheckAndAdjustDateInterval(&buffer) > 0)
		{
			date_intervals.push_back(buffer);
		}		
	}

	signal_table_date_interval_bundles(date_intervals);
}



date DateTablePanel::ParseDateByCell(int row, int column)
{
	wxVariant cell_value;
	table_widget->GetStore()->GetValueByRow(cell_value, row, column);
	return string_to_boost_date(cell_value.GetString().ToStdString(), dateFormat);
}
