#ifndef GROUPS_PANEL_HPP
#define GROUPS_PANEL_HPP

#include <wx/dataview.h>
#include <wx/wx.h>

#include <limits>
#include <sigslot/signal.hpp>
#include <string>
#include <utility>
#include <vector>

#include "../domain/date_group.hpp"
#include "table_panel_base.hpp"

class DateGroupsTablePanel : public TablePanelBase {
 public:
  explicit DateGroupsTablePanel(wxWindow* parent)
      : TablePanelBase(parent,
                       wxDV_SINGLE | wxDV_HORIZ_RULES | wxDV_VERT_RULES) {
    BuildTableLayout();

    Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED,
         &DateGroupsTablePanel::CallbackItemActivated, this);
    Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE,
         &DateGroupsTablePanel::CallbackItemEditing, this);
    Bind(wxEVT_DATAVIEW_SELECTION_CHANGED,
         &DateGroupsTablePanel::CallbackSelectionChanged, this);
    Bind(wxEVT_BUTTON, &DateGroupsTablePanel::CallbackButtonClicked, this);

    table()->AppendTextColumn(L"Group Number", wxDATAVIEW_CELL_INERT);
    table()->AppendTextColumn(L"Group Name", wxDATAVIEW_CELL_EDITABLE);
  }

  static std::wstring GetPanelName() { return {L"Date Group Table"}; }

  void ReceiveDateGroups(const std::vector<DateGroup>& argument_date_groups) {
    date_groups_ = argument_date_groups;

    int change_row_count = 0;
    if (date_groups_.size() <= std::numeric_limits<int>::max()) {
      change_row_count =
          static_cast<int>(date_groups_.size()) - table()->GetItemCount();
    } else {
      std::cerr << "too large unsigned int" << '\n';
    }

    if (change_row_count > 0) {
      for (int index = 0; index < change_row_count; ++index) {
        InsertRow(static_cast<std::size_t>(table()->GetItemCount()));
      }
    }

    if (change_row_count < 0) {
      for (int index = 0; index > change_row_count; --index) {
        RemoveRow(static_cast<std::size_t>(table()->GetItemCount()) - 1);
      }
    }

    for (size_t index = 0; std::cmp_less(index, table()->GetItemCount());
         ++index) {
      const auto row = static_cast<unsigned int>(index);
      table()->SetValue(std::to_wstring(date_groups_[index].GetNumber()), row,
                        0);
      table()->SetValue(date_groups_[index].GetName(), row, 1);
    }
  }

  sigslot::signal<const std::vector<DateGroup>&>& SignalTableDateGroups() {
    return signal_table_date_groups_;
  }

 private:
  void UpdateButtons() {
    if (table()->GetSelectedRow() == wxNOT_FOUND ||
        table()->GetSelectedRow() < 1) {
      delete_button()->Enable(false);
    } else {
      delete_button()->Enable(true);
    }
  }
  void InsertRow(size_t row) {
    if (std::cmp_less_equal(row, table()->GetItemCount())) {
      wxVector<wxVariant> empty_row;
      empty_row.resize(table()->GetColumnCount());
      table()->InsertItem(static_cast<unsigned int>(row), empty_row);
    }
  }
  void RemoveRow(size_t row) {
    if (std::cmp_less_equal(row, table()->GetItemCount())) {
      table()->DeleteItem(static_cast<unsigned int>(row));
    }
  }

  void CallbackButtonClicked(wxCommandEvent& event) {
    auto selected_row = table()->GetSelectedRow();

    if (event.GetId() == wxID_ADD) {
      if (selected_row == wxNOT_FOUND) {
        selected_row = table()->GetItemCount();
      } else if (selected_row < 1) {
        selected_row = 1;
      } else {
        ++selected_row;
      }

      const auto unsigned_row = static_cast<unsigned int>(selected_row);
      InsertRow(unsigned_row);

      date_groups_.insert(date_groups_.cbegin() + selected_row, DateGroup(""));

      signal_table_date_groups_(date_groups_);

      table()->SelectRow(unsigned_row);
      table()->EnsureVisible(table()->RowToItem(selected_row));
      UpdateButtons();
    }

    if (event.GetId() == wxID_DELETE && selected_row != wxNOT_FOUND &&
        selected_row >= 1) {
      const auto unsigned_row = static_cast<unsigned int>(selected_row);
      RemoveRow(unsigned_row);

      date_groups_.erase(date_groups_.cbegin() + selected_row);

      signal_table_date_groups_(date_groups_);

      if (table()->GetItemCount() > 0) {
        if (table()->GetItemCount() == selected_row) {
          table()->SelectRow(unsigned_row - 1);
        } else {
          table()->SelectRow(unsigned_row);
        }
      }

      UpdateButtons();
    }
  }

  void CallbackItemActivated(wxDataViewEvent& event) {
    if (event.GetItem().IsOk() && (event.GetDataViewColumn() != nullptr)) {
      table()->EditItem(event.GetItem(), event.GetDataViewColumn());
    }
  }
  void CallbackItemEditing(wxDataViewEvent& event) {
    if (event.GetEventType() == wxEVT_DATAVIEW_ITEM_EDITING_DONE) {
      event.Veto();

      if (!event.IsEditCancelled()) {
        if (event.GetColumn() == 1) {
          auto edited_string = event.GetValue().GetString().ToStdString();
          const auto selected_row =
              static_cast<unsigned int>(table()->GetSelectedRow());
          table()->SetValue(edited_string.c_str(), selected_row,
                            static_cast<unsigned int>(event.GetColumn()));

          date_groups_[selected_row].SetName(edited_string);

          signal_table_date_groups_(date_groups_);
        }
      }
    }
  }

  void CallbackSelectionChanged(wxDataViewEvent& /*event*/) { UpdateButtons(); }

  std::vector<DateGroup> date_groups_;
  sigslot::signal<const std::vector<DateGroup>&> signal_table_date_groups_;
};
#endif  // GROUPS_PANEL_HPP
