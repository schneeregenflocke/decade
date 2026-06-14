#ifndef GROUPS_PANEL_HPP
#define GROUPS_PANEL_HPP

#include <wx/dataview.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <limits>
#include <memory>
#include <sigslot/signal.hpp>
#include <string>
#include <utility>
#include <vector>

#include "../packages/date_group.hpp"

class DateGroupsTablePanel : public wxPanel {
 public:
  explicit DateGroupsTablePanel(wxWindow* parent)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTAB_TRAVERSAL, wxPanelNameStr) {
    data_table_ = std::make_unique<wxDataViewListCtrl>(
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

    add_row_button_ =
        std::make_unique<wxButton>(this, wxID_ADD, "Add Row").release();
    delete_row_button_ =
        std::make_unique<wxButton>(this, wxID_DELETE, "Delete Row").release();
    delete_row_button_->Disable();

    Bind(wxEVT_BUTTON, &DateGroupsTablePanel::CallbackButtonClicked, this);

    wxBoxSizer* buttons_sizer =
        std::make_unique<wxBoxSizer>(wxHORIZONTAL).release();
    wxSizerFlags const buttons_flags =
        wxSizerFlags().Proportion(0).Border(wxALL, 5);
    buttons_sizer->Add(add_row_button_, buttons_flags);
    buttons_sizer->Add(delete_row_button_, buttons_flags);

    wxBoxSizer* table_sizer =
        std::make_unique<wxBoxSizer>(wxHORIZONTAL).release();
    wxSizerFlags const data_table_flags =
        wxSizerFlags().Proportion(1).Expand().Border(wxALL, 5);
    table_sizer->Add(data_table_, data_table_flags);

    wxBoxSizer* main_sizer = std::make_unique<wxBoxSizer>(wxVERTICAL).release();
    wxSizerFlags const buttons_sizer_flags =
        wxSizerFlags().Proportion(0).Expand().Border(wxALL, 0);
    wxSizerFlags const table_sizer_flags =
        wxSizerFlags().Proportion(1).Expand().Border(wxALL, 0);
    main_sizer->Add(buttons_sizer, buttons_sizer_flags);
    main_sizer->Add(table_sizer, table_sizer_flags);

    SetSizer(main_sizer);

    data_table_->AppendTextColumn(L"Group Number", wxDATAVIEW_CELL_INERT);
    data_table_->AppendTextColumn(L"Group Name", wxDATAVIEW_CELL_EDITABLE);
  }

  static std::wstring GetPanelName() { return {L"Date Group Table"}; }

  void ReceiveDateGroups(const std::vector<DateGroup>& argument_date_groups) {
    date_groups_ = argument_date_groups;

    int change_row_count = 0;
    if (date_groups_.size() <= std::numeric_limits<int>::max()) {
      change_row_count =
          static_cast<int>(date_groups_.size()) - data_table_->GetItemCount();
    } else {
      std::cerr << "too large unsigned int" << '\n';
    }

    if (change_row_count > 0) {
      for (int index = 0; index < change_row_count; ++index) {
        InsertRow(static_cast<std::size_t>(data_table_->GetItemCount()));
      }
    }

    if (change_row_count < 0) {
      for (int index = 0; index > change_row_count; --index) {
        RemoveRow(static_cast<std::size_t>(data_table_->GetItemCount()) - 1);
      }
    }

    for (size_t index = 0; std::cmp_less(index, data_table_->GetItemCount());
         ++index) {
      const auto row = static_cast<unsigned int>(index);
      data_table_->SetValue(std::to_wstring(date_groups_[index].GetNumber()),
                            row, 0);
      data_table_->SetValue(date_groups_[index].GetName(), row, 1);
    }
  }

  sigslot::signal<const std::vector<DateGroup>&>& SignalTableDateGroups() {
    return signal_table_date_groups_;
  }

 private:
  void UpdateButtons() {
    if (data_table_->GetSelectedRow() == wxNOT_FOUND ||
        data_table_->GetSelectedRow() < 1) {
      delete_row_button_->Enable(false);
    } else {
      delete_row_button_->Enable(true);
    }
  }
  void InsertRow(size_t row) {
    if (std::cmp_less_equal(row, data_table_->GetItemCount())) {
      wxVector<wxVariant> empty_row;
      empty_row.resize(data_table_->GetColumnCount());
      data_table_->InsertItem(static_cast<unsigned int>(row), empty_row);
    }
  }
  void RemoveRow(size_t row) {
    if (std::cmp_less_equal(row, data_table_->GetItemCount())) {
      data_table_->DeleteItem(static_cast<unsigned int>(row));
    }
  }

  void CallbackButtonClicked(wxCommandEvent& event) {
    auto selected_row = data_table_->GetSelectedRow();

    if (event.GetId() == wxID_ADD) {
      if (selected_row == wxNOT_FOUND) {
        selected_row = data_table_->GetItemCount();
      } else if (selected_row < 1) {
        selected_row = 1;
      } else {
        ++selected_row;
      }

      const auto unsigned_row = static_cast<unsigned int>(selected_row);
      InsertRow(unsigned_row);

      date_groups_.insert(date_groups_.cbegin() + selected_row, DateGroup(""));

      signal_table_date_groups_(date_groups_);

      data_table_->SelectRow(unsigned_row);
      data_table_->EnsureVisible(data_table_->RowToItem(selected_row));
      UpdateButtons();
    }

    if (event.GetId() == wxID_DELETE && selected_row != wxNOT_FOUND &&
        selected_row >= 1) {
      const auto unsigned_row = static_cast<unsigned int>(selected_row);
      RemoveRow(unsigned_row);

      date_groups_.erase(date_groups_.cbegin() + selected_row);

      signal_table_date_groups_(date_groups_);

      if (data_table_->GetItemCount() > 0) {
        if (data_table_->GetItemCount() == selected_row) {
          data_table_->SelectRow(unsigned_row - 1);
        } else {
          data_table_->SelectRow(unsigned_row);
        }
      }

      UpdateButtons();
    }
  }

  void CallbackItemActivated(wxDataViewEvent& event) {
    if (event.GetItem().IsOk() && (event.GetDataViewColumn() != nullptr)) {
      data_table_->EditItem(event.GetItem(), event.GetDataViewColumn());
    }
  }
  void CallbackItemEditing(wxDataViewEvent& event) {
    if (event.GetEventType() == wxEVT_DATAVIEW_ITEM_EDITING_DONE) {
      event.Veto();

      if (!event.IsEditCancelled()) {
        if (event.GetColumn() == 1) {
          auto edited_string = event.GetValue().GetString().ToStdString();
          const auto selected_row =
              static_cast<unsigned int>(data_table_->GetSelectedRow());
          data_table_->SetValue(edited_string.c_str(), selected_row,
                                static_cast<unsigned int>(event.GetColumn()));

          date_groups_[selected_row].SetName(edited_string);

          signal_table_date_groups_(date_groups_);
        }
      }
    }
  }

  void CallbackSelectionChanged(wxDataViewEvent& /*event*/) { UpdateButtons(); }

  wxWeakRef<wxDataViewListCtrl> data_table_;
  wxWeakRef<wxButton> add_row_button_;
  wxWeakRef<wxButton> delete_row_button_;

  std::vector<DateGroup> date_groups_;
  sigslot::signal<const std::vector<DateGroup>&> signal_table_date_groups_;
};
#endif  // GROUPS_PANEL_HPP
