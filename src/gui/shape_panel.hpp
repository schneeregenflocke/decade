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

#include "../packages/color_palette.hpp"
#include "../packages/date_group.hpp"
#include "../packages/shape_configuration.hpp"
#include "casts.hpp"

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

  // One dynamic "Bar Group" shape configuration mirrors each date group. When
  // groups are added we synthesise fresh configurations from the color palette;
  // existing configurations are left untouched so user customisations survive.
  void ReceiveDateGroups(const std::vector<DateGroup>& date_groups) {
    const size_t persistent_count =
        shape_config_set_.GetNumberPersistentConfigurations();
    const size_t desired_size = persistent_count + date_groups.size();
    const size_t previous_size = shape_config_set_.size();

    shape_config_set_.resize(desired_size);

    // Only the newly appended entries need defaults; entries that already
    // existed keep whatever the user (or a loaded project) configured.
    for (size_t index = std::max(previous_size, persistent_count);
         index < desired_size; ++index) {
      const size_t group_index = index - persistent_count;
      shape_config_set_[index] = MakeBarGroupConfiguration(group_index);
    }

    // "Annual Sum" is the next entry in the same categorical sequence, so it
    // takes the palette color just past the last group. Its palette index moves
    // with the group count, so refresh it only when the count actually changed
    // (a rename keeps the user's customizations intact, like the groups above).
    const size_t previous_group_count = (previous_size > persistent_count)
                                            ? previous_size - persistent_count
                                            : 0;
    if (date_groups.size() != previous_group_count) {
      RefreshAnnualSumConfiguration(date_groups.size());
    }

    UpdateConfigurationList();

    signal_shape_config_set_(shape_config_set_);
  }

  sigslot::signal<const ShapeConfigSet&>& SignalShapeConfigSet() {
    return signal_shape_config_set_;
  }

 private:
  sigslot::signal<const ShapeConfigSet&> signal_shape_config_set_;

  // Builds a categorical shape configuration: the color is taken from the
  // shared palette at the given index, with a stronger outline than fill so the
  // box has a visible border while the fill stays pastel. The single source of
  // the alpha recipe for every palette-driven entry (bar groups and the annual
  // sum).
  static ShapeConfiguration MakeCategoricalConfiguration(std::string name,
                                                         size_t palette_index) {
    constexpr float kDynamicLineWidth = 0.5F;
    constexpr float kOutlineAlpha = 0.75F;
    constexpr float kFillAlpha = 0.35F;

    const glm::vec3 color = palette::CategoricalColor(palette_index);

    return ShapeConfiguration{
        std::move(name),
        true,
        true,
        kDynamicLineWidth,
        ShapeConfiguration::OutlineColorValue{glm::vec4(color, kOutlineAlpha)},
        ShapeConfiguration::FillColorValue{glm::vec4(color, kFillAlpha)}};
  }

  // Default configuration for the dynamic bar group at the given zero-based
  // index, reproducible across sessions because the palette is index-stable.
  static ShapeConfiguration MakeBarGroupConfiguration(size_t group_index) {
    return MakeCategoricalConfiguration(
        ShapeConfigSet::DynamicConfigurationName(group_index), group_index);
  }

  // Re-derives the "Annual Sum" (Years Totals) configuration from the palette
  // at the index right past the last group, so it is colored and styled exactly
  // like a bar group and stays consistent if the palette is ever changed.
  void RefreshAnnualSumConfiguration(size_t group_count) {
    const size_t persistent_count =
        shape_config_set_.GetNumberPersistentConfigurations();
    for (size_t index = 0; index < persistent_count; ++index) {
      if (shape_config_set_[index] ==
          ShapeConfigSet::AnnualSumConfigurationName()) {
        shape_config_set_[index] = MakeCategoricalConfiguration(
            ShapeConfigSet::AnnualSumConfigurationName(), group_count);
        break;
      }
    }
  }

  void InitWidgets() {
    constexpr int kAlphaMax = 100;
    constexpr int kAlphaMin = 0;
    constexpr int kDefaultLabelWidth = 150;
    constexpr double kLineWidthMin = 0.0;
    constexpr double kLineWidthMax = 10.0;
    constexpr double kLineWidthIncrement = 0.05;

    shape_configuration_list_box_ =
        std::make_unique<wxListBox>(
            this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr,
            wxLB_SINGLE | wxLB_NEEDED_SB, wxDefaultValidator, wxEmptyString)
            .release();

    outline_visible_ctrl_ =
        std::make_unique<wxCheckBox>(this, wxID_ANY, L"Outline Visible")
            .release();
    fill_visible_ctrl_ =
        std::make_unique<wxCheckBox>(this, wxID_ANY, L"Fill Visible").release();

    line_color_picker_ =
        std::make_unique<wxColourPickerCtrl>(
            this, wxID_ANY, *wxStockGDI::GetColour(wxStockGDI::COLOUR_BLACK),
            wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_ALPHA)
            .release();
    fill_color_picker_ =
        std::make_unique<wxColourPickerCtrl>(
            this, wxID_ANY, *wxStockGDI::GetColour(wxStockGDI::COLOUR_BLACK),
            wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_ALPHA)
            .release();

    linewidth_ctrl_ =
        std::make_unique<wxSpinCtrlDouble>(
            this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
            /*16384L*/ wxSP_ARROW_KEYS | wxALIGN_RIGHT)
            .release();
    linewidth_ctrl_->SetRange(kLineWidthMin, kLineWidthMax);
    linewidth_ctrl_->SetDigits(2);
    linewidth_ctrl_->SetIncrement(kLineWidthIncrement);

    const wxSize default_label_size(kDefaultLabelWidth, -1);
    linewidth_label_ =
        std::make_unique<wxStaticText>(this, wxID_ANY, "Line Width",
                                       wxDefaultPosition, default_label_size)
            .release();
    linecolor_label_ =
        std::make_unique<wxStaticText>(this, wxID_ANY, "Line Color",
                                       wxDefaultPosition, default_label_size)
            .release();
    fillcolor_label_ =
        std::make_unique<wxStaticText>(this, wxID_ANY, "Fill Color",
                                       wxDefaultPosition, default_label_size)
            .release();
    line_transparency_label_ =
        std::make_unique<wxStaticText>(this, wxID_ANY, "Transparency",
                                       wxDefaultPosition, default_label_size)
            .release();
    fill_transparency_label_ =
        std::make_unique<wxStaticText>(this, wxID_ANY, "Transparency",
                                       wxDefaultPosition, default_label_size)
            .release();

    line_color_alpha_slider_ =
        std::make_unique<wxSlider>(this, wxID_ANY, kAlphaMin, kAlphaMin,
                                   kAlphaMax, wxDefaultPosition, wxDefaultSize,
                                   wxSL_HORIZONTAL)
            .release();
    fill_color_alpha_slider_ =
        std::make_unique<wxSlider>(this, wxID_ANY, kAlphaMin, kAlphaMin,
                                   kAlphaMax, wxDefaultPosition, wxDefaultSize,
                                   wxSL_HORIZONTAL)
            .release();

    line_color_alpha_slider_->SetMax(kAlphaMax);
    fill_color_alpha_slider_->SetMax(kAlphaMax);
  }

  void InitSizers() {
    constexpr int kDefaultSizerBorder = 5;
    std::array<wxSizerFlags, 3> sizer_flags;
    sizer_flags[0].Proportion(0).Expand();
    sizer_flags[1].Proportion(0).CenterVertical().Border(wxALL,
                                                         kDefaultSizerBorder);
    sizer_flags[2].Proportion(1).Expand().Border(wxALL, kDefaultSizerBorder);

    auto* vertical_sizer = std::make_unique<wxBoxSizer>(wxVERTICAL).release();
    SetSizer(vertical_sizer);

    auto* static_box_sizer_elements =
        std::make_unique<wxStaticBoxSizer>(wxVERTICAL, this, "Elements")
            .release();
    auto* static_box_sizer_outline =
        std::make_unique<wxStaticBoxSizer>(wxVERTICAL, this, "Outline")
            .release();
    auto* static_box_sizer_fill =
        std::make_unique<wxStaticBoxSizer>(wxVERTICAL, this, "Fill").release();

    vertical_sizer->Add(static_box_sizer_elements, sizer_flags[2]);
    vertical_sizer->Add(static_box_sizer_outline, sizer_flags[0]);
    vertical_sizer->Add(static_box_sizer_fill, sizer_flags[0]);

    std::array<wxBoxSizer*, 4> horizontal_sizers_outline{};
    for (auto& horizontal_sizer : horizontal_sizers_outline) {
      horizontal_sizer = std::make_unique<wxBoxSizer>(wxHORIZONTAL).release();
      static_box_sizer_outline->Add(horizontal_sizer, sizer_flags[0]);
    }

    std::array<wxBoxSizer*, 3> horizontal_sizers_fill{};
    for (auto& horizontal_sizer : horizontal_sizers_fill) {
      horizontal_sizer = std::make_unique<wxBoxSizer>(wxHORIZONTAL).release();
      static_box_sizer_fill->Add(horizontal_sizer, sizer_flags[0]);
    }

    static_box_sizer_elements->Add(shape_configuration_list_box_,
                                   sizer_flags[2]);

    horizontal_sizers_outline[0]->Add(outline_visible_ctrl_, sizer_flags[2]);
    horizontal_sizers_outline[1]->Add(linewidth_label_, sizer_flags[1]);
    horizontal_sizers_outline[1]->Add(linewidth_ctrl_, sizer_flags[2]);
    horizontal_sizers_outline[2]->Add(linecolor_label_, sizer_flags[1]);
    horizontal_sizers_outline[2]->Add(line_color_picker_, sizer_flags[2]);
    horizontal_sizers_outline[3]->Add(line_transparency_label_, sizer_flags[1]);
    horizontal_sizers_outline[3]->Add(line_color_alpha_slider_, sizer_flags[2]);

    horizontal_sizers_fill[0]->Add(fill_visible_ctrl_, sizer_flags[2]);
    horizontal_sizers_fill[1]->Add(fillcolor_label_, sizer_flags[1]);
    horizontal_sizers_fill[1]->Add(fill_color_picker_, sizer_flags[2]);
    horizontal_sizers_fill[2]->Add(fill_transparency_label_, sizer_flags[1]);
    horizontal_sizers_fill[2]->Add(fill_color_alpha_slider_, sizer_flags[2]);

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

  void CallbackListBook(wxCommandEvent& event) {
    const int selection_index = event.GetSelection();
    if (selection_index == wxNOT_FOUND) {
      return;
    }
    UpdateWidgetForSelection(static_cast<size_t>(selection_index));
  }

  void CallbackCheckBox(wxCommandEvent& event) {
    auto check_status = event.IsChecked();
    const int selection_index = shape_configuration_list_box_->GetSelection();
    if (selection_index == wxNOT_FOUND) {
      return;
    }
    const auto selection = static_cast<size_t>(selection_index);

    if (event.GetEventObject() == outline_visible_ctrl_.get()) {
      shape_config_set_[selection].OutlineVisible(check_status);
    }

    if (event.GetEventObject() == fill_visible_ctrl_.get()) {
      shape_config_set_[selection].FillVisible(check_status);
    }

    UpdateWidgetForSelection(selection);

    signal_shape_config_set_(shape_config_set_);
  }

  void CallbackSpinControlDouble(wxSpinDoubleEvent& event) {
    const auto line_width = static_cast<float>(event.GetValue());
    const int selection_index = shape_configuration_list_box_->GetSelection();
    if (selection_index == wxNOT_FOUND) {
      return;
    }
    const auto element_selection = static_cast<size_t>(selection_index);
    shape_config_set_[element_selection].LineWidth(line_width);

    signal_shape_config_set_(shape_config_set_);
  }

  void CallbackColorPicker(wxColourPickerEvent& event) {
    auto color = to_glm_vec4(event.GetColour());
    const int selection_index = shape_configuration_list_box_->GetSelection();
    if (selection_index == wxNOT_FOUND) {
      return;
    }
    const auto selection = static_cast<size_t>(selection_index);

    if (event.GetEventObject() == line_color_picker_.get()) {
      auto current_line_color_alpha =
          shape_config_set_[selection].OutlineColor()[3];
      color[3] = current_line_color_alpha;
      shape_config_set_[selection].OutlineColor(color);
    }

    if (event.GetEventObject() == fill_color_picker_.get()) {
      auto current_line_color_alpha =
          shape_config_set_[selection].FillColor()[3];
      color[3] = current_line_color_alpha;
      shape_config_set_[selection].FillColor(color);
    }

    signal_shape_config_set_(shape_config_set_);
  }

  void CallbackSlider(wxCommandEvent& event) {
    constexpr float kAlphaScale = 100.0F;
    const auto slider_value = static_cast<float>(event.GetInt());
    const float alpha_value = 1.0F - (slider_value / kAlphaScale);

    const int selection_index = shape_configuration_list_box_->GetSelection();
    if (selection_index == wxNOT_FOUND) {
      return;
    }
    const auto selection = static_cast<size_t>(selection_index);

    if (event.GetEventObject() == line_color_alpha_slider_.get()) {
      auto current_line_color = shape_config_set_[selection].OutlineColor();
      current_line_color[3] = alpha_value;
      shape_config_set_[selection].OutlineColor(current_line_color);
    }

    if (event.GetEventObject() == fill_color_alpha_slider_.get()) {
      auto current_line_color = shape_config_set_[selection].FillColor();
      current_line_color[3] = alpha_value;
      shape_config_set_[selection].FillColor(current_line_color);
    }

    signal_shape_config_set_(shape_config_set_);
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
