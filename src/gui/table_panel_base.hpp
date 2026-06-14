#ifndef TABLE_PANEL_BASE_HPP
#define TABLE_PANEL_BASE_HPP

#include <wx/dataview.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <vector>

#include "wx_owned.hpp"

// Common scaffolding shared by the two data-table panels (date entries and date
// groups): a wxDataViewListCtrl sitting above an "Add Row" / "Delete Row"
// button row. The base owns the three widgets and lays them out; subclasses
// choose the table style and columns, fill the rows and decide what Add/Delete
// actually do (single vs. multi selection, what a new/removed row means) by
// binding their own handlers to `add_button_` / `delete_button_`.
class TablePanelBase : public wxPanel {
 public:
  explicit TablePanelBase(wxWindow* parent, long table_style)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTAB_TRAVERSAL, wxPanelNameStr) {
    table_ = MakeOwned<wxDataViewListCtrl>(this, wxID_ANY, wxDefaultPosition,
                                           wxDefaultSize, table_style,
                                           wxDefaultValidator);
    add_button_ = MakeOwned<wxButton>(this, wxID_ADD, "Add Row");
    delete_button_ = MakeOwned<wxButton>(this, wxID_DELETE, "Delete Row");
    delete_button_->Disable();
  }

 protected:
  // Widgets the base owns, exposed to subclasses through accessors (the data
  // members themselves stay private). Each returns a non-owning pointer that
  // auto-nulls if wx destroys the widget.
  [[nodiscard]] wxDataViewListCtrl* table() const { return table_; }
  [[nodiscard]] wxButton* delete_button() const { return delete_button_; }

  // Lays out the button row (Add, Delete, then any subclass-specific controls)
  // above the table and installs the sizer. Call once from the subclass
  // constructor after creating the extra controls.
  void BuildTableLayout(
      const std::vector<wxWindow*>& extra_button_controls = {}) {
    constexpr int kBorder = 5;

    auto* buttons_sizer = MakeOwned<wxBoxSizer>(wxHORIZONTAL);
    const wxSizerFlags buttons_flags =
        wxSizerFlags().Proportion(0).Border(wxALL, kBorder);
    buttons_sizer->Add(add_button_, buttons_flags);
    buttons_sizer->Add(delete_button_, buttons_flags);
    for (wxWindow* control : extra_button_controls) {
      buttons_sizer->Add(control, buttons_flags);
    }

    auto* table_sizer = MakeOwned<wxBoxSizer>(wxHORIZONTAL);
    table_sizer->Add(
        table_, wxSizerFlags().Proportion(1).Expand().Border(wxALL, kBorder));

    auto* main_sizer = MakeOwned<wxBoxSizer>(wxVERTICAL);
    main_sizer->Add(buttons_sizer, wxSizerFlags().Proportion(0).Expand());
    main_sizer->Add(table_sizer, wxSizerFlags().Proportion(1).Expand());
    SetSizer(main_sizer);
  }

 private:
  wxWeakRef<wxDataViewListCtrl> table_;
  wxWeakRef<wxButton> add_button_;
  wxWeakRef<wxButton> delete_button_;
};

#endif  // TABLE_PANEL_BASE_HPP
