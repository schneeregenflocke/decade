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
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxPanelNameStr)
{

	data_table = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_SINGLE | wxDV_HORIZ_RULES | wxDV_VERT_RULES, wxDefaultValidator);

	Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &DateGroupsTablePanel::OnItemActivated, this);
	Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE, &DateGroupsTablePanel::OnItemEditing, this);
	Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &DateGroupsTablePanel::OnSelectionChanged, this);

	// Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, &DateTablePanel::OnItemEditing, this);
	// Bind(wxEVT_DATAVIEW_ITEM_EDITING_STARTED, &DateTablePanel::OnItemEditing, this);
	// Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &DateTablePanel::OnValueChanged, this);

	addRowButton = new wxButton(this, wxID_ADD, "Add Row");
	deleteRowButton = new wxButton(this, wxID_DELETE, "Delete Row");
	deleteRowButton->Disable();

	Bind(wxEVT_BUTTON, &DateGroupsTablePanel::OnButtonClicked, this);


	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(mainSizer);

	wxSizerFlags sizer_flags0;
	sizer_flags0.Proportion(1).Expand().Border(wxALL, 0);

	wxBoxSizer* table_sizer = new wxBoxSizer(wxHORIZONTAL);
	table_sizer->Add(data_table, sizer_flags0);

	wxSizerFlags sizer_flags1;
	sizer_flags1.Proportion(0).Border(wxALL, 5);

	wxBoxSizer* buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
	buttons_sizer->Add(addRowButton, sizer_flags1);
	buttons_sizer->Add(deleteRowButton, sizer_flags1);

	mainSizer->Add(buttons_sizer, 0, wxEXPAND);
	mainSizer->Add(table_sizer, 1, wxEXPAND);
	
	Layout();

	data_table->AppendTextColumn(L"Group Number", wxDATAVIEW_CELL_INERT);
	data_table->AppendTextColumn(L"Group Name", wxDATAVIEW_CELL_EDITABLE);
}

void DateGroupsTablePanel::UpdateGroups(const std::vector<DateGroup>& argument_date_groups)
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

		if (event.IsEditCancelled() == false)
		{
			auto edited_string = event.GetValue().GetString().ToStdWstring();
			data_table->SetValue(edited_string.c_str(), data_table->GetSelectedRow(), event.GetColumn());

			date_groups[data_table->GetSelectedRow()].name = edited_string;

			signal_table_date_groups(date_groups);
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
