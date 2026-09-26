#include "sizing_mode_switch.hpp"

#include <QtCore/QSignalBlocker>
#include <QtCore/QString>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>
#include <functional>
#include <utility>

#include "make_owned.hpp"

SizingModeSwitch::SizingModeSwitch(QWidget* parent)
    : QWidget(parent),
      fit_(SegmentButton("Fit page")),
      fixed_(SegmentButton("Fixed")) {
  auto* group = MakeOwned<QButtonGroup>(this);
  group->setExclusive(true);
  group->addButton(fit_);
  group->addButton(fixed_);
  fit_->setChecked(true);

  auto* layout = MakeOwned<QHBoxLayout>();
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(fit_);
  layout->addWidget(fixed_);
  setLayout(layout);

  connect(fixed_.data(), &QAbstractButton::toggled, this, [this](bool fixed) {
    if (on_switched_) {
      on_switched_(fixed);
    }
  });
}

bool SizingModeSwitch::IsFixed() const { return fixed_->isChecked(); }

void SizingModeSwitch::SetFixed(bool fixed) {
  const QSignalBlocker block_fit(fit_);
  const QSignalBlocker block_fixed(fixed_);
  (fixed ? fixed_ : fit_)->setChecked(true);
}

void SizingModeSwitch::SetOnSwitched(
    std::function<void(bool fixed)> on_switched) {
  on_switched_ = std::move(on_switched);
}

QToolButton* SizingModeSwitch::SegmentButton(const QString& text) {
  auto* button = MakeOwned<QToolButton>(this);
  button->setText(text);
  button->setCheckable(true);
  button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  return button;
}
