#ifndef SIZING_MODE_SWITCH_HPP
#define SIZING_MODE_SWITCH_HPP

#include <QtCore/QPointer>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>
#include <functional>

// How one axis of the calendar takes its size: two exclusive buttons, "Fit
// page" and "Fixed", side by side as a segmented control, so the mode reads at
// a glance where a check box would need its label.
class SizingModeSwitch : public QWidget {
 public:
  explicit SizingModeSwitch(QWidget* parent);

  [[nodiscard]] bool IsFixed() const;

  // Sets the mode without calling back, as loading a config does.
  void SetFixed(bool fixed);

  // Called when the user switches, with the new mode.
  void SetOnSwitched(std::function<void(bool fixed)> on_switched);

 private:
  [[nodiscard]] QToolButton* SegmentButton(const QString& text);

  QPointer<QToolButton> fit_;
  QPointer<QToolButton> fixed_;
  std::function<void(bool)> on_switched_;
};

#endif  // SIZING_MODE_SWITCH_HPP
