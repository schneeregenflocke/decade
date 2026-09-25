#include "elided_path_label.hpp"

#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QWidget>

ElidedPathLabel::ElidedPathLabel(QWidget* parent) : QLabel(parent) {
  setMinimumWidth(1);
  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

void ElidedPathLabel::SetFullText(const QString& text) {
  full_text_ = text;
  RefreshElided();
}

void ElidedPathLabel::resizeEvent(QResizeEvent* event) {
  QLabel::resizeEvent(event);
  RefreshElided();
}

void ElidedPathLabel::RefreshElided() {
  setText(fontMetrics().elidedText(full_text_, Qt::ElideLeft, width()));
}
