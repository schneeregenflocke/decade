#include "restoring_int_validator.hpp"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtGui/QIntValidator>
#include <functional>
#include <utility>

RestoringIntValidator::RestoringIntValidator(int bottom, int top,
                                             std::function<int()> in_force,
                                             QObject* parent)
    : QIntValidator(bottom, top, parent), in_force_(std::move(in_force)) {}

void RestoringIntValidator::fixup(QString& input) const {
  input = QString::number(in_force_());
}
