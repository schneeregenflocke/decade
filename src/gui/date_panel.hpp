#ifndef HOME_TITAN99_CODE_DECADE_SRC_GUI_DATE_PANEL_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GUI_DATE_PANEL_HPP

#include <wx/dataview.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sigslot/signal.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "../date_utils.hpp"
#include "../packages/date_store.hpp"
#include "../packages/group_store.hpp"

class DateTablePanel {
 public:
  DateTablePanel(wxWindow* parent) {
    wx_panel = std::make_unique<wxPanel>(parent, wxID_ANY, wxDefaultPosition,
                                         wxDefaultSize, wxTAB_TRAVERSAL,
                                         wxPanelNameStr)
                   .release();
    table_widget =
        std::make_unique<wxDataViewListCtrl>(
            wx_panel.get(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
            wxDV_MULTIPLE | wxDV_HORIZ_RULES | wxDV_VERT_RULES,
            wxDefaultValidator)
            .release();

    addRowButton =
        std::make_unique<wxButton>(wx_panel.get(), wxID_ADD, "Add Row")
            .release();
    deleteRowButton =
        std::make_unique<wxButton>(wx_panel.get(), wxID_DELETE, "Delete Row")
            .release();
    deleteRowButton->Disable();

    select_group_control =
        std::make_unique<wxComboBox>(
            wx_panel.get(), wxID_ANY, wxEmptyString, wxDefaultPosition,
            wxDefaultSize, 0, nullptr, 0L, wxDefaultValidator, wxChoiceNameStr)
            .release();

    ////////////////////////////////////////////////////////////////////////////////

    wxBoxSizer* buttons_sizer =
        std::make_unique<wxBoxSizer>(wxHORIZONTAL).release();
    wxSizerFlags buttons_flags = wxSizerFlags().Proportion(0).Border(wxALL, 5);
    buttons_sizer->Add(addRowButton, buttons_flags);
    buttons_sizer->Add(deleteRowButton, buttons_flags);
    buttons_sizer->Add(select_group_control, buttons_flags);

    wxBoxSizer* table_sizer =
        std::make_unique<wxBoxSizer>(wxHORIZONTAL).release();
    wxSizerFlags data_table_flags =
        wxSizerFlags().Proportion(1).Expand().Border(wxALL, 5);
    table_sizer->Add(table_widget, data_table_flags);

    wxBoxSizer* main_sizer = std::make_unique<wxBoxSizer>(wxVERTICAL).release();
    wxSizerFlags buttons_sizer_flags =
        wxSizerFlags().Proportion(0).Expand().Border(wxALL, 0);
    wxSizerFlags table_sizer_flags =
        wxSizerFlags().Proportion(1).Expand().Border(wxALL, 0);
    main_sizer->Add(buttons_sizer, buttons_sizer_flags);
    main_sizer->Add(table_sizer, table_sizer_flags);

    wx_panel->SetSizer(main_sizer);
    // Layout();

    wx_panel->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED,
                   &DateTablePanel::OnItemActivated, this);
    wx_panel->Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE,
                   &DateTablePanel::OnItemEditing, this);
    wx_panel->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED,
                   &DateTablePanel::OnSelectionChanged, this);

    // Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, &DateTablePanel::OnItemEditing,
    // this); Bind(wxEVT_DATAVIEW_ITEM_EDITING_STARTED,
    // &DateTablePanel::OnItemEditing, this);
    wx_panel->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED,
                   &DateTablePanel::OnValueChanged, this);
    wx_panel->Bind(wxEVT_BUTTON, &DateTablePanel::OnButtonClicked, this);
    wx_panel->Bind(wxEVT_COMBOBOX, &DateTablePanel::OnComboBoxSelection, this);

    InitColumns();
    date_format = InitDateFormat();
  }

  wxPanel* PanelPtr() { return wx_panel.get(); }

  void ReceiveDateIntervalBundles(
      const std::vector<DateIntervalBundle>& date_interval_bundles) {
    auto valid_rows_list = BuildValidRowsList();

    // calculate difference
    const auto change_row_number =
        static_cast<int>(date_interval_bundles.size()) -
        static_cast<int>(valid_rows_list.size());

    if (change_row_number > 0) {
      for (int index = 0; index < change_row_number; ++index) {
        // append
        const auto append_index =
            static_cast<std::size_t>(table_widget->GetItemCount());
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

    // fill table from received date_interval_bundles
    for (size_t index = 0; index < valid_rows_list.size(); ++index) {
      const auto row = static_cast<unsigned int>(valid_rows_list[index]);
      const auto first_date = boost_date_to_string(
          date_interval_bundles[index].GetDateInterval().begin());
      table_widget->SetValue(first_date, row, ColumnIndex(Columns::first_date));

      std::string second_date;
      // single date
      if (date_interval_bundles[index].GetDateInterval().begin() ==
          date_interval_bundles[index].GetDateInterval().end()) {
        second_date = "";
      }
      // date interval
      else {
        second_date = boost_date_to_string(
            date_interval_bundles[index].GetDateInterval().end());
      }
      table_widget->SetValue(second_date, row,
                             ColumnIndex(Columns::second_date));

      std::string number;
      if (!date_interval_bundles[index].IsExcluded()) {
        number = std::to_string(date_interval_bundles[index].GetNumber() + 1);

      } else {
        number = "";
      }
      table_widget->SetValue(number, row, ColumnIndex(Columns::number));

      // check if group exists
      if (date_interval_bundles[index].GetGroup() >
          date_group_store.GetGroupMax()) {
        table_widget->SetValue(std::to_string(0), row,
                               ColumnIndex(Columns::group));
        throw std::runtime_error("group cannot exist");
      } else {
        auto group_name =
            date_group_store.GetName(date_interval_bundles[index].GetGroup());
        table_widget->SetValue(group_name, row, ColumnIndex(Columns::group));
      }

      table_widget->SetValue(
          std::to_string(date_interval_bundles[index].GetGroupNumber() + 1),
          row, ColumnIndex(Columns::group_number));

      table_widget->SetValue(
          std::to_string(
              date_interval_bundles[index].GetDateInterval().length().days() +
              1 /*!!!*/),
          row, ColumnIndex(Columns::duration));

      if ((index + 1) < date_interval_bundles.size() &&
          !date_interval_bundles[index].IsExcluded()) {
        table_widget->SetValue(std::to_string(date_interval_bundles[index]
                                                  .GetDateInterInterval()
                                                  .length()
                                                  .days() -
                                              1 /*!!!*/),
                               row, ColumnIndex(Columns::duration_to_next));
      } else {
        table_widget->SetValue(L"", row,
                               ColumnIndex(Columns::duration_to_next));
      }
    }
  }

  void ReceiveDateGroups(const std::vector<DateGroup>& date_groups) {
    date_group_store.ReceiveDateGroups(date_groups);

    SendDateIntervalBundles();

    auto date_groups_std_string = date_group_store.GetDateGroupsNames();
    wxArrayString date_groups_strings;
    date_groups_strings.assign(date_groups_std_string.cbegin(),
                               date_groups_std_string.cend());

    select_group_control->Set(date_groups_strings);
    select_group_control->Select(0);
  }

  sigslot::signal<const std::vector<DateIntervalBundle>&>&
  SignalTableDateIntervalBundles() {
    return signal_table_date_interval_bundles;
  }

 private:
  void InitColumns() {
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

  void SendDateIntervalBundles() {
    auto valid_rows_list = BuildValidRowsList();

    std::vector<DateIntervalBundle> date_interval_bundles;

    for (size_t index = 0; index < valid_rows_list.size(); ++index) {
      const auto valid_index = valid_rows_list[index];

      const auto begin_date = GetDateByCell(
          {static_cast<unsigned int>(valid_index), Columns::first_date});
      const auto end_date = GetDateByCell(
          {static_cast<unsigned int>(valid_index), Columns::second_date});

      boost::gregorian::date_period date_interval =
          boost::gregorian::date_period(begin_date, end_date);

      if (CheckAndAdjustDateInterval(&date_interval) > 0) {
        DateIntervalBundle date_interval_bundle;
        date_interval_bundle.SetDateInterval(date_interval);

        wxVariant group_string;
        table_widget->GetValue(group_string,
                               static_cast<unsigned int>(valid_index),
                               ColumnIndex(Columns::group));

        int group_number = 0;
        try {
          group_number = date_group_store.GetNumber(
              group_string.GetString().ToStdString());
        } catch (const std::exception&) {
          group_number = 0;
        }

        date_interval_bundle.SetGroup(group_number);

        date_interval_bundles.push_back(date_interval_bundle);
      }
    }

    signal_table_date_interval_bundles(date_interval_bundles);
  }

  void UpdateDeleteButton() {
    auto selections = GetSelectionList();

    if (selections.empty()) {
      deleteRowButton->Enable(false);
    } else {
      deleteRowButton->Enable(true);
    }
  }

  std::vector<size_t> BuildValidRowsList() {
    std::vector<size_t> valid_rows_list;

    for (size_t index = 0;
         index < static_cast<size_t>(table_widget->GetItemCount()); ++index) {
      auto begin_date = GetDateByCell(
          {static_cast<unsigned int>(index), Columns::first_date});
      auto end_date = GetDateByCell(
          {static_cast<unsigned int>(index), Columns::second_date});

      if (CheckDateInterval(begin_date, end_date) > 0) {
        valid_rows_list.push_back(index);
        continue;
      }

      const auto row = static_cast<unsigned int>(index);
      table_widget->SetValue("", row, ColumnIndex(Columns::number));
      table_widget->SetValue("", row, ColumnIndex(Columns::group));
      table_widget->SetValue("", row, ColumnIndex(Columns::group_number));
      table_widget->SetValue("", row, ColumnIndex(Columns::duration));
      table_widget->SetValue("", row, ColumnIndex(Columns::duration_to_next));
    }

    return valid_rows_list;
  }

  std::vector<unsigned int> GetSelectionList() {
    wxDataViewItemArray selection_array;
    table_widget->GetSelections(selection_array);

    std::vector<unsigned int> selections;
    for (const auto& selected_item : selection_array) {
      const int selected_row = table_widget->ItemToRow(selected_item);
      selections.push_back(static_cast<unsigned int>(selected_row));
    }

    return selections;
  }

  void InsertRow(size_t row) {
    if (row <= static_cast<size_t>(table_widget->GetItemCount())) {
      wxVector<wxVariant> empty_row;
      empty_row.resize(table_widget->GetColumnCount());
      empty_row[static_cast<size_t>(ColumnIndex(Columns::group))] =
          date_group_store.GetName(0);
      table_widget->InsertItem(static_cast<unsigned int>(row), empty_row);
    }
  }

  void RemoveRow(size_t row) {
    if (row >= static_cast<size_t>(table_widget->GetItemCount())) {
      std::cout << "try to remove not existent row" << '\n';
      throw std::runtime_error("try to remove not existent row");
    }
    table_widget->DeleteItem(static_cast<unsigned int>(row));
  }

  enum class Columns : std::uint8_t {
    first_date,
    second_date,
    number,
    group,
    group_number,
    duration,
    duration_to_next,
    comment
  };

  struct CellIndex {
    unsigned int row{0};
    Columns column{Columns::first_date};
  };

  static constexpr unsigned int ColumnIndex(Columns column) {
    return static_cast<unsigned int>(column);
  }

  boost::gregorian::date GetDateByCell(CellIndex cell) const {
    wxVariant cell_value;
    table_widget->GetStore()->GetValueByRow(cell_value, cell.row,
                                            ColumnIndex(cell.column));
    return string_to_boost_date(cell_value.GetString().ToStdString(),
                                date_format);
  }

  void OnItemActivated(wxDataViewEvent& event) {
    // Change GUI interaction behavior
    if (event.GetItem().IsOk() && event.GetDataViewColumn()) {
      table_widget->EditItem(event.GetItem(), event.GetDataViewColumn());
    }
  }

  void OnValueChanged(wxDataViewEvent& event) { (void)event; }

  void OnItemEditing(wxDataViewEvent& event) {
    if (event.GetEventType() == wxEVT_DATAVIEW_ITEM_EDITING_DONE &&
        (event.GetColumn() == ColumnIndex(Columns::first_date) ||
         event.GetColumn() == ColumnIndex(Columns::second_date))) {
      event.Veto();

      if (!event.IsEditCancelled()) {
        auto edited_string = event.GetValue().GetString().ToStdString();

        const auto selected_row =
            static_cast<unsigned int>(table_widget->GetSelectedRow());
        const auto edited_column = static_cast<unsigned int>(event.GetColumn());

        // Check Date
        auto edited_date = string_to_boost_date(edited_string, date_format);
        if (!edited_date.is_special()) {
          std::string parsed_string = boost_date_to_string(edited_date);
          table_widget->SetValue(parsed_string.c_str(), selected_row,
                                 edited_column);
        } else {
          table_widget->SetValue(edited_string.c_str(), selected_row,
                                 edited_column);
        }

        SendDateIntervalBundles();
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
        insert_row = static_cast<unsigned int>(table_widget->GetItemCount());
      } else {
        insert_row = selections.back() + 1;
      }

      InsertRow(insert_row);
      table_widget->SelectRow(insert_row);
      table_widget->EnsureVisible(
          table_widget->RowToItem(static_cast<int>(insert_row)));
      UpdateDeleteButton();
    }

    if (event.GetId() == wxID_DELETE && !selections.empty()) {
      const auto post_remove_select = selections.front();

      for (auto riter = selections.rbegin(); riter != selections.rend();
           ++riter) {
        RemoveRow(*riter);
      }

      if (table_widget->GetItemCount() > 0) {
        if (static_cast<unsigned int>(table_widget->GetItemCount()) ==
            post_remove_select) {
          table_widget->SelectRow(post_remove_select - 1);
        } else {
          table_widget->SelectRow(post_remove_select);
        }
      }

      UpdateDeleteButton();

      SendDateIntervalBundles();
    }
  }

  void OnComboBoxSelection(wxCommandEvent& event) {
    auto selections = GetSelectionList();

    auto group_number = event.GetSelection();
    auto group_name = date_group_store.GetName(group_number);

    for (size_t index = 0; index < selections.size(); ++index) {
      table_widget->SetValue(group_name, selections[index],
                             ColumnIndex(Columns::group));
    }

    SendDateIntervalBundles();
  }

  wxWeakRef<wxPanel> wx_panel;
  wxWeakRef<wxDataViewListCtrl> table_widget;
  wxWeakRef<wxButton> addRowButton;
  wxWeakRef<wxButton> deleteRowButton;
  wxWeakRef<wxComboBox> select_group_control;

  date_format_descriptor date_format;
  DateGroupStore date_group_store;
  sigslot::signal<const std::vector<DateIntervalBundle>&>
      signal_table_date_interval_bundles;
};
#endif  // HOME_TITAN99_CODE_DECADE_SRC_GUI_DATE_PANEL_HPP
