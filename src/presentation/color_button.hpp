#ifndef COLOR_BUTTON_HPP
#define COLOR_BUTTON_HPP

#include <QtGui/QColor>
#include <QtGui/QIcon>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QPushButton>
#include <functional>
#include <utility>

// A button that shows a colour and opens the colour dialogue on a click — Qt
// carries no colour-picker widget of its own, only the modal QColorDialog.
//
// The alpha channel stays out on purpose: every user of this sits beside a
// transparency slider that owns the fourth channel, so a dialogue offering
// alpha as well would be a second write path onto the same value.
class ColorButton : public QPushButton {
 public:
  explicit ColorButton(QWidget* parent = nullptr) : QPushButton(parent) {
    RefreshSwatch();
    connect(this, &QPushButton::clicked, this, [this]() { PickColor(); });
  }

  // Shows a colour without reporting it — the way back from the state.
  void SetColor(const QColor& color) {
    color_ = color;
    RefreshSwatch();
  }

  [[nodiscard]] const QColor& Color() const { return color_; }

  void SetOnChanged(std::function<void()> on_changed) {
    on_changed_ = std::move(on_changed);
  }

 private:
  static constexpr int kSwatchWidthPx = 32;
  static constexpr int kSwatchHeightPx = 16;

  void PickColor() {
    const QColor picked = QColorDialog::getColor(color_, this, "Choose Colour");
    if (!picked.isValid()) {
      return;
    }
    // The picker carries no alpha; whoever holds the slider keeps that channel.
    SetColor(
        QColor(picked.red(), picked.green(), picked.blue(), color_.alpha()));
    if (on_changed_) {
      on_changed_();
    }
  }

  // With a border, because a white swatch on a light button is otherwise
  // indistinguishable from no swatch at all.
  void RefreshSwatch() {
    QPixmap swatch(kSwatchWidthPx, kSwatchHeightPx);
    swatch.fill(QColor(color_.red(), color_.green(), color_.blue()));
    QPainter painter(&swatch);
    painter.setPen(Qt::darkGray);
    painter.drawRect(0, 0, kSwatchWidthPx - 1, kSwatchHeightPx - 1);
    painter.end();
    setIcon(QIcon(swatch));
    setText(color_.name(QColor::HexRgb));
  }

  QColor color_{Qt::black};
  std::function<void()> on_changed_;
};

#endif  // COLOR_BUTTON_HPP
