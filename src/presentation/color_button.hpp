#ifndef COLOR_BUTTON_HPP
#define COLOR_BUTTON_HPP

#include <QtCore/Qt>
#include <QtGui/QColor>
#include <QtWidgets/QPushButton>
#include <functional>

// A button that shows a colour and opens the colour dialogue on a click — Qt
// carries no colour-picker widget of its own, only the modal QColorDialog.
//
// The alpha channel stays out on purpose: every user of this sits beside a
// transparency slider that owns the fourth channel, so a dialogue offering
// alpha as well would be a second write path onto the same value.
class ColorButton : public QPushButton {
 public:
  explicit ColorButton(QWidget* parent = nullptr);

  // Shows a colour without reporting it — the way back from the state.
  void SetColor(const QColor& color);

  [[nodiscard]] const QColor& Color() const;

  void SetOnChanged(std::function<void()> on_changed);

 private:
  static constexpr int kSwatchWidthPx = 32;
  static constexpr int kSwatchHeightPx = 16;

  void PickColor();

  // With a border, because a white swatch on a light button is otherwise
  // indistinguishable from no swatch at all.
  void RefreshSwatch();

  QColor color_{Qt::black};
  std::function<void()> on_changed_;
};

#endif  // COLOR_BUTTON_HPP
