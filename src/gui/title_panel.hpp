#ifndef HOME_TITAN99_CODE_DECADE_SRC_GUI_TITLE_PANEL_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GUI_TITLE_PANEL_HPP

// #include "../wx_widgets_include.hpp"
#include <wx/clrpicker.h>
#include <wx/spinctrl.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include "../packages/title_config.hpp"

#include <sigslot/signal.hpp>

#include <glm/glm.hpp>

#include <array>
#include <memory>

class TitleSetupPanel {
public:
  explicit TitleSetupPanel(wxWindow *parent)
  {
    wx_panel = std::make_unique<wxPanel>(parent, wxID_ANY).release();

    constexpr int kFieldCount = 5;
    constexpr int kSizerBorder = 5;
    constexpr int kLabelWidth = 120;
    constexpr int kAlphaMax = 255;
    constexpr double kSizeRatioIncrement = 0.05;

    const wxSizerFlags sizer_flags_0 = wxSizerFlags().Proportion(0).Expand();
    const wxSizerFlags sizer_flags_1 =
        wxSizerFlags().Proportion(0).Expand().Border(wxALL, kSizerBorder);
    const wxSizerFlags sizer_flags_2 =
        wxSizerFlags().Proportion(1).Expand().Border(wxALL, kSizerBorder);

    auto *vertical_sizer = std::make_unique<wxBoxSizer>(wxVERTICAL).release();
    wx_panel->SetSizer(vertical_sizer);

    std::array<wxBoxSizer *, kFieldCount> horizontal_sizers{};

    for (auto &sizer : horizontal_sizers) {
      sizer = std::make_unique<wxBoxSizer>(wxHORIZONTAL).release();
      vertical_sizer->Add(sizer, sizer_flags_0);
    }

    std::array<wxStaticText *, kFieldCount> labels{};

    labels[0] =
        std::make_unique<wxStaticText>(wx_panel.get(), wxID_ANY, L"Frame Height").release();
    labels[0]->SetMinSize(wxSize(kLabelWidth, -1));
    horizontal_sizers[0]->Add(labels[0], sizer_flags_1);

    labels[1] =
        std::make_unique<wxStaticText>(wx_panel.get(), wxID_ANY, L"Font Size Ratio").release();
    labels[1]->SetMinSize(wxSize(kLabelWidth, -1));
    horizontal_sizers[1]->Add(labels[1], sizer_flags_1);

    labels[2] = std::make_unique<wxStaticText>(wx_panel.get(), wxID_ANY, L"Text").release();
    labels[2]->SetMinSize(wxSize(kLabelWidth, -1));
    horizontal_sizers[2]->Add(labels[2], sizer_flags_1);

    labels[3] = std::make_unique<wxStaticText>(wx_panel.get(), wxID_ANY, L"Text Color").release();
    labels[3]->SetMinSize(wxSize(kLabelWidth, -1));
    horizontal_sizers[3]->Add(labels[3], sizer_flags_1);
    labels[3]->Enable(false);

    labels[4] =
        std::make_unique<wxStaticText>(wx_panel.get(), wxID_ANY, L"Color Transparency").release();
    labels[4]->SetMinSize(wxSize(kLabelWidth, -1));
    horizontal_sizers[4]->Add(labels[4], sizer_flags_1);
    labels[4]->Enable(false);

    frame_height_ctrl = std::make_unique<wxSpinCtrlDouble>(wx_panel.get()).release();
    frame_height_ctrl->SetDigits(2);
    horizontal_sizers[0]->Add(frame_height_ctrl, sizer_flags_2);

    size_ratio_ctrl = std::make_unique<wxSpinCtrlDouble>(wx_panel.get()).release();
    size_ratio_ctrl->SetDigits(2);
    size_ratio_ctrl->SetIncrement(kSizeRatioIncrement);
    horizontal_sizers[1]->Add(size_ratio_ctrl, sizer_flags_2);

    title_text_edit = std::make_unique<wxTextCtrl>(wx_panel.get(), wxID_ANY).release();
    horizontal_sizers[2]->Add(title_text_edit, sizer_flags_2);

    text_color_picker =
        std::make_unique<wxColourPickerCtrl>(
            wx_panel.get(), wxID_ANY, *wxStockGDI::GetColour(wxStockGDI::COLOUR_BLACK),
            wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_ALPHA)
            .release();
    text_color_picker->Enable(false);
    horizontal_sizers[3]->Add(text_color_picker, sizer_flags_2);

    alpha_slider = std::make_unique<wxSlider>(wx_panel.get(), wxID_ANY, kAlphaMax, 0, kAlphaMax,
                                              wxDefaultPosition, wxDefaultSize,
                                              wxSL_HORIZONTAL | wxSL_LABELS)
                       .release();
    alpha_slider->Enable(false);
    horizontal_sizers[4]->Add(alpha_slider, sizer_flags_2);

    vertical_sizer->Layout();

    ////////////////////////////////////////

    wx_panel->Bind(wxEVT_SPINCTRLDOUBLE, &TitleSetupPanel::CallbackSpinControl, this);
    wx_panel->Bind(wxEVT_TEXT, &TitleSetupPanel::CallbackTextControl, this);
    wx_panel->Bind(wxEVT_COLOURPICKER_CHANGED, &TitleSetupPanel::CallbackColorPickerControl, this);
    wx_panel->Bind(wxEVT_SLIDER, &TitleSetupPanel::CallbackSliderControl, this);

    ////////////////////////////////////////
  }

  wxPanel *PanelPtr() { return wx_panel.get(); }

  void SendDefaultValues() { SendTitleConfig(); }

  void SendTitleConfig() { signal_title_config(title_config); }

  void ReceiveTitleConfig(const TitleConfig &incoming_title_config)
  {
    title_config = incoming_title_config;
    UpdateWidgetForSelection();
  }

  sigslot::signal<const TitleConfig &> &SignalTitleConfig() { return signal_title_config; }

private:
  void UpdateWidgetForSelection()
  {
    frame_height_ctrl->SetValue(static_cast<double>(title_config.FrameHeight()));
    size_ratio_ctrl->SetValue(static_cast<double>(title_config.FontSizeRatio()));

    title_text_edit->ChangeValue(title_config.TitleText());

    /*wxColour color;
    color.Set(
            glm::floatBitsToUint(title_config.text_color[0]),
            glm::floatBitsToUint(title_config.text_color[1]),
            glm::floatBitsToUint(title_config.text_color[2]),
            glm::floatBitsToUint(title_config.text_color[3])
    );

    text_color_picker->SetColour(color);
    alpha_slider->SetValue(glm::floatBitsToUint(title_config.text_color[3]));*/
  }

  void CallbackSpinControl(wxSpinDoubleEvent &event)
  {
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

  void CallbackTextControl(wxCommandEvent &event)
  {
    if (title_text_edit.get() == event.GetEventObject()) {
      title_config.SetTitleText(event.GetString().ToStdString());

      SendTitleConfig();
    }
  }

  void CallbackColorPickerControl(wxColourPickerEvent & /*event*/)
  {
    // if (event.GetId() == ID_COLOR_PICKER)
    //{
    /*auto color = event.GetColour();

    title_config.text_color[0] = glm::uintBitsToFloat(color.Red());
    title_config.text_color[1] = glm::uintBitsToFloat(color.Green());
    title_config.text_color[2] = glm::uintBitsToFloat(color.Blue());
    title_config.text_color[3] = 1.0f;*/
    // title_config.text_color[3] = glm::uintBitsToFloat(color.Alpha());
    //}

    // SendTitleConfig();
  }

  void CallbackSliderControl(wxCommandEvent & /*event*/)
  {
    // if (event.GetId() == ID_SLIDER)
    //{
    // title_config.text_color[3] = glm::uintBitsToFloat(event.GetInt());
    //}

    // SendTitleConfig();
  }

  wxWeakRef<wxPanel> wx_panel{nullptr};

  TitleConfig title_config;
  sigslot::signal<const TitleConfig &> signal_title_config;

  wxWeakRef<wxSpinCtrlDouble> frame_height_ctrl;
  wxWeakRef<wxSpinCtrlDouble> size_ratio_ctrl;

  wxWeakRef<wxTextCtrl> title_text_edit;

  wxWeakRef<wxColourPickerCtrl> text_color_picker;
  wxWeakRef<wxSlider> alpha_slider;
};
#endif // HOME_TITAN99_CODE_DECADE_SRC_GUI_TITLE_PANEL_HPP
