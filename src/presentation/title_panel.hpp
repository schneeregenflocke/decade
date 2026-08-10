#ifndef TITLE_PANEL_HPP
#define TITLE_PANEL_HPP

#include <QtCore/QPointer>
#include <QtWidgets/QWidget>
#include <sigslot/signal.hpp>

#include "../domain/title_config.hpp"
#include "alpha_slider.hpp"
#include "color_button.hpp"

class QDoubleSpinBox;

// The area height, font size and colour of the title. The title text itself has
// no field here: it gets edited in the canvas (a double click) and comes
// through as part of the received TitleConfig alone.
class TitleSetupPanel : public QWidget {
 public:
  explicit TitleSetupPanel(QWidget* parent);

  void SendDefaultValues();

  void SendTitleConfig();

  void ReceiveTitleConfig(const TitleConfig& incoming_title_config);

  sigslot::signal<const TitleConfig&>& SignalTitleConfig();

 private:
  // wxSpinCtrlDouble capped at 100 without being told; the number stays, so a
  // project written before the toolkit swap still fits its field.
  static constexpr double kAreaHeightMaxMm = 100.0;
  static constexpr double kFontSizeMinPt = 1.0;
  static constexpr double kFontSizeMaxPt = 500.0;
  static constexpr double kFontSizeIncrementPt = 1.0;
  static constexpr float kAlphaByteMax = 255.0F;

  void UpdateWidgetForSelection();

  // The colour button carries no alpha (that channel lives on the slider), so
  // the two get read together and written as one value.
  void ApplyColorFromWidgets();

  void SendUnlessLoading();

  TitleConfig title_config_;
  sigslot::signal<const TitleConfig&> signal_title_config_;

  QPointer<QDoubleSpinBox> area_height_spin_;
  QPointer<QDoubleSpinBox> font_size_spin_;
  QPointer<ColorButton> text_color_button_;
  QPointer<AlphaSlider> alpha_slider_;

  bool loading_{false};
};
#endif  // TITLE_PANEL_HPP
