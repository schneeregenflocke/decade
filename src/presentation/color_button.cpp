#include "color_button.hpp"

#include <QtCore/Qt>
#include <QtGui/QColor>
#include <QtGui/QIcon>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include <cstddef>
#include <functional>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <utility>

#include "../domain/color_palette.hpp"
#include "casts.hpp"

namespace {

// Six rows of eight. Qt keeps the standard swatches globally rather than per
// dialogue, and its own values are a mechanical sampling of the RGB cube — the
// project palette is what the bar groups are actually drawn in.
constexpr int kStandardColorCount = 48;

void OfferProjectPalette() {
  for (int index = 0; index < kStandardColorCount; ++index) {
    const glm::vec3 color =
        palette::CategoricalColor(static_cast<std::size_t>(index));
    QColorDialog::setStandardColor(index, ToQColor(glm::vec4{color, 1.0F}));
  }
}

}  // namespace

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
  OfferProjectPalette();
  // Qt's own dialogue rather than the platform one: the platform helpers pass
  // the colour and the alpha flag on and nothing else, so a native dialogue
  // would never show the palette set above.
  const QColor picked = QColorDialog::getColor(
      color_, this, "Choose Colour", QColorDialog::DontUseNativeDialog);
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
