#ifndef LICENSE_PANEL_HPP
#define LICENSE_PANEL_HPP

#include <wx/weakref.h>
#include <wx/wx.h>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Resource.h"

class LicenseInformationDialog : public wxDialog {
 public:
  explicit LicenseInformationDialog(wxWindow* parent)
      : wxDialog(parent, wxID_ANY, L"Open Source Licenses Information",
                 wxDefaultPosition,
                 wxSize(kDefaultWidth, kDefaultHeight) /*wxDefaultSize*/,
                 wxCAPTION | wxRESIZE_BORDER | wxMAXIMIZE_BOX) {
    license_select_list_box =
        std::make_unique<wxListBox>(this, wxID_ANY, wxDefaultPosition,
                                    wxDefaultSize, 0, nullptr,
                                    wxLB_SINGLE | wxLB_NEEDED_SB)
            .release();

    text_view_ctrl = std::make_unique<wxTextCtrl>(
                         this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                         wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY)
                         .release();

    const wxSizerFlags flags0 = wxSizerFlags().Proportion(1).Expand();
    const wxSizerFlags flags1 =
        wxSizerFlags().Proportion(1).Expand().Border(wxALL, kBorderSize);
    const wxSizerFlags flags2 =
        wxSizerFlags().Proportion(0).Expand().Border(wxALL, kBorderSize);
    const wxSizerFlags flags3 =
        wxSizerFlags().Proportion(0).Border(wxALL, kBorderSize).Right();

    auto* vertical_sizer = std::make_unique<wxBoxSizer>(wxVERTICAL).release();
    SetSizer(vertical_sizer);

    auto* horizontal_sizer =
        std::make_unique<wxBoxSizer>(wxHORIZONTAL).release();
    vertical_sizer->Add(horizontal_sizer, flags0);

    horizontal_sizer->Add(license_select_list_box, flags2);
    horizontal_sizer->Add(text_view_ctrl, flags1);

    auto* close_button = std::make_unique<wxButton>(this, wxID_CLOSE).release();

    auto* button_sizer = std::make_unique<wxStdDialogButtonSizer>().release();
    button_sizer->AddButton(close_button);
    button_sizer->Realize();

    vertical_sizer->Add(button_sizer, flags3);

    Bind(wxEVT_LISTBOX, &LicenseInformationDialog::SlotSelectLicense, this);
    Bind(wxEVT_BUTTON, &LicenseInformationDialog::CloseDialog, this);

    CollectLicenses();

    license_select_list_box->Select(0);
    SelectLicense(collected_licenses.begin()->first);
  }

 private:
  static constexpr int kDefaultWidth = 800;
  static constexpr int kDefaultHeight = 600;
  static constexpr int kBorderSize = 10;

  void CollectLicenses() {
    collected_licenses.clear();

    collected_licenses.emplace_back("Decade",
                                    LOAD_RESOURCE(decade_LICENSE).toString());
    collected_licenses.emplace_back(
        "embed-resource",
        LOAD_RESOURCE(embed_resource_LICENSE_LICENSE).toString());
    collected_licenses.emplace_back(
        "csv2", LOAD_RESOURCE(csv2_copyright_LICENSE).toString());
    collected_licenses.emplace_back(
        "csv2mio", LOAD_RESOURCE(csv2mio_LICENSE_LICENSE).toString());
    collected_licenses.emplace_back(
        "sigslot", LOAD_RESOURCE(sigslot_LICENSE_LICENSE).toString());

    for (const auto& license : collected_licenses) {
      license_select_list_box->AppendString(license.first);
    }
  }

  void SlotSelectLicense(wxCommandEvent& event) {
    SelectLicense(event.GetString().ToStdString());
  }
  void CloseDialog(wxCommandEvent& event) {
    (void)event;
    EndModal(0);
  }
  void SelectLicense(const std::string& map_key) {
    auto iter = std::ranges::find_if(
        collected_licenses,
        [&](const string_pair& compare) { return compare.first == map_key; });

    text_view_ctrl->Clear();
    *text_view_ctrl << iter->second;
    text_view_ctrl->ShowPosition(0);
  }

  wxWeakRef<wxListBox> license_select_list_box{nullptr};
  wxWeakRef<wxTextCtrl> text_view_ctrl{nullptr};

  using string_pair = std::pair<std::string, std::string>;

  std::vector<std::pair<std::string, std::string>> collected_licenses;
};
#endif  // LICENSE_PANEL_HPP
