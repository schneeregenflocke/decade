#ifndef CALENDAR_VIEW_COMBO_BOX_HPP
#define CALENDAR_VIEW_COMBO_BOX_HPP

#include <QtWidgets/QComboBox>
#include <QtWidgets/QWidget>

#include "../domain/calendar_view.hpp"

// Offers every calendar view by name, in the order kCalendarViews lists them,
// so a view added there shows up here without a change.
class CalendarViewComboBox : public QComboBox {
 public:
  explicit CalendarViewComboBox(QWidget* parent);

  [[nodiscard]] CalendarView View() const;
  void SetView(CalendarView view);
};
#endif  // CALENDAR_VIEW_COMBO_BOX_HPP
