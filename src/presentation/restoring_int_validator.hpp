#ifndef RESTORING_INT_VALIDATOR_HPP
#define RESTORING_INT_VALIDATOR_HPP

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtGui/QIntValidator>
#include <functional>

// An integer range whose invalid leftovers fall back to the value in force.
// QLineEdit calls fixup when Enter or a focus change meets input outside the
// range, so the field never shows one number while another one applies.
class RestoringIntValidator : public QIntValidator {
 public:
  RestoringIntValidator(int bottom, int top, std::function<int()> in_force,
                        QObject* parent);

  void fixup(QString& input) const override;

 private:
  std::function<int()> in_force_;
};
#endif  // RESTORING_INT_VALIDATOR_HPP
