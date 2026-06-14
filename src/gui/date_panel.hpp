#ifndef DATE_PANEL_HPP
#define DATE_PANEL_HPP

#include <wx/dataview.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <cstdint>
#include <exception>
#include <memory>
#include <ranges>
#include <sigslot/signal.hpp>
#include <string>
#include <utility>
#include <vector>

#include "../packages/date.hpp"
#include "../packages/date_entry.hpp"
#include "../packages/date_format.hpp"
#include "../packages/date_group.hpp"
#include "../packages/date_group_store.hpp"
#include "../packages/date_period.hpp"
#include "wx_owned.hpp"

// The panel is the user-facing boundary for date intervals: the "To Date"
// column shows and accepts the *inclusive* end date, while every DateEntry
// sent or received carries the internal half-open period [begin, end). The
// conversion happens exactly here (PeriodFromInclusiveDates on input, Last()
// on display) and nowhere else.
class DateTablePanel : public wxPanel {
 public:
  // `date_format` is owned by the composition root (MainWindow) so the whole
  // application shares one locale configuration.
  DateTablePanel(wxWindow* parent, LocaleDateFormatter& date_format)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTAB_TRAVERSAL, wxPanelNameStr),
        date_format_(date_format) {
    table_widget_ = MakeOwned<wxDataViewListCtrl>(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxDV_MULTIPLE | wxDV_HORIZ_RULES | wxDV_VERT_RULES, wxDefaultValidator);

    add_row_button_ = MakeOwned<wxButton>(this, wxID_ADD, "Add Row");
    delete_row_button_ = MakeOwned<wxButton>(this, wxID_DELETE, "Delete Row");
    delete_row_button_->Disable();

    select_group_control_ = MakeOwned<wxComboBox>(
        this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0,
        nullptr, 0L, wxDefaultValidator, wxChoiceNameStr);

    auto* buttons_sizer = MakeOwned<wxBoxSizer>(wxHORIZONTAL);
    wxSizerFlags const buttons_flags =
        wxSizerFlags().Proportion(0).Border(wxALL, 5);
    buttons_sizer->Add(add_row_button_, buttons_flags);
    buttons_sizer->Add(delete_row_button_, buttons_flags);
    buttons_sizer->Add(select_group_control_, buttons_flags);

    auto* table_sizer = MakeOwned<wxBoxSizer>(wxHORIZONTAL);
    wxSizerFlags const data_table_flags =
        wxSizerFlags().Proportion(1).Expand().Border(wxALL, 5);
    table_sizer->Add(table_widget_, data_table_flags);

    auto* main_sizer = MakeOwned<wxBoxSizer>(wxVERTICAL);
    wxSizerFlags const buttons_sizer_flags =
        wxSizerFlags().Proportion(0).Expand().Border(wxALL, 0);
    wxSizerFlags const table_sizer_flags =
        wxSizerFlags().Proportion(1).Expand().Border(wxALL, 0);
    main_sizer->Add(buttons_sizer, buttons_sizer_flags);
    main_sizer->Add(table_sizer, table_sizer_flags);

    SetSizer(main_sizer);

    Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &DateTablePanel::OnItemActivated, this);
    Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE, &DateTablePanel::OnItemEditing,
         this);
    Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &DateTablePanel::OnSelectionChanged,
         this);
    Bind(wxEVT_BUTTON, &DateTablePanel::OnButtonClicked, this);
    Bind(wxEVT_COMBOBOX, &DateTablePanel::OnComboBoxSelection, this);

    InitColumns();
  }

  void ReceiveDateEntries(const std::vector<DateEntry>& date_entries) {
    auto valid_rows_list = BuildValidRowsList();

    // calculate difference
    const auto change_row_number = static_cast<int>(date_entries.size()) -
                                   static_cast<int>(valid_rows_list.size());

    if (change_row_number > 0) {
      for (int index = 0; index < change_row_number; ++index) {
        // append
        const auto append_index =
            static_cast<std::size_t>(table_widget_->GetItemCount());
        InsertRow(append_index);
        valid_rows_list.push_back(append_index);
      }
    }

    // negative number
    if (change_row_number < 0) {
      // index not used in body
      for (int index = change_row_number; index < 0; ++index) {
        RemoveRow(valid_rows_list.back());
        valid_rows_list.pop_back();
      }
    }

    // fill table from received date_entries
    for (size_t index = 0; index < valid_rows_list.size(); ++index) {
      const auto row = static_cast<unsigned int>(valid_rows_list[index]);
      const auto first_date =
          date_format_.Format(date_entries[index].GetDateInterval().Begin());
      table_widget_->SetValue(first_date, row,
                              ColumnIndex(Columns::first_date));

      // The to-column shows the inclusive last day; a single-day period
      // (length 1) shows an empty to-date.
      std::string second_date;
      if (date_entries[index].GetDateInterval().LengthDays() > 1) {
        second_date =
            date_format_.Format(date_entries[index].GetDateInterval().Last());
      }
      table_widget_->SetValue(second_date, row,
                              ColumnIndex(Columns::second_date));

      const std::string number =
          std::to_string(date_entries[index].GetNumber() + 1);
      table_widget_->SetValue(number, row, ColumnIndex(Columns::number));

      // Unknown groups fall back to the default group (0); the store resets
      // them the same way on its side (CheckAndAdjustGroupIntegrity).
      int group = date_entries[index].GetGroup();
      if (group > date_group_store_.GetGroupMax()) {
        group = 0;
      }
      table_widget_->SetValue(date_group_store_.GetName(group), row,
                              ColumnIndex(Columns::group));

      table_widget_->SetValue(
          std::to_string(date_entries[index].GetGroupNumber() + 1), row,
          ColumnIndex(Columns::group_number));

      table_widget_->SetValue(
          std::to_string(date_entries[index].GetDateInterval().LengthDays()),
          row, ColumnIndex(Columns::duration));

      // The inter-interval (end_i, begin_{i+1}) is half-open as well, so its
      // length is exactly the number of free days between the two entries.
      if ((index + 1) < date_entries.size()) {
        table_widget_->SetValue(
            std::to_string(
                date_entries[index].GetDateInterInterval().LengthDays()),
            row, ColumnIndex(Columns::duration_to_next));
      } else {
        table_widget_->SetValue(L"", row,
                                ColumnIndex(Columns::duration_to_next));
      }
    }
  }

  void ReceiveDateGroups(const std::vector<DateGroup>& date_groups) {
    date_group_store_.ReceiveDateGroups(date_groups);

    SendDateEntries();

    auto date_groups_std_string = date_group_store_.GetDateGroupsNames();
    wxArrayString date_groups_strings;
    date_groups_strings.assign(date_groups_std_string.cbegin(),
                               date_groups_std_string.cend());

    select_group_control_->Set(date_groups_strings);
    select_group_control_->Select(0);
  }

  sigslot::signal<const std::vector<DateEntry>&>& SignalTableDateEntries() {
    return signal_table_date_entries_;
  }

 private:
  void InitColumns() {
    table_widget_->AppendTextColumn(L"From Date", wxDATAVIEW_CELL_EDITABLE);
    table_widget_->AppendTextColumn(L"To Date", wxDATAVIEW_CELL_EDITABLE);
    table_widget_->AppendTextColumn(L"Number", wxDATAVIEW_CELL_INERT);
    table_widget_->AppendTextColumn(L"Group", wxDATAVIEW_CELL_INERT);
    table_widget_->AppendTextColumn(L"Group Number", wxDATAVIEW_CELL_INERT);
    table_widget_->AppendTextColumn(L"Duration", wxDATAVIEW_CELL_INERT);
    table_widget_->AppendTextColumn(L"Duration to next", wxDATAVIEW_CELL_INERT);
  }

  void SendDateEntries() {
    auto valid_rows_list = BuildValidRowsList();

    std::vector<DateEntry> date_entries;

    for (unsigned long const valid_index : valid_rows_list) {
      const auto begin_date =
          GetDateByCell({.row = static_cast<unsigned int>(valid_index),
                         .column = Columns::first_date});
      const auto last_date =
          GetDateByCell({.row = static_cast<unsigned int>(valid_index),
                         .column = Columns::second_date});

      const DatePeriod date_interval =
          PeriodFromInclusiveDates(begin_date, last_date);

      if (!date_interval.IsNull()) {
        DateEntry date_entry;
        date_entry.SetDateInterval(date_interval);

        wxVariant group_string;
        table_widget_->GetValue(group_string,
                                static_cast<unsigned int>(valid_index),
                                ColumnIndex(Columns::group));

        int group_number = 0;
        try {
          group_number = date_group_store_.GetNumber(
              group_string.GetString().ToStdString());
        } catch (const std::exception&) {
          group_number = 0;
        }

        date_entry.SetGroup(group_number);

        date_entries.push_back(date_entry);
      }
    }

    signal_table_date_entries_(date_entries);
  }

  void UpdateDeleteButton() {
    auto selections = GetSelectionList();

    if (selections.empty()) {
      delete_row_button_->Enable(false);
    } else {
      delete_row_button_->Enable(true);
    }
  }

  std::vector<size_t> BuildValidRowsList() {
    std::vector<size_t> valid_rows_list;

    for (size_t index = 0; std::cmp_less(index, table_widget_->GetItemCount());
         ++index) {
      auto begin_date = GetDateByCell({.row = static_cast<unsigned int>(index),
                                       .column = Columns::first_date});
      auto last_date = GetDateByCell({.row = static_cast<unsigned int>(index),
                                      .column = Columns::second_date});

      if (!PeriodFromInclusiveDates(begin_date, last_date).IsNull()) {
        valid_rows_list.push_back(index);
        continue;
      }

      const auto row = static_cast<unsigned int>(index);
      table_widget_->SetValue("", row, ColumnIndex(Columns::number));
      table_widget_->SetValue("", row, ColumnIndex(Columns::group));
      table_widget_->SetValue("", row, ColumnIndex(Columns::group_number));
      table_widget_->SetValue("", row, ColumnIndex(Columns::duration));
      table_widget_->SetValue("", row, ColumnIndex(Columns::duration_to_next));
    }

    return valid_rows_list;
  }

  std::vector<unsigned int> GetSelectionList() {
    wxDataViewItemArray selection_array;
    table_widget_->GetSelections(selection_array);

    std::vector<unsigned int> selections;
    for (const auto& selected_item : selection_array) {
      const int selected_row = table_widget_->ItemToRow(selected_item);
      selections.push_back(static_cast<unsigned int>(selected_row));
    }

    return selections;
  }

  void InsertRow(size_t row) {
    if (std::cmp_less_equal(row, table_widget_->GetItemCount())) {
      wxVector<wxVariant> empty_row;
      empty_row.resize(table_widget_->GetColumnCount());
      empty_row[static_cast<size_t>(ColumnIndex(Columns::group))] =
          date_group_store_.GetName(0);
      table_widget_->InsertItem(static_cast<unsigned int>(row), empty_row);
    }
  }

  void RemoveRow(size_t row) {
    // Guard against an out-of-range row instead of throwing: this runs inside a
    // wx event handler, where an escaping exception would tear down the app.
    if (std::cmp_greater_equal(row, table_widget_->GetItemCount())) {
      return;
    }
    table_widget_->DeleteItem(static_cast<unsigned int>(row));
  }

  enum class Columns : std::uint8_t {
    first_date,
    second_date,
    number,
    group,
    group_number,
    duration,
    duration_to_next
  };

  struct CellIndex {
    unsigned int row{0};
    Columns column{Columns::first_date};
  };

  static constexpr unsigned int ColumnIndex(Columns column) {
    return static_cast<unsigned int>(column);
  }

  Date GetDateByCell(CellIndex cell) {
    wxVariant cell_value;
    table_widget_->GetStore()->GetValueByRow(cell_value, cell.row,
                                             ColumnIndex(cell.column));
    return date_format_.Parse(cell_value.GetString().ToStdString());
  }

  void OnItemActivated(wxDataViewEvent& event) {
    // Change GUI interaction behavior
    if (event.GetItem().IsOk() && (event.GetDataViewColumn() != nullptr)) {
      table_widget_->EditItem(event.GetItem(), event.GetDataViewColumn());
    }
  }

  void OnItemEditing(wxDataViewEvent& event) {
    if (event.GetEventType() == wxEVT_DATAVIEW_ITEM_EDITING_DONE &&
        (event.GetColumn() == ColumnIndex(Columns::first_date) ||
         event.GetColumn() == ColumnIndex(Columns::second_date))) {
      event.Veto();

      if (!event.IsEditCancelled()) {
        auto edited_string = event.GetValue().GetString().ToStdString();

        const auto selected_row =
            static_cast<unsigned int>(table_widget_->GetSelectedRow());
        const auto edited_column = static_cast<unsigned int>(event.GetColumn());

        // Check Date
        auto edited_date = date_format_.Parse(edited_string);
        if (edited_date.IsValid()) {
          std::string const parsed_string = date_format_.Format(edited_date);
          table_widget_->SetValue(parsed_string.c_str(), selected_row,
                                  edited_column);
        } else {
          table_widget_->SetValue(edited_string.c_str(), selected_row,
                                  edited_column);
        }

        SendDateEntries();
      }
    }
  }

  void OnSelectionChanged(wxDataViewEvent& event) {
    (void)event;
    UpdateDeleteButton();
  }

  void OnButtonClicked(wxCommandEvent& event) {
    auto selections = GetSelectionList();

    if (event.GetId() == wxID_ADD) {
      unsigned int insert_row = 0;
      // if no selection do append
      if (selections.empty()) {
        insert_row = static_cast<unsigned int>(table_widget_->GetItemCount());
      } else {
        insert_row = selections.back() + 1;
      }

      InsertRow(insert_row);
      table_widget_->SelectRow(insert_row);
      table_widget_->EnsureVisible(
          table_widget_->RowToItem(static_cast<int>(insert_row)));
      UpdateDeleteButton();
    }

    if (event.GetId() == wxID_DELETE && !selections.empty()) {
      const auto post_remove_select = selections.front();

      for (unsigned int const& selection : std::views::reverse(selections)) {
        RemoveRow(selection);
      }

      if (table_widget_->GetItemCount() > 0) {
        if (std::cmp_equal(table_widget_->GetItemCount(), post_remove_select)) {
          table_widget_->SelectRow(post_remove_select - 1);
        } else {
          table_widget_->SelectRow(post_remove_select);
        }
      }

      UpdateDeleteButton();

      SendDateEntries();
    }
  }

  void OnComboBoxSelection(wxCommandEvent& event) {
    auto selections = GetSelectionList();

    auto group_number = event.GetSelection();
    auto group_name = date_group_store_.GetName(group_number);

    for (unsigned int const selection : selections) {
      table_widget_->SetValue(group_name, selection,
                              ColumnIndex(Columns::group));
    }

    SendDateEntries();
  }

  wxWeakRef<wxDataViewListCtrl> table_widget_;
  wxWeakRef<wxButton> add_row_button_;
  wxWeakRef<wxButton> delete_row_button_;
  wxWeakRef<wxComboBox> select_group_control_;

  LocaleDateFormatter& date_format_;
  DateGroupStore date_group_store_;
  sigslot::signal<const std::vector<DateEntry>&> signal_table_date_entries_;
};
#endif  // DATE_PANEL_HPP
