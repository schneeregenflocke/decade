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


#include "date_groups_table.h"


DateGroupsTablePanel::DateGroupsTablePanel(wxWindow* parent) :
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxPanelNameStr),
	toggle_value_changed_by_function_call_and_not_by_user(false)
{

	data_table = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_SINGLE | wxDV_HORIZ_RULES | wxDV_VERT_RULES, wxDefaultValidator);

	Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &DateGroupsTablePanel::OnItemActivated, this);
	Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE, &DateGroupsTablePanel::OnItemEditing, this);
	Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &DateGroupsTablePanel::OnSelectionChanged, this);

	// Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, &DateGroupsTablePanel::OnItemEditing, this);
	// Bind(wxEVT_DATAVIEW_ITEM_EDITING_STARTED, &DateGroupsTablePanel::OnItemEditing, this);
	Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &DateGroupsTablePanel::OnValueChanged, this);

	addRowButton = new wxButton(this, wxID_ADD, "Add Row");
	deleteRowButton = new wxButton(this, wxID_DELETE, "Delete Row");
	deleteRowButton->Disable();

	Bind(wxEVT_BUTTON, &DateGroupsTablePanel::OnButtonClicked, this);

	////////////////////////////////////////////////////////////////////////////////

	wxBoxSizer* buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxSizerFlags buttons_flags = wxSizerFlags().Proportion(0).Border(wxALL, 5);
	buttons_sizer->Add(addRowButton, buttons_flags);
	buttons_sizer->Add(deleteRowButton, buttons_flags);


	wxBoxSizer* table_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxSizerFlags data_table_flags = wxSizerFlags().Proportion(1).Expand().Border(wxALL, 5);
	table_sizer->Add(data_table, data_table_flags);


	wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
	wxSizerFlags buttons_sizer_flags = wxSizerFlags().Proportion(0).Expand().Border(wxALL, 0);
	wxSizerFlags table_sizer_flags = wxSizerFlags().Proportion(1).Expand().Border(wxALL, 0);
	main_sizer->Add(buttons_sizer, buttons_sizer_flags);
	main_sizer->Add(table_sizer, table_sizer_flags);
	
	SetSizer(main_sizer);
	//Layout();

	////////////////////////////////////////////////////////////////////////////////

	data_table->AppendTextColumn(L"Group Number", wxDATAVIEW_CELL_INERT);
	data_table->AppendTextColumn(L"Group Name", wxDATAVIEW_CELL_EDITABLE);
	data_table->AppendToggleColumn(L"Exclude", wxDATAVIEW_CELL_ACTIVATABLE);
}

void DateGroupsTablePanel::ReceiveDateGroups(const std::vector<DateGroup>& argument_date_groups)
{
	date_groups = argument_date_groups;
	
	int change_row_count = 0;
	if (date_groups.size() <= std::numeric_limits<int>::max())
	{
		change_row_count = static_cast<int>(date_groups.size()) - data_table->GetItemCount();
	}
	else
	{
		std::cerr << L"too large unsigend int" << '\n';
	}

	if (change_row_count > 0)
	{
		for (int index = 0; index < change_row_count; ++index)
		{
			InsertRow(data_table->GetItemCount());
		}
	}

	if (change_row_count < 0)
	{
		for (int index = 0; index > change_row_count; --index)
		{
			RemoveRow(data_table->GetItemCount() - 1);
		}
	}

	for (size_t index = 0; index < data_table->GetItemCount(); ++index)
	{
		data_table->SetValue(std::to_wstring(date_groups[index].number), index, 0);
		data_table->SetValue(date_groups[index].name, index, 1);

		toggle_value_changed_by_function_call_and_not_by_user = true;
		data_table->SetToggleValue(date_groups[index].exclude, index, 2);
		toggle_value_changed_by_function_call_and_not_by_user = false;
	}
}


void DateGroupsTablePanel::OnValueChanged(wxDataViewEvent& event)
{
	if (event.GetColumn() == 2 && data_table->GetSelectedRow() != wxNOT_FOUND)
	{
		if (toggle_value_changed_by_function_call_and_not_by_user == false)
		{
			auto value = data_table->GetToggleValue(data_table->GetSelectedRow(), 2);
			// std::cout << "value " << value << '\n';
			date_groups[data_table->GetSelectedRow()].exclude = value;
			signal_table_date_groups(date_groups);
		}
	}
}


void DateGroupsTablePanel::OnItemActivated(wxDataViewEvent& event)
{
	if (event.GetItem().IsOk() && event.GetDataViewColumn())
	{
		data_table->EditItem(event.GetItem(), event.GetDataViewColumn());
	}

	
}

void DateGroupsTablePanel::OnItemEditing(wxDataViewEvent& event)
{
	if (event.GetEventType() == wxEVT_DATAVIEW_ITEM_EDITING_DONE)
	{
		event.Veto();

		if (event.IsEditCancelled() == false )
		{
			if (event.GetColumn() == 1)
			{
				auto edited_string = event.GetValue().GetString().ToStdWstring();
				data_table->SetValue(edited_string.c_str(), data_table->GetSelectedRow(), event.GetColumn());

				date_groups[data_table->GetSelectedRow()].name = edited_string;

				signal_table_date_groups(date_groups);
			}	
		}
	}
}

void DateGroupsTablePanel::OnSelectionChanged(wxDataViewEvent& event)
{
	UpdateButtons();
}



void DateGroupsTablePanel::OnButtonClicked(wxCommandEvent& event)
{
	auto selected_row = data_table->GetSelectedRow();

	if (event.GetId() == wxID_ADD)
	{
		if (selected_row == wxNOT_FOUND)
		{
			selected_row = data_table->GetItemCount();
		}
		else if (selected_row < 1)
		{
			selected_row = 1;
		}
		else
		{
			++selected_row;
		}

		InsertRow(selected_row);

		date_groups.insert(date_groups.cbegin() + selected_row, DateGroup(L""));

		signal_table_date_groups(date_groups);

		data_table->SelectRow(selected_row);
		data_table->EnsureVisible(data_table->RowToItem(selected_row));
		UpdateButtons();
	}

	if (event.GetId() == wxID_DELETE && selected_row != wxNOT_FOUND && selected_row >= 1)
	{
		RemoveRow(selected_row);

		date_groups.erase(date_groups.cbegin() + selected_row);

		signal_table_date_groups(date_groups);

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

void DateGroupsTablePanel::UpdateButtons()
{
	if (data_table->GetSelectedRow() == wxNOT_FOUND || data_table->GetSelectedRow() < 1)
	{
		deleteRowButton->Enable(false);
	}
	else
	{
		deleteRowButton->Enable(true);
	}
}

void DateGroupsTablePanel::InsertRow(size_t row)
{
	if (row <= data_table->GetItemCount())
	{
		std::vector<wxVariant> empty_row;
		empty_row.resize(data_table->GetColumnCount());
		empty_row[2] = wxVariant(static_cast<bool>(false));
		data_table->InsertItem(row, empty_row);
	}
}

void DateGroupsTablePanel::RemoveRow(size_t row)
{
	if (row <= data_table->GetItemCount())
	{
		data_table->DeleteItem(row);
	}
}

std::wstring DateGroupsTablePanel::GetPanelName()
{
	return std::wstring(L"Date Group Table");
}
