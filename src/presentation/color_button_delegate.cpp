#include "color_button_delegate.hpp"

#include <QtCore/qtmetamacros.h>

#include <QtCore/QAbstractItemModel>
#include <QtCore/QEvent>
#include <QtCore/QModelIndex>
#include <QtCore/QObject>
#include <QtCore/QRect>
#include <QtCore/QSize>
#include <QtCore/Qt>
#include <QtGui/QColor>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOptionButton>
#include <QtWidgets/QStyleOptionViewItem>
#include <QtWidgets/QStyledItemDelegate>

ColorButtonDelegate::ColorButtonDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

void ColorButtonDelegate::paint(QPainter* painter,
                                const QStyleOptionViewItem& option,
                                const QModelIndex& index) const {
  QStyleOptionButton button;
  button.rect = option.rect.adjusted(kButtonMarginPx, kButtonMarginPx,
                                     -kButtonMarginPx, -kButtonMarginPx);
  button.state = QStyle::State_Enabled | QStyle::State_Raised;
  const QStyle* style =
      option.widget != nullptr ? option.widget->style() : QApplication::style();
  style->drawControl(QStyle::CE_PushButton, &button, painter, option.widget);

  // With a border, because a white swatch on a light button is otherwise
  // indistinguishable from no swatch at all.
  QRect swatch(0, 0, kSwatchWidthPx, kSwatchHeightPx);
  swatch.moveCenter(button.rect.center());
  painter->save();
  painter->fillRect(swatch, index.data(Qt::UserRole).value<QColor>());
  painter->setPen(option.palette.color(QPalette::WindowText));
  painter->drawRect(swatch);
  painter->restore();
}

QSize ColorButtonDelegate::sizeHint(const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const {
  QSize hint = QStyledItemDelegate::sizeHint(option, index);
  hint.setWidth(kSwatchWidthPx + (kButtonPaddingPx * 2));
  return hint;
}

bool ColorButtonDelegate::editorEvent(QEvent* event,
                                      QAbstractItemModel* /*model*/,
                                      const QStyleOptionViewItem& option,
                                      const QModelIndex& index) {
  if (event->type() != QEvent::MouseButtonRelease) {
    return false;
  }
  const auto* mouse = dynamic_cast<const QMouseEvent*>(event);
  if (mouse == nullptr || mouse->button() != Qt::LeftButton ||
      !option.rect.contains(mouse->position().toPoint())) {
    return false;
  }
  emit Clicked(index);
  return true;
}
