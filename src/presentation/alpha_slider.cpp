#include "alpha_slider.hpp"

#include <QtCore/QSignalBlocker>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>
#include <QtWidgets/QWidget>
#include <functional>
#include <utility>

#include "make_owned.hpp"

AlphaSlider::AlphaSlider(QWidget* parent) : QWidget(parent) {
  slider_ = MakeOwned<QSlider>(Qt::Horizontal, this);
  slider_->setRange(0, kMax);
  slider_->setValue(kMax);

  value_label_ = MakeOwned<QLabel>(this);
  value_label_->setMinimumWidth(kValueLabelWidthPx);

  auto* layout = MakeOwned<QHBoxLayout>();
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(slider_, 1);
  layout->addWidget(value_label_);
  setLayout(layout);

  RefreshLabel();
  connect(slider_.data(), &QSlider::valueChanged, this, [this](int) {
    RefreshLabel();
    if (on_changed_) {
      on_changed_();
    }
  });
}

void AlphaSlider::SetValue(int alpha) {
  const QSignalBlocker blocker(slider_);
  slider_->setValue(alpha);
  RefreshLabel();
}

int AlphaSlider::Value() const { return slider_->value(); }

void AlphaSlider::SetOnChanged(std::function<void()> on_changed) {
  on_changed_ = std::move(on_changed);
}

void AlphaSlider::RefreshLabel() {
  value_label_->setText(QString::number(Value()));
}
