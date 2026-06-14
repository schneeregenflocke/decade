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
        wxSizerFlags().Proportion(1).CenterVertical().Border(wxALL,
                                                             kSizerBorder);

    auto* vertical_sizer = std::make_unique<wxBoxSizer>(wxVERTICAL).release();
    SetSizer(vertical_sizer);

    frame_height_ctrl_ = std::make_unique<wxSpinCtrlDouble>(this).release();
    frame_height_ctrl_->SetDigits(2);
    AddLabelledRow(vertical_sizer, L"Frame Height", frame_height_ctrl_,
                   row_flags, label_flags, field_flags, kLabelWidth);

    size_ratio_ctrl_ = std::make_unique<wxSpinCtrlDouble>(this).release();
    size_ratio_ctrl_->SetDigits(2);
    size_ratio_ctrl_->SetIncrement(kSizeRatioIncrement);
    AddLabelledRow(vertical_sizer, L"Font Size Ratio", size_ratio_ctrl_,
                   row_flags, label_flags, field_flags, kLabelWidth);

    title_text_edit_ = std::make_unique<wxTextCtrl>(this, wxID_ANY).release();
    AddLabelledRow(vertical_sizer, L"Text", title_text_edit_, row_flags,
                   label_flags, field_flags, kLabelWidth);

    constexpr int kAlphaMax = 255;
    text_color_picker_ =
        std::make_unique<wxColourPickerCtrl>(
            this, wxID_ANY, *wxStockGDI::GetColour(wxStockGDI::COLOUR_BLACK))
            .release();
    AddLabelledRow(vertical_sizer, L"Color", text_color_picker_, row_flags,
                   label_flags, field_flags, kLabelWidth);

    alpha_slider_ =
        std::make_unique<wxSlider>(this, wxID_ANY, kAlphaMax, 0, kAlphaMax,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxSL_HORIZONTAL | wxSL_LABELS)
            .release();
    AddLabelledRow(vertical_sizer, L"Transparency", alpha_slider_, row_flags,
                   label_flags, field_flags, kLabelWidth);

    vertical_sizer->Layout();

    Bind(wxEVT_SPINCTRLDOUBLE, &TitleSetupPanel::CallbackSpinControl, this);
    Bind(wxEVT_TEXT, &TitleSetupPanel::CallbackTextControl, this);
    Bind(wxEVT_COLOURPICKER_CHANGED,
         &TitleSetupPanel::CallbackColorPickerControl, this);
    Bind(wxEVT_SLIDER, &TitleSetupPanel::CallbackSliderControl, this);
  }

  void SendDefaultValues() { SendTitleConfig(); }

  void SendTitleConfig() { signal_title_config_(title_config_); }

  void ReceiveTitleConfig(const TitleConfig& incoming_title_config) {
    title_config_ = incoming_title_config;
    UpdateWidgetForSelection();
  }

  sigslot::signal<const TitleConfig&>& SignalTitleConfig() {
    return signal_title_config_;
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
    frame_height_ctrl_->SetValue(
        static_cast<double>(title_config_.FrameHeight()));
    size_ratio_ctrl_->SetValue(
        static_cast<double>(title_config_.FontSizeRatio()));

    title_text_edit_->ChangeValue(title_config_.TitleText());

    const std::array<float, 4>& text_color = title_config_.TextColor();
    text_color_picker_->SetColour(wxColour(
        ToByte(text_color[0]), ToByte(text_color[1]), ToByte(text_color[2])));
    alpha_slider_->SetValue(static_cast<int>(ToByte(text_color[3])));
  }

  void CallbackSpinControl(wxSpinDoubleEvent& event) {
    auto float_value = static_cast<float>(event.GetValue());

    if (frame_height_ctrl_.get() == event.GetEventObject()) {
      title_config_.SetFrameHeight(float_value);

      SendTitleConfig();
    }

    if (size_ratio_ctrl_.get() == event.GetEventObject()) {
      title_config_.SetFontSizeRatio(float_value);

      SendTitleConfig();
    }
  }

  void CallbackTextControl(wxCommandEvent& event) {
    if (title_text_edit_.get() == event.GetEventObject()) {
      title_config_.SetTitleText(event.GetString().ToStdString());

      SendTitleConfig();
    }
  }

  void CallbackColorPickerControl(wxColourPickerEvent& event) {
    const wxColour color = event.GetColour();
    std::array<float, 4> text_color = title_config_.TextColor();
    text_color[0] = ToChannel(color.Red());
    text_color[1] = ToChannel(color.Green());
    text_color[2] = ToChannel(color.Blue());
    title_config_.SetTextColor(text_color);

    SendTitleConfig();
  }

  void CallbackSliderControl(wxCommandEvent& event) {
    std::array<float, 4> text_color = title_config_.TextColor();
    text_color[3] = ToChannel(event.GetInt());
    title_config_.SetTextColor(text_color);

    SendTitleConfig();
  }

  TitleConfig title_config_;
  sigslot::signal<const TitleConfig&> signal_title_config_;

  wxWeakRef<wxSpinCtrlDouble> frame_height_ctrl_;
  wxWeakRef<wxSpinCtrlDouble> size_ratio_ctrl_;

  wxWeakRef<wxTextCtrl> title_text_edit_;

  wxWeakRef<wxColourPickerCtrl> text_color_picker_;
  wxWeakRef<wxSlider> alpha_slider_;
};
#endif  // TITLE_PANEL_HPP
