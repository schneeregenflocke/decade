#ifndef SHAPE_PANEL_HPP
#define SHAPE_PANEL_HPP

#include <wx/clrpicker.h>
#include <wx/spinctrl.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <algorithm>
#include <memory>
#include <sigslot/signal.hpp>
#include <string>
#include <vector>

#include "../packages/shape_configuration.hpp"
#include "casts.hpp"
#include "wx_owned.hpp"

class ElementsSetupsPanel : public wxPanel {
 public:
  explicit ElementsSetupsPanel(wxWindow* parent)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTAB_TRAVERSAL, wxEmptyString) {
    InitWidgets();
    InitSizers();

    Bind(wxEVT_LISTBOX, &ElementsSetupsPanel::CallbackListBook, this);
    Bind(wxEVT_CHECKBOX, &ElementsSetupsPanel::CallbackCheckBox, this);
    Bind(wxEVT_SPINCTRLDOUBLE, &ElementsSetupsPanel::CallbackSpinControlDouble,
         this);
    Bind(wxEVT_COLOURPICKER_CHANGED, &ElementsSetupsPanel::CallbackColorPicker,
         this);
    Bind(wxEVT_SLIDER, &ElementsSetupsPanel::CallbackSlider, this);
  }

  void ReceiveShapeConfigSet(const ShapeConfigSet& incoming_shape_config_set) {
    shape_config_set_ = incoming_shape_config_set;
    UpdateConfigurationList();
  }

  sigslot::signal<const ShapeConfigSet&>& SignalShapeConfigSet() {
    return signal_shape_config_set_;
  }

 private:
  sigslot::signal<const ShapeConfigSet&> signal_shape_config_set_;

  void InitWidgets() {
    constexpr int kAlphaMax = 100;
    constexpr int kAlphaMin = 0;
    constexpr int kDefaultLabelWidth = 150;
    constexpr double kLineWidthMin = 0.0;
    constexpr double kLineWidthMax = 10.0;
    constexpr double kLineWidthIncrement = 0.05;

    shape_configuration_list_box_ = MakeOwned<wxListBox>(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr,
        wxLB_SINGLE | wxLB_NEEDED_SB, wxDefaultValidator, wxEmptyString);

    outline_visible_ctrl_ =
        MakeOwned<wxCheckBox>(this, wxID_ANY, L"Outline Visible");
    fill_visible_ctrl_ = MakeOwned<wxCheckBox>(this, wxID_ANY, L"Fill Visible");

    line_color_picker_ = MakeOwned<wxColourPickerCtrl>(
        this, wxID_ANY, *wxStockGDI::GetColour(wxStockGDI::COLOUR_BLACK),
        wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_ALPHA);
    fill_color_picker_ = MakeOwned<wxColourPickerCtrl>(
        this, wxID_ANY, *wxStockGDI::GetColour(wxStockGDI::COLOUR_BLACK),
        wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_ALPHA);

    linewidth_ctrl_ = MakeOwned<wxSpinCtrlDouble>(
        this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
        /*16384L*/ wxSP_ARROW_KEYS | wxALIGN_RIGHT);
    linewidth_ctrl_->SetRange(kLineWidthMin, kLineWidthMax);
    linewidth_ctrl_->SetDigits(2);
    linewidth_ctrl_->SetIncrement(kLineWidthIncrement);

    const wxSize default_label_size(kDefaultLabelWidth, -1);
    linewidth_label_ = MakeOwned<wxStaticText>(
        this, wxID_ANY, "Line Width", wxDefaultPosition, default_label_size);
    linecolor_label_ = MakeOwned<wxStaticText>(
        this, wxID_ANY, "Line Color", wxDefaultPosition, default_label_size);
    fillcolor_label_ = MakeOwned<wxStaticText>(
        this, wxID_ANY, "Fill Color", wxDefaultPosition, default_label_size);
    line_transparency_label_ = MakeOwned<wxStaticText>(
        this, wxID_ANY, "Transparency", wxDefaultPosition, default_label_size);
    fill_transparency_label_ = MakeOwned<wxStaticText>(
        this, wxID_ANY, "Transparency", wxDefaultPosition, default_label_size);

    line_color_alpha_slider_ =
        MakeOwned<wxSlider>(this, wxID_ANY, kAlphaMin, kAlphaMin, kAlphaMax,
                            wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
    fill_color_alpha_slider_ =
        MakeOwned<wxSlider>(this, wxID_ANY, kAlphaMin, kAlphaMin, kAlphaMax,
                            wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);

    line_color_alpha_slider_->SetMax(kAlphaMax);
    fill_color_alpha_slider_->SetMax(kAlphaMax);
  }

  void InitSizers() {
    constexpr int kDefaultSizerBorder = 5;
    // [0] sub-sizers/static boxes, [1] labels, [2] widgets that should grow
    // vertically (the elements box and its list), [3] fixed-height fields in a
    // horizontal row (spin/colour/slider/checkbox): these must NOT be expanded
    // on the cross (vertical) axis or GTK allocates them negative heights.
    std::array<wxSizerFlags, 4> sizer_flags;
    sizer_flags[0].Proportion(0).Expand();
    sizer_flags[1].Proportion(0).CenterVertical().Border(wxALL,
                                                         kDefaultSizerBorder);
    sizer_flags[2].Proportion(1).Expand().Border(wxALL, kDefaultSizerBorder);
    sizer_flags[3].Proportion(1).CenterVertical().Border(wxALL,
                                                         kDefaultSizerBorder);

    auto* vertical_sizer = MakeOwned<wxBoxSizer>(wxVERTICAL);
    SetSizer(vertical_sizer);

    auto* static_box_sizer_elements =
        MakeOwned<wxStaticBoxSizer>(wxVERTICAL, this, "Elements");
    auto* static_box_sizer_outline =
        MakeOwned<wxStaticBoxSizer>(wxVERTICAL, this, "Outline");
    auto* static_box_sizer_fill =
        MakeOwned<wxStaticBoxSizer>(wxVERTICAL, this, "Fill");

    vertical_sizer->Add(static_box_sizer_elements, sizer_flags[2]);
    vertical_sizer->Add(static_box_sizer_outline, sizer_flags[0]);
    vertical_sizer->Add(static_box_sizer_fill, sizer_flags[0]);

    std::array<wxBoxSizer*, 4> horizontal_sizers_outline{};
    for (auto& horizontal_sizer : horizontal_sizers_outline) {
      horizontal_sizer = MakeOwned<wxBoxSizer>(wxHORIZONTAL);
      static_box_sizer_outline->Add(horizontal_sizer, sizer_flags[0]);
    }

    std::array<wxBoxSizer*, 3> horizontal_sizers_fill{};
    for (auto& horizontal_sizer : horizontal_sizers_fill) {
      horizontal_sizer = MakeOwned<wxBoxSizer>(wxHORIZONTAL);
      static_box_sizer_fill->Add(horizontal_sizer, sizer_flags[0]);
    }

    static_box_sizer_elements->Add(shape_configuration_list_box_,
                                   sizer_flags[2]);

    horizontal_sizers_outline[0]->Add(outline_visible_ctrl_, sizer_flags[3]);
    horizontal_sizers_outline[1]->Add(linewidth_label_, sizer_flags[1]);
    horizontal_sizers_outline[1]->Add(linewidth_ctrl_, sizer_flags[3]);
    horizontal_sizers_outline[2]->Add(linecolor_label_, sizer_flags[1]);
    horizontal_sizers_outline[2]->Add(line_color_picker_, sizer_flags[3]);
    horizontal_sizers_outline[3]->Add(line_transparency_label_, sizer_flags[1]);
    horizontal_sizers_outline[3]->Add(line_color_alpha_slider_, sizer_flags[3]);

    horizontal_sizers_fill[0]->Add(fill_visible_ctrl_, sizer_flags[3]);
    horizontal_sizers_fill[1]->Add(fillcolor_label_, sizer_flags[1]);
    horizontal_sizers_fill[1]->Add(fill_color_picker_, sizer_flags[3]);
    horizontal_sizers_fill[2]->Add(fill_transparency_label_, sizer_flags[1]);
    horizontal_sizers_fill[2]->Add(fill_color_alpha_slider_, sizer_flags[3]);

    vertical_sizer->Layout();
  }

  void UpdateConfigurationList() {
    shape_configuration_list_box_->Clear();

    for (size_t index = 0; index < shape_config_set_.size(); ++index) {
      shape_configuration_list_box_->AppendString(
          shape_config_set_[index].Name());
    }

    const int number_of_items =
        static_cast<int>(shape_configuration_list_box_->GetCount());
    if (number_of_items <= 0) {
      return;
    }

    int selection = shape_configuration_list_box_->GetSelection();
    if (selection == wxNOT_FOUND) {
      selection = 0;
    }
    selection = std::clamp(selection, 0, number_of_items - 1);
    shape_configuration_list_box_->Select(selection);
    UpdateWidgetForSelection(static_cast<size_t>(selection));
  }

  void UpdateWidgetForSelection(size_t selection) {
    if (selection >= shape_config_set_.size()) {
      return;
    }

    auto& current_configuration = shape_config_set_[selection];

    auto outline_visible = current_configuration.OutlineVisible();
    outline_visible_ctrl_->SetValue(outline_visible);
    linewidth_ctrl_->Enable(outline_visible);
    line_color_picker_->Enable(outline_visible);
    line_color_alpha_slider_->Enable(outline_visible);

    auto fill_visible = current_configuration.FillVisible();
    fill_visible_ctrl_->SetValue(fill_visible);
    fill_color_picker_->Enable(fill_visible);
    fill_color_alpha_slider_->Enable(fill_visible);

    linewidth_ctrl_->SetValue(
        static_cast<double>(current_configuration.LineWidthDisabled()));

    constexpr float kAlphaScale = 100.0F;
    const auto outline_color = current_configuration.OutlineColorDisabled();
    const auto fill_color = current_configuration.FillColorDisabled();

    line_color_picker_->SetColour(ToWxColor(outline_color));
    line_color_alpha_slider_->SetValue(
        static_cast<int>(kAlphaScale - (outline_color[3] * kAlphaScale)));

    fill_color_picker_->SetColour(ToWxColor(fill_color));
    fill_color_alpha_slider_->SetValue(
        static_cast<int>(kAlphaScale - (fill_color[3] * kAlphaScale)));
  }

  // Runs `mutate` on the currently selected configuration (passing the config
  // and its index) and re-publishes the set. No-op when nothing is selected.
  // Concentrates the "fetch selection, bail if none, emit afterwards" boiler-
  // plate every editing callback shares.
  template <typename Mutator>
  void EditSelectedConfiguration(Mutator mutate) {
    const int selection_index = shape_configuration_list_box_->GetSelection();
    if (selection_index == wxNOT_FOUND) {
      return;
    }
    const auto selection = static_cast<size_t>(selection_index);
    mutate(shape_config_set_[selection], selection);
    signal_shape_config_set_(shape_config_set_);
  }

  void CallbackListBook(wxCommandEvent& event) {
    const int selection_index = event.GetSelection();
    if (selection_index == wxNOT_FOUND) {
      return;
    }
    UpdateWidgetForSelection(static_cast<size_t>(selection_index));
  }

  void CallbackCheckBox(wxCommandEvent& event) {
    const bool check_status = event.IsChecked();
    EditSelectedConfiguration(
        [&](ShapeConfiguration& config, size_t selection) {
          if (event.GetEventObject() == outline_visible_ctrl_.get()) {
            config.OutlineVisible(check_status);
          }
          if (event.GetEventObject() == fill_visible_ctrl_.get()) {
            config.FillVisible(check_status);
          }
          UpdateWidgetForSelection(selection);
        });
  }

  void CallbackSpinControlDouble(wxSpinDoubleEvent& event) {
    const auto line_width = static_cast<float>(event.GetValue());
    EditSelectedConfiguration([&](ShapeConfiguration& config, size_t /*sel*/) {
      config.LineWidth(line_width);
    });
  }

  void CallbackColorPicker(wxColourPickerEvent& event) {
    auto color = ToGlmVec4(event.GetColour());
    EditSelectedConfiguration([&](ShapeConfiguration& config, size_t /*sel*/) {
      // Keep the existing alpha; the colour picker only edits RGB.
      if (event.GetEventObject() == line_color_picker_.get()) {
        color[3] = config.OutlineColor()[3];
        config.OutlineColor(color);
      }
      if (event.GetEventObject() == fill_color_picker_.get()) {
        color[3] = config.FillColor()[3];
        config.FillColor(color);
      }
    });
  }

  void CallbackSlider(wxCommandEvent& event) {
    constexpr float kAlphaScale = 100.0F;
    const auto slider_value = static_cast<float>(event.GetInt());
    const float alpha_value = 1.0F - (slider_value / kAlphaScale);

    EditSelectedConfiguration([&](ShapeConfiguration& config, size_t /*sel*/) {
      if (event.GetEventObject() == line_color_alpha_slider_.get()) {
        auto current = config.OutlineColor();
        current[3] = alpha_value;
        config.OutlineColor(current);
      }
      if (event.GetEventObject() == fill_color_alpha_slider_.get()) {
        auto current = config.FillColor();
        current[3] = alpha_value;
        config.FillColor(current);
      }
    });
  }

  ShapeConfigSet shape_config_set_;

  wxWeakRef<wxListBox> shape_configuration_list_box_{nullptr};
  wxWeakRef<wxCheckBox> outline_visible_ctrl_{nullptr};
  wxWeakRef<wxCheckBox> fill_visible_ctrl_{nullptr};
  wxWeakRef<wxSpinCtrlDouble> linewidth_ctrl_{nullptr};
  wxWeakRef<wxColourPickerCtrl> line_color_picker_{nullptr};
  wxWeakRef<wxColourPickerCtrl> fill_color_picker_{nullptr};
  wxWeakRef<wxSlider> line_color_alpha_slider_{nullptr};
  wxWeakRef<wxSlider> fill_color_alpha_slider_{nullptr};

  wxWeakRef<wxStaticText> linewidth_label_{nullptr};
  wxWeakRef<wxStaticText> linecolor_label_{nullptr};
  wxWeakRef<wxStaticText> fillcolor_label_{nullptr};
  wxWeakRef<wxStaticText> line_transparency_label_{nullptr};
  wxWeakRef<wxStaticText> fill_transparency_label_{nullptr};
};
#endif  // SHAPE_PANEL_HPP
