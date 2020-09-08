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
	
	table_widget = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE | wxDV_HORIZ_RULES | wxDV_VERT_RULES, wxDefaultValidator);
	 
	addRowButton = new wxButton(this, wxID_ADD, "Add Row");
	deleteRowButton = new wxButton(this, wxID_DELETE, "Delete Row");
	deleteRowButton->Disable();

	select_group_control = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, 0L, wxDefaultValidator, wxChoiceNameStr);
	

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
	buttons_sizer->Add(select_group_control, sizer_flags1);

	mainSizer->Add(buttons_sizer, 0, wxEXPAND);
	mainSizer->Add(table_sizer, 1, wxEXPAND);
	

	Layout();

	//group_choices.push_back(L"uninitialized");

	Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &DateTablePanel::OnItemActivated, this);
	Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE, &DateTablePanel::OnItemEditing, this);
	Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &DateTablePanel::OnSelectionChanged, this);

	// Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, &DateTablePanel::OnItemEditing, this);
	// Bind(wxEVT_DATAVIEW_ITEM_EDITING_STARTED, &DateTablePanel::OnItemEditing, this);
	Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &DateTablePanel::OnValueChanged, this);

	Bind(wxEVT_BUTTON, &DateTablePanel::OnButtonClicked, this);

	Bind(wxEVT_COMBOBOX, &DateTablePanel::OnComboBoxSelection, this);
	
	InitColumns();

	date_format = InitDateFormat();
}

void DateTablePanel::InitColumns()
{
	// table_widget->ClearColumns();

	table_widget->AppendTextColumn(L"From Date", wxDATAVIEW_CELL_EDITABLE);
	table_widget->AppendTextColumn(L"To Date", wxDATAVIEW_CELL_EDITABLE);
	table_widget->AppendTextColumn(L"Number", wxDATAVIEW_CELL_INERT);
	table_widget->AppendTextColumn(L"Group", wxDATAVIEW_CELL_INERT);
	table_widget->AppendTextColumn(L"Group Number", wxDATAVIEW_CELL_INERT);
	table_widget->AppendTextColumn(L"Duration", wxDATAVIEW_CELL_INERT);
	table_widget->AppendTextColumn(L"Duration to next", wxDATAVIEW_CELL_INERT);
	table_widget->AppendTextColumn(L"Comment", wxDATAVIEW_CELL_EDITABLE);
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

		if (date_interval_bundles[index].exclude == false)
		{
			table_widget->SetValue(std::to_string(date_interval_bundles[index].number + 1), valid_rows[index], 2);
		}
		else
		{
			table_widget->SetValue(L"", valid_rows[index], 2);
		}
		

		// Check for group existing
		if (date_interval_bundles[index].group > date_group_store.GetGroupMax())
		{
			table_widget->SetValue(std::to_string(0), valid_rows[index], 3);
		}
		else
		{
			auto group_name = date_group_store.GetName(date_interval_bundles[index].group);
			table_widget->SetValue(group_name, valid_rows[index], 3);
		}

		table_widget->SetValue(std::to_string(date_interval_bundles[index].group_number + 1), valid_rows[index], 4);
		

		table_widget->SetValue(std::to_string(date_interval_bundles[index].date_interval.length().days() + 1/*!!!*/), valid_rows[index], 5);

		if (index < (date_interval_bundles.size() - 1) && date_interval_bundles[index].exclude == false)
		{
			table_widget->SetValue(std::to_string(date_interval_bundles[index].date_inter_interval.length().days() - 1/*!!!*/), valid_rows[index], 6);
		}
		else
		{
			table_widget->SetValue(L"", valid_rows[index], 6);
		}
	}
}

void DateTablePanel::UpdateGroups(const std::vector<DateGroup>& argument_date_groups)
{
	date_group_store.SetDateGroups(argument_date_groups);

	ScanTable();

	auto date_groups_std_string = date_group_store.GetDateGroupsNames();
	wxArrayString date_groups_strings;
	date_groups_strings.assign(date_groups_std_string.cbegin(), date_groups_std_string.cend());

	select_group_control->Set(date_groups_strings);
	select_group_control->Select(0);
}


void DateTablePanel::OnItemActivated(wxDataViewEvent& event)
{
	// Change GUI interaction behavior
	if (event.GetItem().IsOk() && event.GetDataViewColumn())
	{
		table_widget->EditItem(event.GetItem(), event.GetDataViewColumn());
	}
}

void DateTablePanel::OnValueChanged(wxDataViewEvent& event)
{
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
			auto edited_date = string_to_boost_date(edited_string, date_format);
			if (edited_date.is_special() == false)
			{
				std::string parsed_string = boost_date_to_string(edited_date);
				table_widget->SetValue(parsed_string.c_str(), table_widget->GetSelectedRow(), event.GetColumn());
			}
			else
			{
				table_widget->SetValue(edited_string.c_str(), table_widget->GetSelectedRow(), event.GetColumn());
			}
		}		
	}

	/*if (event.GetEventType() == wxEVT_DATAVIEW_ITEM_EDITING_DONE && event.GetColumn() == 3)
	{
		event.Veto();

		if (event.IsEditCancelled() == false)
		{
			auto group_number = event.GetValue().GetLong();
			//std::cout << __LINE__  << " " << group_number << '\n';
			table_widget->SetValue(group_number, table_widget->GetSelectedRow(), event.GetColumn());
		}
	}*/

	ScanTable();
}

void DateTablePanel::OnSelectionChanged(wxDataViewEvent& event)
{
	UpdateWidgets();
}

void DateTablePanel::OnButtonClicked(wxCommandEvent& event)
{
	auto selections = GetSelections();

	if (event.GetId() == wxID_ADD)
	{
		int insert_row = 0;
		// if no selection do append
		if (selections.size() == 0)
		{
			insert_row = table_widget->GetItemCount();
		}
		else
		{
			insert_row = selections.back() + 1;
		}
		
		InsertRow(insert_row);
		table_widget->SelectRow(insert_row);
		table_widget->EnsureVisible(table_widget->RowToItem(insert_row));
		UpdateWidgets();
	}

	if (event.GetId() == wxID_DELETE && selections.size() > 0)
	{
		auto post_remove_select = selections.front();
		
		for (auto riter = selections.rbegin(); riter != selections.rend(); ++riter)
		{
			RemoveRow(*riter);
		}
		
		if (table_widget->GetItemCount() > 0)
		{
			if (table_widget->GetItemCount() == post_remove_select)
			{
				table_widget->SelectRow(post_remove_select - 1);
			}
			else
			{
				table_widget->SelectRow(post_remove_select);
			}	
		}

		UpdateWidgets();
	}
}

void DateTablePanel::OnComboBoxSelection(wxCommandEvent& event)
{
	auto selections = GetSelections();

	auto group_number = event.GetSelection();
	auto group_name = date_group_store.GetName(group_number);

	for (size_t index = 0; index < selections.size(); ++index)
	{
		table_widget->SetValue(group_name, selections[index], 3);
	}

	ScanTable();
}

void DateTablePanel::UpdateWidgets()
{
	auto selections = GetSelections();

	if (selections.size() == 0)
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
		empty_row[3] = date_group_store.GetName(0);
		table_widget->InsertItem(row, empty_row);
	}
}

void DateTablePanel::RemoveRow(size_t row)
{
	if (row < table_widget->GetItemCount())
	{
		table_widget->DeleteItem(row);
		ScanTable();
	}
	else
	{
		std::cout << "try to remove not existent row" << '\n';
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

	std::vector<DateIntervalBundle> date_interval_bundles;

	for (size_t index = 0; index < valid_rows.size(); ++index)
	{
		auto valid_index = valid_rows[index];
		
		auto begin_date = ParseDateByCell(valid_index, 0);
		auto end_date = ParseDateByCell(valid_index, 1);
		boost::gregorian::date_period temporary_date_interval = boost::gregorian::date_period(begin_date, end_date);

		if (CheckAndAdjustDateInterval(&temporary_date_interval) > 0)
		{
			DateIntervalBundle temporary_interval_bundle;

			temporary_interval_bundle.date_interval = temporary_date_interval;

			wxVariant group_string;
			table_widget->GetValue(group_string, valid_index, 3);
			// std::cout << __LINE__ << " " << index << " " << group_number.GetType() << " "  << group_number.GetLong() <<'\n';
			
			int group_number;
			try
			{
				group_number = date_group_store.GetNumber(group_string.GetString().ToStdWstring());
			}
			catch (...)
			{
				group_number = 0;
			}
			 
			temporary_interval_bundle.group = group_number;

			date_interval_bundles.push_back(temporary_interval_bundle);
		}		
	}

	signal_table_date_interval_bundles(date_interval_bundles);
}

std::vector<int> DateTablePanel::GetSelections()
{
	wxDataViewItemArray selection_array;
	table_widget->GetSelections(selection_array);
	
	std::vector<int> selections;
	for (const auto& selected_item : selection_array)
	{
		int selected_row = table_widget->ItemToRow(selected_item);
		selections.push_back(selected_row);
	}

	return selections;
}



boost::gregorian::date DateTablePanel::ParseDateByCell(int row, int column)
{
	wxVariant cell_value;
	table_widget->GetStore()->GetValueByRow(cell_value, row, column);
	return string_to_boost_date(cell_value.GetString().ToStdString(), date_format);
}
