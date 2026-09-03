#include "color_button.hpp"

#include <QtCore/Qt>
#include <QtGui/QColor>
#include <QtGui/QIcon>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include <functional>
#include <utility>

ColorButton::ColorButton(QWidget* parent) : QPushButton(parent) {
  RefreshSwatch();
  connect(this, &QPushButton::clicked, this, [this]() { PickColor(); });
}

void ColorButton::SetColor(const QColor& color) {
  color_ = color;
  RefreshSwatch();
}

const QColor& ColorButton::Color() const { return color_; }

void ColorButton::SetOnChanged(std::function<void()> on_changed) {
  on_changed_ = std::move(on_changed);
}

void ColorButton::PickColor() {
  const QColor picked = QColorDialog::getColor(color_, this, "Choose Colour");
  if (!picked.isValid()) {
    return;
  }
  // The picker carries no alpha; whoever holds the slider keeps that channel.
  SetColor(QColor(picked.red(), picked.green(), picked.blue(), color_.alpha()));
  if (on_changed_) {
    on_changed_();
  }
}

void ColorButton::RefreshSwatch() {
  QPixmap swatch(kSwatchWidthPx, kSwatchHeightPx);
  swatch.fill(QColor(color_.red(), color_.green(), color_.blue()));
  QPainter painter(&swatch);
  painter.setPen(Qt::darkGray);
  painter.drawRect(0, 0, kSwatchWidthPx - 1, kSwatchHeightPx - 1);
  painter.end();
  setIcon(QIcon(swatch));
  setText(color_.name(QColor::HexRgb));
}
