#ifndef SHAPE_PANEL_HPP
#define SHAPE_PANEL_HPP

#include <wx/clrpicker.h>
#include <wx/listbox.h>
#include <wx/spinctrl.h>
#include <wx/splitter.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <glm/vec4.hpp>
#include <sigslot/signal.hpp>
#include <string>
#include <vector>

#include "../domain/detail/reentry_guard.hpp"
#include "../domain/shape_configuration.hpp"
#include "casts.hpp"
#include "wx_owned.hpp"

// The home of the shape configurations: the list on the left names them, the
// fields on the right edit the selected one. The scene tree mirrors the same
// values and stays read-only — this is where they get changed (#65).
//
// Master and detail rather than one long form, because the set grows with the
// date groups: the fixed configurations plus one entry per group.
class ShapeSetupPanel : public wxPanel {
 public:
  explicit ShapeSetupPanel(wxWindow* parent) : wxPanel(parent, wxID_ANY) {
    auto* splitter = MakeOwned<wxSplitterWindow>(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D);
    splitter->SetMinimumPaneSize(kMinPanePx);

    name_list_ = MakeOwned<wxListBox>(splitter, wxID_ANY);
    auto* detail_panel = MakeOwned<wxPanel>(splitter, wxID_ANY);
    CreateDetailFields(detail_panel);

    splitter->SplitVertically(name_list_, detail_panel, kSashPositionPx);

    auto* vertical_sizer = MakeOwned<wxBoxSizer>(wxVERTICAL);
    vertical_sizer->Add(splitter, 1, wxEXPAND | wxALL, kSizerBorderPx);
    SetSizer(vertical_sizer);

    name_list_->Bind(wxEVT_LISTBOX, &ShapeSetupPanel::CallbackSelection, this);
    Bind(wxEVT_CHECKBOX, &ShapeSetupPanel::CallbackEdit, this);
    Bind(wxEVT_COLOURPICKER_CHANGED, &ShapeSetupPanel::CallbackEdit, this);
    Bind(wxEVT_SLIDER, &ShapeSetupPanel::CallbackEdit, this);
    Bind(wxEVT_SPINCTRLDOUBLE, &ShapeSetupPanel::CallbackEdit, this);
  }

  // The set arrives whole and replaces what stands here. The selection follows
  // the name, not the row: a new date group adds an entry and would otherwise
  // shift the selection to a different configuration.
  void ReceiveShapeConfigSet(const ShapeConfigSet& shape_config_set) {
    if (editing_) {
      return;
    }
    shape_config_set_ = shape_config_set;
    RebuildNameList();
    RefreshDetail();
  }

  sigslot::signal<const ShapeConfigSet&>& SignalShapeConfigSet() {
    return signal_shape_config_set_;
  }

 private:
  // The colour picker carries no alpha channel, so alpha rides on a slider of
  // its own, byte-valued like the wx colour.
  static constexpr float kAlphaByteMax = 255.0F;
  static constexpr int kAlphaSliderMax = 255;
  static constexpr int kSashPositionPx = 180;
  static constexpr int kMinPanePx = 80;
  static constexpr int kSizerBorderPx = 5;
  static constexpr int kLabelWidthPx = 120;
  static constexpr double kLineWidthMinMm = 0.0;
  static constexpr double kLineWidthMaxMm = 20.0;
  static constexpr double kLineWidthIncrementMm = 0.1;

  void CreateDetailFields(wxPanel* detail_panel) {
    const wxSizerFlags row_flags = wxSizerFlags().Proportion(0).Expand();
    const wxSizerFlags label_flags =
        wxSizerFlags().Proportion(0).Expand().Border(wxALL, kSizerBorderPx);
    const wxSizerFlags field_flags =
        wxSizerFlags().Proportion(1).CenterVertical().Border(wxALL,
                                                             kSizerBorderPx);

    auto* vertical_sizer = MakeOwned<wxBoxSizer>(wxVERTICAL);
    detail_panel->SetSizer(vertical_sizer);

    outline_visible_ = MakeOwned<wxCheckBox>(detail_panel, wxID_ANY, "");
    AddLabelledRow(detail_panel, vertical_sizer, L"Outline Visible",
                   outline_visible_, row_flags, label_flags, field_flags);

    outline_color_ = MakeOwned<wxColourPickerCtrl>(detail_panel, wxID_ANY);
    AddLabelledRow(detail_panel, vertical_sizer, L"Outline Color",
                   outline_color_, row_flags, label_flags, field_flags);

    outline_alpha_ = MakeAlphaSlider(detail_panel);
    AddLabelledRow(detail_panel, vertical_sizer, L"Outline Transparency",
                   outline_alpha_, row_flags, label_flags, field_flags);

    fill_visible_ = MakeOwned<wxCheckBox>(detail_panel, wxID_ANY, "");
    AddLabelledRow(detail_panel, vertical_sizer, L"Fill Visible", fill_visible_,
                   row_flags, label_flags, field_flags);

    fill_color_ = MakeOwned<wxColourPickerCtrl>(detail_panel, wxID_ANY);
    AddLabelledRow(detail_panel, vertical_sizer, L"Fill Color", fill_color_,
                   row_flags, label_flags, field_flags);

    fill_alpha_ = MakeAlphaSlider(detail_panel);
    AddLabelledRow(detail_panel, vertical_sizer, L"Fill Transparency",
                   fill_alpha_, row_flags, label_flags, field_flags);

    line_width_ = MakeOwned<wxSpinCtrlDouble>(detail_panel);
    line_width_->SetDigits(2);
    line_width_->SetRange(kLineWidthMinMm, kLineWidthMaxMm);
    line_width_->SetIncrement(kLineWidthIncrementMm);
    AddLabelledRow(detail_panel, vertical_sizer, L"Line Width (mm)",
                   line_width_, row_flags, label_flags, field_flags);

    vertical_sizer->Layout();
  }

  static wxSlider* MakeAlphaSlider(wxPanel* detail_panel) {
    return MakeOwned<wxSlider>(detail_panel, wxID_ANY, kAlphaSliderMax, 0,
                               kAlphaSliderMax, wxDefaultPosition,
                               wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
  }

  static void AddLabelledRow(wxPanel* detail_panel, wxSizer* parent_sizer,
                             const wxString& text, wxWindow* field,
                             const wxSizerFlags& row_flags,
                             const wxSizerFlags& label_flags,
                             const wxSizerFlags& field_flags) {
    auto* row_sizer = MakeOwned<wxBoxSizer>(wxHORIZONTAL);
    auto* label = MakeOwned<wxStaticText>(detail_panel, wxID_ANY, text);
    label->SetMinSize(wxSize(kLabelWidthPx, -1));
    row_sizer->Add(label, label_flags);
    row_sizer->Add(field, field_flags);
    parent_sizer->Add(row_sizer, row_flags);
  }

  // Every configuration of the set in one list: the fixed ones first, the
  // per-date-group ones after them, in the order the set holds them.
  [[nodiscard]] std::vector<std::string> ConfigurationNames() const {
    std::vector<std::string> names;
    names.reserve(shape_config_set_.FixedConfigurations().size() +
                  shape_config_set_.GroupConfigurations().size());
    for (const auto& config : shape_config_set_.FixedConfigurations()) {
      names.push_back(config.Name());
    }
    for (const auto& config : shape_config_set_.GroupConfigurations()) {
      names.push_back(config.Name());
    }
    return names;
  }

  void RebuildNameList() {
    const std::vector<std::string> names = ConfigurationNames();

    wxArrayString items;
    for (const std::string& name : names) {
      items.Add(wxString::FromUTF8(name));
    }
    name_list_->Set(items);

    if (selected_name_.empty() && !names.empty()) {
      selected_name_ = names.front();
    }
    const int row = RowOf(names, selected_name_);
    if (row < 0) {
      selected_name_ = names.empty() ? std::string{} : names.front();
      name_list_->SetSelection(names.empty() ? wxNOT_FOUND : 0);
      return;
    }
    name_list_->SetSelection(row);
  }

  static int RowOf(const std::vector<std::string>& names,
                   const std::string& name) {
    for (std::size_t index = 0; index < names.size(); ++index) {
      if (names[index] == name) {
        return static_cast<int>(index);
      }
    }
    return -1;
  }

  // The fields show the configured values, not the ones visibility cleans up:
  // switching a fill off and on again must give back the colour that was set.
  void RefreshDetail() {
    const ShapeConfiguration config =
        shape_config_set_.GetShapeConfiguration(selected_name_);
    const bool has_selection = config.Name() == selected_name_;

    outline_visible_->Enable(has_selection);
    outline_color_->Enable(has_selection);
    outline_alpha_->Enable(has_selection);
    fill_visible_->Enable(has_selection);
    fill_color_->Enable(has_selection);
    fill_alpha_->Enable(has_selection);
    line_width_->Enable(has_selection);
    if (!has_selection) {
      return;
    }

    outline_visible_->SetValue(config.OutlineVisible());
    ShowColor(config.OutlineColorDisabled(), outline_color_, outline_alpha_);
    fill_visible_->SetValue(config.FillVisible());
    ShowColor(config.FillColorDisabled(), fill_color_, fill_alpha_);
    line_width_->SetValue(static_cast<double>(config.LineWidthDisabled()));
  }

  static void ShowColor(const glm::vec4& color,
                        const wxWeakRef<wxColourPickerCtrl>& picker,
                        const wxWeakRef<wxSlider>& alpha) {
    const wxColour wx_color = ToWxColor(color);
    picker->SetColour(wx_color);
    alpha->SetValue(wx_color.Alpha());
  }

  static glm::vec4 ReadColor(const wxWeakRef<wxColourPickerCtrl>& picker,
                             const wxWeakRef<wxSlider>& alpha) {
    glm::vec4 color = ToGlmVec4(picker->GetColour());
    color[3] = static_cast<float>(alpha->GetValue()) / kAlphaByteMax;
    return color;
  }

  void CallbackSelection(wxCommandEvent& /*event*/) {
    const int row = name_list_->GetSelection();
    if (row == wxNOT_FOUND) {
      return;
    }
    selected_name_ = name_list_->GetString(static_cast<unsigned>(row)).ToStdString();
    RefreshDetail();
  }

  // One handler for every field: the configuration gets rebuilt from the
  // widgets as a whole, so no field needs a branch of its own.
  void CallbackEdit(wxCommandEvent& /*event*/) {
    if (selected_name_.empty()) {
      return;
    }
    const ShapeConfiguration edited{
        selected_name_,
        outline_visible_->GetValue(),
        fill_visible_->GetValue(),
        static_cast<float>(line_width_->GetValue()),
        ShapeConfiguration::OutlineColorValue{
            ReadColor(outline_color_, outline_alpha_)},
        ShapeConfiguration::FillColorValue{ReadColor(fill_color_, fill_alpha_)}};
    if (!shape_config_set_.UpdateConfiguration(edited)) {
      return;
    }
    // The set comes back over the bus while this call still runs; the guard
    // keeps that echo from rebuilding the list under the user's hands.
    const domain::detail::ScopedReentryFlag guard(editing_);
    signal_shape_config_set_(shape_config_set_);
  }

  ShapeConfigSet shape_config_set_;
  std::string selected_name_;
  sigslot::signal<const ShapeConfigSet&> signal_shape_config_set_;

  wxWeakRef<wxListBox> name_list_;
  wxWeakRef<wxCheckBox> outline_visible_;
  wxWeakRef<wxColourPickerCtrl> outline_color_;
  wxWeakRef<wxSlider> outline_alpha_;
  wxWeakRef<wxCheckBox> fill_visible_;
  wxWeakRef<wxColourPickerCtrl> fill_color_;
  wxWeakRef<wxSlider> fill_alpha_;
  wxWeakRef<wxSpinCtrlDouble> line_width_;

  bool editing_{false};
};
#endif  // SHAPE_PANEL_HPP
