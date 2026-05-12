#ifndef HOME_TITAN99_CODE_DECADE_SRC_GUI_GROUPS_PANEL_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GUI_GROUPS_PANEL_HPP

#include <wx/dataview.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <limits>
#include <memory>
#include <sigslot/signal.hpp>
#include <string>
#include <utility>
#include <vector>

#include "../packages/group_store.hpp"

class DateGroupsTablePanel : public wxPanel {
 public:
  DateGroupsTablePanel(wxWindow* parent)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTAB_TRAVERSAL, wxPanelNameStr),
        toggle_value_changed_by_function_call_and_not_by_user(false) {
    data_table = std::make_unique<wxDataViewListCtrl>(
                     this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                     wxDV_SINGLE | wxDV_HORIZ_RULES | wxDV_VERT_RULES,
                     wxDefaultValidator)
                     .release();

    Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED,
         &DateGroupsTablePanel::CallbackItemActivated, this);
    Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE,
         &DateGroupsTablePanel::CallbackItemEditing, this);
    Bind(wxEVT_DATAVIEW_SELECTION_CHANGED,
         &DateGroupsTablePanel::CallbackSelectionChanged, this);

    // Bind(wxEVT_DATAVIEW_ITEM_START_EDITING,
    // &DateGroupsTablePanel::OnItemEditing, this);
    // Bind(wxEVT_DATAVIEW_ITEM_EDITING_STARTED,
    // &DateGroupsTablePanel::OnItemEditing, this);
    Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED,
         &DateGroupsTablePanel::CallbackValueChanged, this);

    addRowButton =
        std::make_unique<wxButton>(this, wxID_ADD, "Add Row").release();
    deleteRowButton =
        std::make_unique<wxButton>(this, wxID_DELETE, "Delete Row").release();
    deleteRowButton->Disable();

    Bind(wxEVT_BUTTON, &DateGroupsTablePanel::CallbackButtonClicked, this);

    ////////////////////////////////////////////////////////////////////////////////

    wxBoxSizer* buttons_sizer =
        std::make_unique<wxBoxSizer>(wxHORIZONTAL).release();
    wxSizerFlags buttons_flags = wxSizerFlags().Proportion(0).Border(wxALL, 5);
    buttons_sizer->Add(addRowButton, buttons_flags);
    buttons_sizer->Add(deleteRowButton, buttons_flags);

    wxBoxSizer* table_sizer =
        std::make_unique<wxBoxSizer>(wxHORIZONTAL).release();
    wxSizerFlags data_table_flags =
        wxSizerFlags().Proportion(1).Expand().Border(wxALL, 5);
    table_sizer->Add(data_table, data_table_flags);

    wxBoxSizer* main_sizer = std::make_unique<wxBoxSizer>(wxVERTICAL).release();
    wxSizerFlags buttons_sizer_flags =
        wxSizerFlags().Proportion(0).Expand().Border(wxALL, 0);
    wxSizerFlags table_sizer_flags =
        wxSizerFlags().Proportion(1).Expand().Border(wxALL, 0);
    main_sizer->Add(buttons_sizer, buttons_sizer_flags);
    main_sizer->Add(table_sizer, table_sizer_flags);

    SetSizer(main_sizer);
    // Layout();

    ////////////////////////////////////////////////////////////////////////////////

    data_table->AppendTextColumn(L"Group Number", wxDATAVIEW_CELL_INERT);
    data_table->AppendTextColumn(L"Group Name", wxDATAVIEW_CELL_EDITABLE);
    data_table->AppendToggleColumn(L"Exclude", wxDATAVIEW_CELL_ACTIVATABLE);
  }

  std::wstring GetPanelName() const {
    return std::wstring(L"Date Group Table");
  }

  void ReceiveDateGroups(const std::vector<DateGroup>& argument_date_groups) {
    date_groups = argument_date_groups;

    int change_row_count = 0;
    if (date_groups.size() <= std::numeric_limits<int>::max()) {
      change_row_count =
          static_cast<int>(date_groups.size()) - data_table->GetItemCount();
    } else {
      std::cerr << "too large unsigned int" << '\n';
    }

    if (change_row_count > 0) {
      for (int index = 0; index < change_row_count; ++index) {
        InsertRow(static_cast<std::size_t>(data_table->GetItemCount()));
      }
    }

    if (change_row_count < 0) {
      for (int index = 0; index > change_row_count; --index) {
        RemoveRow(static_cast<std::size_t>(data_table->GetItemCount()) - 1);
      }
    }

    for (size_t index = 0; std::cmp_less(index, data_table->GetItemCount());
         ++index) {
      const auto row = static_cast<unsigned int>(index);
      data_table->SetValue(std::to_wstring(date_groups[index].GetNumber()), row,
                           0);
      data_table->SetValue(date_groups[index].GetName(), row, 1);

      toggle_value_changed_by_function_call_and_not_by_user = true;
      data_table->SetToggleValue(date_groups[index].IsExcluded(), row, 2);
      toggle_value_changed_by_function_call_and_not_by_user = false;
    }
  }

  sigslot::signal<const std::vector<DateGroup>&>& SignalTableDateGroups() {
    return signal_table_date_groups;
  }

 private:
  void UpdateButtons() {
    if (data_table->GetSelectedRow() == wxNOT_FOUND ||
        data_table->GetSelectedRow() < 1) {
      deleteRowButton->Enable(false);
    } else {
      deleteRowButton->Enable(true);
    }
  }
  void InsertRow(size_t row) {
    if (std::cmp_less_equal(row, data_table->GetItemCount())) {
      wxVector<wxVariant> empty_row;
      empty_row.resize(data_table->GetColumnCount());
      empty_row[2] = wxVariant(false);
      data_table->InsertItem(static_cast<unsigned int>(row), empty_row);
    }
  }
  void RemoveRow(size_t row) {
    if (std::cmp_less_equal(row, data_table->GetItemCount())) {
      data_table->DeleteItem(static_cast<unsigned int>(row));
    }
  }

  void CallbackButtonClicked(wxCommandEvent& event) {
    auto selected_row = data_table->GetSelectedRow();

    if (event.GetId() == wxID_ADD) {
      if (selected_row == wxNOT_FOUND) {
        selected_row = data_table->GetItemCount();
      } else if (selected_row < 1) {
        selected_row = 1;
      } else {
        ++selected_row;
      }

      const auto unsigned_row = static_cast<unsigned int>(selected_row);
      InsertRow(unsigned_row);

      date_groups.insert(date_groups.cbegin() + selected_row, DateGroup(""));

      signal_table_date_groups(date_groups);

      data_table->SelectRow(unsigned_row);
      data_table->EnsureVisible(data_table->RowToItem(selected_row));
      UpdateButtons();
    }

    if (event.GetId() == wxID_DELETE && selected_row != wxNOT_FOUND &&
        selected_row >= 1) {
      const auto unsigned_row = static_cast<unsigned int>(selected_row);
      RemoveRow(unsigned_row);

      date_groups.erase(date_groups.cbegin() + selected_row);

      signal_table_date_groups(date_groups);

      if (data_table->GetItemCount() > 0) {
        if (data_table->GetItemCount() == selected_row) {
          data_table->SelectRow(unsigned_row - 1);
        } else {
          data_table->SelectRow(unsigned_row);
        }
      }

      UpdateButtons();
    }
  }

  void CallbackItemActivated(wxDataViewEvent& event) {
    if (event.GetItem().IsOk() && event.GetDataViewColumn()) {
      data_table->EditItem(event.GetItem(), event.GetDataViewColumn());
    }
  }
  void CallbackItemEditing(wxDataViewEvent& event) {
    if (event.GetEventType() == wxEVT_DATAVIEW_ITEM_EDITING_DONE) {
      event.Veto();

      if (event.IsEditCancelled() == false) {
        if (event.GetColumn() == 1) {
          auto edited_string = event.GetValue().GetString().ToStdString();
          const auto selected_row =
              static_cast<unsigned int>(data_table->GetSelectedRow());
          data_table->SetValue(edited_string.c_str(), selected_row,
                               static_cast<unsigned int>(event.GetColumn()));

          date_groups[selected_row].SetName(edited_string);

          signal_table_date_groups(date_groups);
        }
      }
    }
  }

  void CallbackSelectionChanged(wxDataViewEvent& /*event*/) { UpdateButtons(); }

  void CallbackValueChanged(wxDataViewEvent& event) {
    if (event.GetColumn() == 2 && data_table->GetSelectedRow() != wxNOT_FOUND) {
      if (toggle_value_changed_by_function_call_and_not_by_user == false) {
        const auto selected_row =
            static_cast<unsigned int>(data_table->GetSelectedRow());
        auto value = data_table->GetToggleValue(selected_row, 2);
        // std::cout << "value " << value << '\n';
        date_groups[selected_row].SetExcluded(value);
        signal_table_date_groups(date_groups);
      }
    }
  }

  wxWeakRef<wxDataViewListCtrl> data_table;
  wxWeakRef<wxButton> addRowButton;
  wxWeakRef<wxButton> deleteRowButton;

  std::vector<DateGroup> date_groups;
  sigslot::signal<const std::vector<DateGroup>&> signal_table_date_groups;

  // Please fix me, research in wxWidgets
  bool toggle_value_changed_by_function_call_and_not_by_user;
};
#endif  // HOME_TITAN99_CODE_DECADE_SRC_GUI_GROUPS_PANEL_HPP
