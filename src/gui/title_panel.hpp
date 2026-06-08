#ifndef TITLE_PANEL_HPP
#define TITLE_PANEL_HPP

// #include "../wx_widgets_include.hpp"
#include <wx/clrpicker.h>
#include <wx/spinctrl.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <array>
#include <cmath>
#include <memory>
#include <sigslot/signal.hpp>

#include "../packages/title_config.hpp"

class TitleSetupPanel : public wxPanel {
 public:
  explicit TitleSetupPanel(wxWindow* parent) : wxPanel(parent, wxID_ANY) {
    constexpr int kSizerBorder = 5;
    constexpr int kLabelWidth = 120;
    constexpr double kSizeRatioIncrement = 0.05;

    const wxSizerFlags row_flags = wxSizerFlags().Proportion(0).Expand();
    const wxSizerFlags label_flags =
        wxSizerFlags().Proportion(0).Expand().Border(wxALL, kSizerBorder);
    const wxSizerFlags field_flags =
        wxSizerFlags().Proportion(1).Expand().Border(wxALL, kSizerBorder);

    auto* vertical_sizer = std::make_unique<wxBoxSizer>(wxVERTICAL).release();
    SetSizer(vertical_sizer);

    frame_height_ctrl = std::make_unique<wxSpinCtrlDouble>(this).release();
    frame_height_ctrl->SetDigits(2);
    AddLabelledRow(vertical_sizer, L"Frame Height", frame_height_ctrl,
                   row_flags, label_flags, field_flags, kLabelWidth);

    size_ratio_ctrl = std::make_unique<wxSpinCtrlDouble>(this).release();
    size_ratio_ctrl->SetDigits(2);
    size_ratio_ctrl->SetIncrement(kSizeRatioIncrement);
    AddLabelledRow(vertical_sizer, L"Font Size Ratio", size_ratio_ctrl,
                   row_flags, label_flags, field_flags, kLabelWidth);

    title_text_edit = std::make_unique<wxTextCtrl>(this, wxID_ANY).release();
    AddLabelledRow(vertical_sizer, L"Text", title_text_edit, row_flags,
                   label_flags, field_flags, kLabelWidth);

    constexpr int kAlphaMax = 255;
    text_color_picker =
        std::make_unique<wxColourPickerCtrl>(
            this, wxID_ANY, *wxStockGDI::GetColour(wxStockGDI::COLOUR_BLACK))
            .release();
    AddLabelledRow(vertical_sizer, L"Color", text_color_picker, row_flags,
                   label_flags, field_flags, kLabelWidth);

    alpha_slider =
        std::make_unique<wxSlider>(this, wxID_ANY, kAlphaMax, 0, kAlphaMax,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxSL_HORIZONTAL | wxSL_LABELS)
            .release();
    AddLabelledRow(vertical_sizer, L"Transparency", alpha_slider, row_flags,
                   label_flags, field_flags, kLabelWidth);

    vertical_sizer->Layout();

    ////////////////////////////////////////

    Bind(wxEVT_SPINCTRLDOUBLE, &TitleSetupPanel::CallbackSpinControl, this);
    Bind(wxEVT_TEXT, &TitleSetupPanel::CallbackTextControl, this);
    Bind(wxEVT_COLOURPICKER_CHANGED,
         &TitleSetupPanel::CallbackColorPickerControl, this);
    Bind(wxEVT_SLIDER, &TitleSetupPanel::CallbackSliderControl, this);

    ////////////////////////////////////////
  }

  void SendDefaultValues() { SendTitleConfig(); }

  void SendTitleConfig() { signal_title_config(title_config); }

  void ReceiveTitleConfig(const TitleConfig& incoming_title_config) {
    title_config = incoming_title_config;
    UpdateWidgetForSelection();
  }

  sigslot::signal<const TitleConfig&>& SignalTitleConfig() {
    return signal_title_config;
  }

 private:
  // Colour channels are stored as floats in [0, 1] in the domain but exposed by
  // wx as 8-bit byte values.
  static constexpr float kByteMax = 255.0F;

  static unsigned char ToByte(float channel) {
    return static_cast<unsigned char>(std::lround(channel * kByteMax));
  }

  static float ToChannel(int byte_value) {
    return static_cast<float>(byte_value) / kByteMax;
  }

  void AddLabelledRow(wxSizer* parent_sizer, const wxString& text,
                      wxWindow* field, const wxSizerFlags& row_flags,
                      const wxSizerFlags& label_flags,
                      const wxSizerFlags& field_flags, int label_width) {
    auto* row_sizer = std::make_unique<wxBoxSizer>(wxHORIZONTAL).release();
    auto* label =
        std::make_unique<wxStaticText>(this, wxID_ANY, text).release();
    label->SetMinSize(wxSize(label_width, -1));
    row_sizer->Add(label, label_flags);
    row_sizer->Add(field, field_flags);
    parent_sizer->Add(row_sizer, row_flags);
  }

  void UpdateWidgetForSelection() {
    frame_height_ctrl->SetValue(
        static_cast<double>(title_config.FrameHeight()));
    size_ratio_ctrl->SetValue(
        static_cast<double>(title_config.FontSizeRatio()));

    title_text_edit->ChangeValue(title_config.TitleText());

    const std::array<float, 4>& text_color = title_config.TextColor();
    text_color_picker->SetColour(wxColour(
        ToByte(text_color[0]), ToByte(text_color[1]), ToByte(text_color[2])));
    alpha_slider->SetValue(static_cast<int>(ToByte(text_color[3])));
  }

  void CallbackSpinControl(wxSpinDoubleEvent& event) {
    auto float_value = static_cast<float>(event.GetValue());

    if (frame_height_ctrl.get() == event.GetEventObject()) {
      title_config.SetFrameHeight(float_value);

      SendTitleConfig();
    }

    if (size_ratio_ctrl.get() == event.GetEventObject()) {
      title_config.SetFontSizeRatio(float_value);

      SendTitleConfig();
    }
  }

  void CallbackTextControl(wxCommandEvent& event) {
    if (title_text_edit.get() == event.GetEventObject()) {
      title_config.SetTitleText(event.GetString().ToStdString());

      SendTitleConfig();
    }
  }

  void CallbackColorPickerControl(wxColourPickerEvent& event) {
    const wxColour color = event.GetColour();
    std::array<float, 4> text_color = title_config.TextColor();
    text_color[0] = ToChannel(color.Red());
    text_color[1] = ToChannel(color.Green());
    text_color[2] = ToChannel(color.Blue());
    title_config.SetTextColor(text_color);

    SendTitleConfig();
  }

  void CallbackSliderControl(wxCommandEvent& event) {
    std::array<float, 4> text_color = title_config.TextColor();
    text_color[3] = ToChannel(event.GetInt());
    title_config.SetTextColor(text_color);

    SendTitleConfig();
  }

  TitleConfig title_config;
  sigslot::signal<const TitleConfig&> signal_title_config;

  wxWeakRef<wxSpinCtrlDouble> frame_height_ctrl;
  wxWeakRef<wxSpinCtrlDouble> size_ratio_ctrl;

  wxWeakRef<wxTextCtrl> title_text_edit;

  wxWeakRef<wxColourPickerCtrl> text_color_picker;
  wxWeakRef<wxSlider> alpha_slider;
};
#endif  // TITLE_PANEL_HPP
