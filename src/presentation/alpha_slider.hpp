#ifndef ALPHA_SLIDER_HPP
#define ALPHA_SLIDER_HPP

#include <QtCore/QPointer>
#include <QtWidgets/QWidget>
#include <functional>

class QLabel;
class QSlider;

// A byte-valued transparency slider with its value beside it. Qt draws no
// labels on a QSlider, and an alpha slider without a readable number is a guess
// — hence the pairing, in one place instead of three.
//
// Byte-valued (0…255) rather than a fraction, because that is the scale the
// colour beside it speaks in.
class AlphaSlider : public QWidget {
 public:
  static constexpr int kMax = 255;

  explicit AlphaSlider(QWidget* parent = nullptr);

  // Shows a value without reporting it — the way back from the state.
  void SetValue(int alpha);

  [[nodiscard]] int Value() const;

  void SetOnChanged(std::function<void()> on_changed);

 private:
  static constexpr int kValueLabelWidthPx = 32;

  void RefreshLabel();

  QPointer<QSlider> slider_;
  QPointer<QLabel> value_label_;
  std::function<void()> on_changed_;
};

#endif  // ALPHA_SLIDER_HPP
