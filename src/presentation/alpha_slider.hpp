#ifndef ALPHA_SLIDER_HPP
#define ALPHA_SLIDER_HPP

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>
#include <QtWidgets/QWidget>
#include <functional>
#include <utility>

#include "make_owned.hpp"

// A byte-valued transparency slider with its value beside it. Qt draws no
// labels on a QSlider, and an alpha slider without a readable number is a guess
// — hence the pairing, in one place instead of three.
//
// Byte-valued (0…255) rather than a fraction, because that is the scale the
// colour beside it speaks in.
class AlphaSlider : public QWidget {
 public:
  static constexpr int kMax = 255;

  explicit AlphaSlider(QWidget* parent = nullptr) : QWidget(parent) {
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

  // Shows a value without reporting it — the way back from the state.
  void SetValue(int alpha) {
    const QSignalBlocker blocker(slider_);
    slider_->setValue(alpha);
    RefreshLabel();
  }

  [[nodiscard]] int Value() const { return slider_->value(); }

  void SetOnChanged(std::function<void()> on_changed) {
    on_changed_ = std::move(on_changed);
  }

 private:
  static constexpr int kValueLabelWidthPx = 32;

  void RefreshLabel() { value_label_->setText(QString::number(Value())); }

  QPointer<QSlider> slider_;
  QPointer<QLabel> value_label_;
  std::function<void()> on_changed_;
};

#endif  // ALPHA_SLIDER_HPP
