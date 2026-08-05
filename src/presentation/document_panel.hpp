#ifndef DOCUMENT_PANEL_HPP
#define DOCUMENT_PANEL_HPP

#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <memory>
#include <string>

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

    vertical_sizer->Add(WrapInGroup(L"File", CreateFilePathRow(kSizerBorderPx)),
                        group_flags);
    vertical_sizer->Add(WrapInGroup(L"Page", page_setup_panel_), group_flags);
    vertical_sizer->Add(WrapInGroup(L"Font", font_panel_), group_flags);
    vertical_sizer->Add(WrapInGroup(L"Title", title_setup_panel_), group_flags);
  }

  void ReceiveProjectFilePath(const std::string& file_path) {
    file_path_ = file_path;
    const bool has_path = !file_path_.empty();
    file_path_label_->SetLabelText(has_path ? wxString::FromUTF8(file_path_)
                                            : wxString(L"unsaved project"));
    file_path_label_->SetToolTip(has_path ? wxString::FromUTF8(file_path_)
                                          : wxString());
    copy_button_->Enable(has_path);
    Layout();
  }

  [[nodiscard]] PageSetupPanel* GetPageSetupPanel() const {
    return page_setup_panel_;
  }
  [[nodiscard]] FontPanel* GetFontPanel() const { return font_panel_; }
  [[nodiscard]] TitleSetupPanel* GetTitleSetupPanel() const {
    return title_setup_panel_;
  }

 private:
  // A label instead of an input field: the path never gets typed. Copying runs
  // over the button beside it.
  wxSizer* CreateFilePathRow(int border_px) {
    auto* row_sizer = MakeOwned<wxBoxSizer>(wxHORIZONTAL);

    auto* label = MakeOwned<wxStaticText>(
        this, wxID_ANY, L"unsaved project", wxDefaultPosition, wxDefaultSize,
        wxST_ELLIPSIZE_START | wxST_NO_AUTORESIZE);
    // The path must not dictate the column width; the shortening happens at the
    // front, so the file name stays visible.
    label->SetMinSize(wxSize(1, -1));
    file_path_label_ = label;

    auto* copy_button = MakeOwned<wxButton>(this, wxID_ANY, L"Copy");
    copy_button->Enable(false);
    copy_button->Bind(wxEVT_BUTTON,
                      [this](wxCommandEvent&) { CopyFilePathToClipboard(); });
    copy_button_ = copy_button;

    row_sizer->Add(label, wxSizerFlags().Proportion(1).CenterVertical().Border(
                              wxALL, border_px));
    row_sizer->Add(copy_button,
                   wxSizerFlags().Proportion(0).Border(wxALL, border_px));
    return row_sizer;
  }

  void CopyFilePathToClipboard() const {
    if (file_path_.empty() || !wxTheClipboard->Open()) {
      return;
    }
    wxTheClipboard->SetData(
        MakeOwned<wxTextDataObject>(wxString::FromUTF8(file_path_)));
    wxTheClipboard->Close();
  }

  wxSizer* WrapInGroup(const wxString& label, wxWindow* panel) {
    auto* box_sizer = MakeOwned<wxStaticBoxSizer>(wxVERTICAL, this, label);
    box_sizer->Add(panel, wxSizerFlags().Proportion(1).Expand());
    return box_sizer;
  }

  wxSizer* WrapInGroup(const wxString& label, wxSizer* content) {
    auto* box_sizer = MakeOwned<wxStaticBoxSizer>(wxVERTICAL, this, label);
    box_sizer->Add(content, wxSizerFlags().Proportion(1).Expand());
    return box_sizer;
  }

  std::string file_path_;
  wxWeakRef<wxStaticText> file_path_label_;
  wxWeakRef<wxButton> copy_button_;
  wxWeakRef<PageSetupPanel> page_setup_panel_;
  wxWeakRef<FontPanel> font_panel_;
  wxWeakRef<TitleSetupPanel> title_setup_panel_;
};
#endif  // DOCUMENT_PANEL_HPP
