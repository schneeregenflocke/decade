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

	data_table = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_SINGLE, wxDefaultValidator);

	Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &DateGroupsTablePanel::OnItemActivated, this);
	Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE, &DateGroupsTablePanel::OnItemEditing, this);
	Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &DateGroupsTablePanel::OnSelectionChanged, this);

	// Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, &DataTablePanel::OnItemEditing, this);
	// Bind(wxEVT_DATAVIEW_ITEM_EDITING_STARTED, &DataTablePanel::OnItemEditing, this);
	// Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &DataTablePanel::OnValueChanged, this);

	addRowButton = new wxButton(this, wxID_ADD, "Add Row");
	deleteRowButton = new wxButton(this, wxID_DELETE, "Delete Row");
	deleteRowButton->Disable();

	Bind(wxEVT_BUTTON, &DateGroupsTablePanel::OnButtonClicked, this);


	wxBoxSizer* subSizer = new wxBoxSizer(wxHORIZONTAL);
	subSizer->Add(addRowButton, 0, wxALL, 5);
	subSizer->Add(deleteRowButton, 0, wxALL, 5);


	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(data_table, 1, wxEXPAND | wxALL, 5);
	mainSizer->Add(subSizer, 0, wxEXPAND);
	SetSizer(mainSizer);

	data_table->AppendTextColumn(L"Group Number", wxDATAVIEW_CELL_INERT);
	data_table->AppendTextColumn(L"Group Name", wxDATAVIEW_CELL_EDITABLE);

	/*
	wxArrayString test_string;
	test_string.Add(L"choice A");
	test_string.Add(L"choice B");
	wxDataViewChoiceRenderer* choice_renderer = new wxDataViewChoiceRenderer(test_string);
	wxDataViewColumn* test_column = new wxDataViewColumn(L"test column", choice_renderer, 5);
	data_table->AppendColumn(test_column);
	*/
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

void DateGroupsTablePanel::UpdateButtons()
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

void DateGroupsTablePanel::InsertRow(size_t row)
{
	if (row <= data_table->GetItemCount())
	{
		std::vector<wxVariant> empty_row;
		empty_row.resize(data_table->GetColumnCount());
		data_table->InsertItem(row, empty_row);

		date_groups.insert(date_groups.cbegin() + row, DateGroup(0, L""));
		signal_table_date_groups(date_groups);
	}
}

void DateGroupsTablePanel::RemoveRow(size_t row)
{
	if (row <= data_table->GetItemCount())
	{
		data_table->DeleteItem(row);

		date_groups.erase(date_groups.cbegin() + row);
		signal_table_date_groups(date_groups);
	}
}

std::wstring DateGroupsTablePanel::GetPanelName()
{
	return std::wstring(L"Date Group Table");
}
