#ifndef HOME_TITAN99_CODE_DECADE_SRC_GUI_SHAPE_PANEL_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GUI_SHAPE_PANEL_HPP

#include <wx/clrpicker.h>
#include <wx/spinctrl.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <algorithm>
#include <memory>
#include <sigslot/signal.hpp>
#include <string>
#include <vector>

#include "../casts.hpp"
#include "../packages/group_store.hpp"
#include "../packages/shape_config.hpp"

class ElementsSetupsPanel {
 public:
  explicit ElementsSetupsPanel(wxWindow* parent) {
    wx_panel =
        std::make_unique<wxPanel>(parent, wxID_ANY, wxDefaultPosition,
                                  wxDefaultSize, wxTAB_TRAVERSAL, wxEmptyString)
            .release();

    InitWidgets();
    InitSizers();

    wx_panel->Bind(wxEVT_LISTBOX, &ElementsSetupsPanel::CallbackListBook, this);
    wx_panel->Bind(wxEVT_CHECKBOX, &ElementsSetupsPanel::CallbackCheckBox,
                   this);
    wx_panel->Bind(wxEVT_SPINCTRLDOUBLE,
                   &ElementsSetupsPanel::CallbackSpinControlDouble, this);
    wx_panel->Bind(wxEVT_COLOURPICKER_CHANGED,
                   &ElementsSetupsPanel::CallbackColorPicker, this);
    wx_panel->Bind(wxEVT_SLIDER, &ElementsSetupsPanel::CallbackSlider, this);
  }

  wxPanel* PanelPtr() { return wx_panel.get(); }

  void ReceiveShapeConfigurationStorage(
      const ShapeConfigurationStorage& incoming_shape_configuration_storage) {
    shape_configuration_storage.CopyFrom(incoming_shape_configuration_storage);
    UpdateConfigurationList();
  }

  void ReceiveDateGroups(const std::vector<DateGroup>& date_groups) {
    const size_t number_dynamic_configurations =
        shape_configuration_storage.size() -
        shape_configuration_storage.GetNumberPersistentConfigurations();

    const int adjust_number_dynamic_configurations =
        static_cast<int>(date_groups.size()) -
        static_cast<int>(number_dynamic_configurations);

    const int current_size =
        static_cast<int>(shape_configuration_storage.size());
    const int minimum_size = static_cast<int>(
        shape_configuration_storage.GetNumberPersistentConfigurations());
    const int desired_size =
        current_size + adjust_number_dynamic_configurations;
    const int adjusted_size = std::max(desired_size, minimum_size);
    shape_configuration_storage.resize(static_cast<size_t>(adjusted_size));

    if (adjust_number_dynamic_configurations > 0) {
      size_t index = date_groups.size() - adjust_number_dynamic_configurations;

      constexpr float kDynamicLineWidth = 0.5F;
      constexpr float kOutlineRed = 0.25F;
      constexpr float kOutlineGreen = 0.25F;
      constexpr float kOutlineBlue = 0.75F;
      constexpr float kOutlineAlpha = 0.75F;
      constexpr float kFillRed = 0.25F;
      constexpr float kFillGreen = 0.25F;
      constexpr float kFillBlue = 0.75F;
      constexpr float kFillAlpha = 0.35F;

      for (; index < date_groups.size(); ++index) {
        ShapeConfiguration temporary(
            std::string("Bar Group ") + std::to_string(index), true, true,
            kDynamicLineWidth,
            ShapeConfiguration::OutlineColorValue{glm::vec4(
                kOutlineRed, kOutlineGreen, kOutlineBlue, kOutlineAlpha)},
            ShapeConfiguration::FillColorValue{
                glm::vec4(kFillRed, kFillGreen, kFillBlue, kFillAlpha)});
        temporary.RandomColor();
        shape_configuration_storage[shape_configuration_storage
                                        .GetNumberPersistentConfigurations() +
                                    index] = temporary;
      }
    }

    UpdateConfigurationList();

    signal_shape_configuration_storage(shape_configuration_storage);
  }

  sigslot::signal<const ShapeConfigurationStorage&>&
  SignalShapeConfigurationStorage() {
    return signal_shape_configuration_storage;
  }

 private:
  sigslot::signal<const ShapeConfigurationStorage&>
      signal_shape_configuration_storage;

  void InitWidgets() {
    constexpr int kAlphaMax = 100;
    constexpr int kAlphaMin = 0;
    constexpr int kDefaultLabelWidth = 150;
    constexpr double kLineWidthMin = 0.0;
    constexpr double kLineWidthMax = 10.0;
    constexpr double kLineWidthIncrement = 0.05;

    shape_configuration_list_box =
        std::make_unique<wxListBox>(wx_panel.get(), wxID_ANY, wxDefaultPosition,
                                    wxDefaultSize, 0, nullptr,
                                    wxLB_SINGLE | wxLB_NEEDED_SB,
                                    wxDefaultValidator, wxEmptyString)
            .release();

    outline_visible_ctrl = std::make_unique<wxCheckBox>(
                               wx_panel.get(), wxID_ANY, L"Outline Visible")
                               .release();
    fill_visible_ctrl =
        std::make_unique<wxCheckBox>(wx_panel.get(), wxID_ANY, L"Fill Visible")
            .release();

    line_color_picker = std::make_unique<wxColourPickerCtrl>(
                            wx_panel.get(), wxID_ANY,
                            *wxStockGDI::GetColour(wxStockGDI::COLOUR_BLACK),
                            wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_ALPHA)
                            .release();
    fill_color_picker = std::make_unique<wxColourPickerCtrl>(
                            wx_panel.get(), wxID_ANY,
                            *wxStockGDI::GetColour(wxStockGDI::COLOUR_BLACK),
                            wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_ALPHA)
                            .release();

    linewidth_ctrl = std::make_unique<wxSpinCtrlDouble>(
                         wx_panel.get(), wxID_ANY, wxEmptyString,
                         wxDefaultPosition, wxDefaultSize,
                         /*16384L*/ wxSP_ARROW_KEYS | wxALIGN_RIGHT)
                         .release();
    linewidth_ctrl->SetRange(kLineWidthMin, kLineWidthMax);
    linewidth_ctrl->SetDigits(2);
    linewidth_ctrl->SetIncrement(kLineWidthIncrement);

    const wxSize default_label_size(kDefaultLabelWidth, -1);
    linewidth_label =
        std::make_unique<wxStaticText>(wx_panel.get(), wxID_ANY, "Line Width",
                                       wxDefaultPosition, default_label_size)
            .release();
    linecolor_label =
        std::make_unique<wxStaticText>(wx_panel.get(), wxID_ANY, "Line Color",
                                       wxDefaultPosition, default_label_size)
            .release();
    fillcolor_label =
        std::make_unique<wxStaticText>(wx_panel.get(), wxID_ANY, "Fill Color",
                                       wxDefaultPosition, default_label_size)
            .release();
    line_transparency_label =
        std::make_unique<wxStaticText>(wx_panel.get(), wxID_ANY, "Transparency",
                                       wxDefaultPosition, default_label_size)
            .release();
    fill_transparency_label =
        std::make_unique<wxStaticText>(wx_panel.get(), wxID_ANY, "Transparency",
                                       wxDefaultPosition, default_label_size)
            .release();

    line_color_alpha_slider =
        std::make_unique<wxSlider>(wx_panel.get(), wxID_ANY, kAlphaMin,
                                   kAlphaMin, kAlphaMax, wxDefaultPosition,
                                   wxDefaultSize, wxSL_HORIZONTAL)
            .release();
    fill_color_alpha_slider =
        std::make_unique<wxSlider>(wx_panel.get(), wxID_ANY, kAlphaMin,
                                   kAlphaMin, kAlphaMax, wxDefaultPosition,
                                   wxDefaultSize, wxSL_HORIZONTAL)
            .release();

    line_color_alpha_slider->SetMax(kAlphaMax);
    fill_color_alpha_slider->SetMax(kAlphaMax);
  }

  void InitSizers() {
    constexpr int kDefaultSizerBorder = 5;
    std::array<wxSizerFlags, 3> sizer_flags;
    sizer_flags[0].Proportion(0).Expand();
    sizer_flags[1].Proportion(0).CenterVertical().Border(wxALL,
                                                         kDefaultSizerBorder);
    sizer_flags[2].Proportion(1).Expand().Border(wxALL, kDefaultSizerBorder);

    auto* vertical_sizer = std::make_unique<wxBoxSizer>(wxVERTICAL).release();
    wx_panel->SetSizer(vertical_sizer);

    auto* static_box_sizer_elements =
        std::make_unique<wxStaticBoxSizer>(wxVERTICAL, wx_panel.get(),
                                           "Elements")
            .release();
    auto* static_box_sizer_outline = std::make_unique<wxStaticBoxSizer>(
                                         wxVERTICAL, wx_panel.get(), "Outline")
                                         .release();
    auto* static_box_sizer_fill =
        std::make_unique<wxStaticBoxSizer>(wxVERTICAL, wx_panel.get(), "Fill")
            .release();

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

    static_box_sizer_elements->Add(shape_configuration_list_box,
                                   sizer_flags[2]);

    horizontal_sizers_outline[0]->Add(outline_visible_ctrl, sizer_flags[2]);
    horizontal_sizers_outline[1]->Add(linewidth_label, sizer_flags[1]);
    horizontal_sizers_outline[1]->Add(linewidth_ctrl, sizer_flags[2]);
    horizontal_sizers_outline[2]->Add(linecolor_label, sizer_flags[1]);
    horizontal_sizers_outline[2]->Add(line_color_picker, sizer_flags[2]);
    horizontal_sizers_outline[3]->Add(line_transparency_label, sizer_flags[1]);
    horizontal_sizers_outline[3]->Add(line_color_alpha_slider, sizer_flags[2]);

    horizontal_sizers_fill[0]->Add(fill_visible_ctrl, sizer_flags[2]);
    horizontal_sizers_fill[1]->Add(fillcolor_label, sizer_flags[1]);
    horizontal_sizers_fill[1]->Add(fill_color_picker, sizer_flags[2]);
    horizontal_sizers_fill[2]->Add(fill_transparency_label, sizer_flags[1]);
    horizontal_sizers_fill[2]->Add(fill_color_alpha_slider, sizer_flags[2]);

    vertical_sizer->Layout();
  }

  void UpdateConfigurationList() {
    shape_configuration_list_box->Clear();

    for (size_t index = 0; index < shape_configuration_storage.size();
         ++index) {
      shape_configuration_list_box->AppendString(
          shape_configuration_storage[index].Name());
    }

    const int number_of_items =
        static_cast<int>(shape_configuration_list_box->GetCount());
    if (number_of_items <= 0) {
      return;
    }

    int selection = shape_configuration_list_box->GetSelection();
    if (selection == wxNOT_FOUND) {
      selection = 0;
    }
    selection = std::clamp(selection, 0, number_of_items - 1);
    shape_configuration_list_box->Select(selection);
    UpdateWidgetForSelection(static_cast<size_t>(selection));
  }

  void UpdateWidgetForSelection(size_t selection) {
    if (selection >= shape_configuration_storage.size()) {
      return;
    }

    auto& current_configuration = shape_configuration_storage[selection];

    auto outline_visible = current_configuration.OutlineVisible();
    outline_visible_ctrl->SetValue(outline_visible);
    linewidth_ctrl->Enable(outline_visible);
    line_color_picker->Enable(outline_visible);
    line_color_alpha_slider->Enable(outline_visible);

    auto fill_visible = current_configuration.FillVisible();
    fill_visible_ctrl->SetValue(fill_visible);
    fill_color_picker->Enable(fill_visible);
    fill_color_alpha_slider->Enable(fill_visible);

    linewidth_ctrl->SetValue(current_configuration.LineWidthDisabled());

    constexpr float kAlphaScale = 100.0F;
    const auto outline_color = current_configuration.OutlineColorDisabled();
    const auto fill_color = current_configuration.FillColorDisabled();

    line_color_picker->SetColour(to_wx_color(outline_color));
    line_color_alpha_slider->SetValue(
        static_cast<int>(kAlphaScale - (outline_color[3] * kAlphaScale)));

    fill_color_picker->SetColour(to_wx_color(fill_color));
    fill_color_alpha_slider->SetValue(
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
    const int selection_index = shape_configuration_list_box->GetSelection();
    if (selection_index == wxNOT_FOUND) {
      return;
    }
    const auto selection = static_cast<size_t>(selection_index);

    if (event.GetEventObject() == outline_visible_ctrl.get()) {
      shape_configuration_storage[selection].OutlineVisible(check_status);
    }

    if (event.GetEventObject() == fill_visible_ctrl.get()) {
      shape_configuration_storage[selection].FillVisible(check_status);
    }

    UpdateWidgetForSelection(selection);

    signal_shape_configuration_storage(shape_configuration_storage);
  }

  void CallbackSpinControlDouble(wxSpinDoubleEvent& event) {
    const auto line_width = static_cast<float>(event.GetValue());
    const int selection_index = shape_configuration_list_box->GetSelection();
    if (selection_index == wxNOT_FOUND) {
      return;
    }
    const auto element_selection = static_cast<size_t>(selection_index);
    shape_configuration_storage[element_selection].LineWidth(line_width);

    signal_shape_configuration_storage(shape_configuration_storage);
  }

  void CallbackColorPicker(wxColourPickerEvent& event) {
    auto color = to_glm_vec4(event.GetColour());
    const int selection_index = shape_configuration_list_box->GetSelection();
    if (selection_index == wxNOT_FOUND) {
      return;
    }
    const auto selection = static_cast<size_t>(selection_index);

    if (event.GetEventObject() == line_color_picker.get()) {
      auto current_line_color_alpha =
          shape_configuration_storage[selection].OutlineColor()[3];
      color[3] = current_line_color_alpha;
      shape_configuration_storage[selection].OutlineColor(color);
    }

    if (event.GetEventObject() == fill_color_picker.get()) {
      auto current_line_color_alpha =
          shape_configuration_storage[selection].FillColor()[3];
      color[3] = current_line_color_alpha;
      shape_configuration_storage[selection].FillColor(color);
    }

    signal_shape_configuration_storage(shape_configuration_storage);
  }

  void CallbackSlider(wxCommandEvent& event) {
    constexpr float kAlphaScale = 100.0F;
    const auto slider_value = static_cast<float>(event.GetInt());
    const float alpha_value = 1.0F - (slider_value / kAlphaScale);

    const int selection_index = shape_configuration_list_box->GetSelection();
    if (selection_index == wxNOT_FOUND) {
      return;
    }
    const auto selection = static_cast<size_t>(selection_index);

    if (event.GetEventObject() == line_color_alpha_slider.get()) {
      auto current_line_color =
          shape_configuration_storage[selection].OutlineColor();
      current_line_color[3] = alpha_value;
      shape_configuration_storage[selection].OutlineColor(current_line_color);
    }

    if (event.GetEventObject() == fill_color_alpha_slider.get()) {
      auto current_line_color =
          shape_configuration_storage[selection].FillColor();
      current_line_color[3] = alpha_value;
      shape_configuration_storage[selection].FillColor(current_line_color);
    }

    signal_shape_configuration_storage(shape_configuration_storage);
  }

  ShapeConfigurationStorage shape_configuration_storage;

  wxWeakRef<wxPanel> wx_panel{nullptr};

  wxWeakRef<wxListBox> shape_configuration_list_box{nullptr};
  wxWeakRef<wxCheckBox> outline_visible_ctrl{nullptr};
  wxWeakRef<wxCheckBox> fill_visible_ctrl{nullptr};
  wxWeakRef<wxSpinCtrlDouble> linewidth_ctrl{nullptr};
  wxWeakRef<wxColourPickerCtrl> line_color_picker{nullptr};
  wxWeakRef<wxColourPickerCtrl> fill_color_picker{nullptr};
  wxWeakRef<wxSlider> line_color_alpha_slider{nullptr};
  wxWeakRef<wxSlider> fill_color_alpha_slider{nullptr};

  wxWeakRef<wxStaticText> linewidth_label{nullptr};
  wxWeakRef<wxStaticText> linecolor_label{nullptr};
  wxWeakRef<wxStaticText> fillcolor_label{nullptr};
  wxWeakRef<wxStaticText> line_transparency_label{nullptr};
  wxWeakRef<wxStaticText> fill_transparency_label{nullptr};
};
#endif  // HOME_TITAN99_CODE_DECADE_SRC_GUI_SHAPE_PANEL_HPP
