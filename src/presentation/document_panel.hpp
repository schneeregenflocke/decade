#ifndef DOCUMENT_PANEL_HPP
#define DOCUMENT_PANEL_HPP

#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <memory>

#include "font_panel.hpp"
#include "page_panel.hpp"
#include "title_panel.hpp"
#include "wx_owned.hpp"

// Presentation: composite tab that groups the page-format, font and title
// settings — all of which configure the overall rendered document — into a
// single notebook page. It owns the three child panels and exposes them so the
// binder can wire each one to its store exactly as before; the child panels and
// their signals are unchanged.
class DocumentSetupPanel : public wxPanel {
 public:
  explicit DocumentSetupPanel(wxWindow* parent) : wxPanel(parent, wxID_ANY) {
    constexpr int kSizerBorderPx = 5;
    const wxSizerFlags group_flags =
        wxSizerFlags().Proportion(0).Expand().Border(wxALL, kSizerBorderPx);

    auto* vertical_sizer = MakeOwned<wxBoxSizer>(wxVERTICAL);
    SetSizer(vertical_sizer);

    page_setup_panel_ = MakeOwned<PageSetupPanel>(this);
    font_panel_ = MakeOwned<FontPanel>(this);
    title_setup_panel_ = MakeOwned<TitleSetupPanel>(this);

    vertical_sizer->Add(WrapInGroup(L"Page", page_setup_panel_), group_flags);
    vertical_sizer->Add(WrapInGroup(L"Font", font_panel_), group_flags);
    vertical_sizer->Add(WrapInGroup(L"Title", title_setup_panel_), group_flags);
  }

  [[nodiscard]] PageSetupPanel* GetPageSetupPanel() const {
    return page_setup_panel_;
  }
  [[nodiscard]] FontPanel* GetFontPanel() const { return font_panel_; }
  [[nodiscard]] TitleSetupPanel* GetTitleSetupPanel() const {
    return title_setup_panel_;
  }

 private:
  wxSizer* WrapInGroup(const wxString& label, wxWindow* panel) {
    auto* box_sizer = MakeOwned<wxStaticBoxSizer>(wxVERTICAL, this, label);
    box_sizer->Add(panel, wxSizerFlags().Proportion(1).Expand());
    return box_sizer;
  }

  wxWeakRef<PageSetupPanel> page_setup_panel_;
  wxWeakRef<FontPanel> font_panel_;
  wxWeakRef<TitleSetupPanel> title_setup_panel_;
};
#endif  // DOCUMENT_PANEL_HPP
