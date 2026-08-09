#ifndef TITLE_PANEL_HPP
#define TITLE_PANEL_HPP

#include <QtCore/QPointer>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QWidget>
#include <glm/vec4.hpp>
#include <sigslot/signal.hpp>

#include "../domain/title_config.hpp"
#include "alpha_slider.hpp"
#include "casts.hpp"
#include "color_button.hpp"
#include "make_owned.hpp"

// The area height, font size and colour of the title. The title text itself has
// no field here: it gets edited in the canvas (a double click) and comes
// through as part of the received TitleConfig alone.
class TitleSetupPanel : public QWidget {
 public:
  explicit TitleSetupPanel(QWidget* parent) : QWidget(parent) {
    constexpr int kBorderPx = 5;

    area_height_spin_ = MakeOwned<QDoubleSpinBox>(this);
    area_height_spin_->setDecimals(2);
    area_height_spin_->setRange(0.0, kAreaHeightMaxMm);

    font_size_spin_ = MakeOwned<QDoubleSpinBox>(this);
    font_size_spin_->setDecimals(1);
    font_size_spin_->setRange(kFontSizeMinPt, kFontSizeMaxPt);
    font_size_spin_->setSingleStep(kFontSizeIncrementPt);

    text_color_button_ = MakeOwned<ColorButton>(this);
    alpha_slider_ = MakeOwned<AlphaSlider>(this);

    auto* form_layout = MakeOwned<QFormLayout>();
    form_layout->setContentsMargins(kBorderPx, kBorderPx, kBorderPx, kBorderPx);
    form_layout->addRow("Area Height", area_height_spin_.data());
    form_layout->addRow("Font Size (pt)", font_size_spin_.data());
    form_layout->addRow("Color", text_color_button_.data());
    form_layout->addRow("Transparency", alpha_slider_.data());
    setLayout(form_layout);

    connect(area_height_spin_.data(), &QDoubleSpinBox::valueChanged, this,
            [this](double value) {
              title_config_.SetAreaHeight(static_cast<float>(value));
              SendUnlessLoading();
            });
    connect(font_size_spin_.data(), &QDoubleSpinBox::valueChanged, this,
            [this](double value) {
              title_config_.SetFontSizePoints(static_cast<float>(value));
              SendUnlessLoading();
            });
    text_color_button_->SetOnChanged([this]() { ApplyColorFromWidgets(); });
    alpha_slider_->SetOnChanged([this]() { ApplyColorFromWidgets(); });
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
  // wxSpinCtrlDouble capped at 100 without being told; the number stays, so a
  // project written before the toolkit swap still fits its field.
  static constexpr double kAreaHeightMaxMm = 100.0;
  static constexpr double kFontSizeMinPt = 1.0;
  static constexpr double kFontSizeMaxPt = 500.0;
  static constexpr double kFontSizeIncrementPt = 1.0;
  static constexpr float kAlphaByteMax = 255.0F;

  void UpdateWidgetForSelection() {
    // Loading the widgets fires their change signals; without the guard every
    // received config would be sent straight back out.
    loading_ = true;
    area_height_spin_->setValue(
        static_cast<double>(title_config_.AreaHeight()));
    font_size_spin_->setValue(
        static_cast<double>(title_config_.FontSizePoints()));

    const QColor color = ToQColor(title_config_.TextColor());
    text_color_button_->SetColor(color);
    alpha_slider_->SetValue(color.alpha());
    loading_ = false;
  }

  // The colour button carries no alpha (that channel lives on the slider), so
  // the two get read together and written as one value.
  void ApplyColorFromWidgets() {
    glm::vec4 text_color = ToGlmVec4(text_color_button_->Color());
    text_color[3] = static_cast<float>(alpha_slider_->Value()) / kAlphaByteMax;
    title_config_.SetTextColor(text_color);
    SendUnlessLoading();
  }

  void SendUnlessLoading() {
    if (!loading_) {
      SendTitleConfig();
    }
  }

  TitleConfig title_config_;
  sigslot::signal<const TitleConfig&> signal_title_config_;

  QPointer<QDoubleSpinBox> area_height_spin_;
  QPointer<QDoubleSpinBox> font_size_spin_;
  QPointer<ColorButton> text_color_button_;
  QPointer<AlphaSlider> alpha_slider_;

  bool loading_{false};
};
#endif  // TITLE_PANEL_HPP
