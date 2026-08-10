#include "title_panel.hpp"

#include <QtGui/QColor>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QWidget>
#include <glm/ext/vector_float4.hpp>
#include <sigslot/signal.hpp>

#include "../domain/title_config.hpp"
#include "alpha_slider.hpp"
#include "casts.hpp"
#include "color_button.hpp"
#include "make_owned.hpp"

TitleSetupPanel::TitleSetupPanel(QWidget* parent) : QWidget(parent) {
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

void TitleSetupPanel::SendDefaultValues() { SendTitleConfig(); }

void TitleSetupPanel::SendTitleConfig() { signal_title_config_(title_config_); }

void TitleSetupPanel::ReceiveTitleConfig(
    const TitleConfig& incoming_title_config) {
  title_config_ = incoming_title_config;
  UpdateWidgetForSelection();
}

sigslot::signal<const TitleConfig&>& TitleSetupPanel::SignalTitleConfig() {
  return signal_title_config_;
}

void TitleSetupPanel::UpdateWidgetForSelection() {
  // Loading the widgets fires their change signals; without the guard every
  // received config would be sent straight back out.
  loading_ = true;
  area_height_spin_->setValue(static_cast<double>(title_config_.AreaHeight()));
  font_size_spin_->setValue(
      static_cast<double>(title_config_.FontSizePoints()));

  const QColor color = ToQColor(title_config_.TextColor());
  text_color_button_->SetColor(color);
  alpha_slider_->SetValue(color.alpha());
  loading_ = false;
}

void TitleSetupPanel::ApplyColorFromWidgets() {
  glm::vec4 text_color = ToGlmVec4(text_color_button_->Color());
  text_color[3] = static_cast<float>(alpha_slider_->Value()) / kAlphaByteMax;
  title_config_.SetTextColor(text_color);
  SendUnlessLoading();
}

void TitleSetupPanel::SendUnlessLoading() {
  if (!loading_) {
    SendTitleConfig();
  }
}
