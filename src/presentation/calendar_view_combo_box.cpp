#include "calendar_view_combo_box.hpp"

#include <QtCore/qtypes.h>

#include <QtCore/QString>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QWidget>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string_view>

#include "../domain/calendar_view.hpp"

CalendarViewComboBox::CalendarViewComboBox(QWidget* parent)
    : QComboBox(parent) {
  for (const CalendarView view : kCalendarViews) {
    const std::string_view name = CalendarViewName(view);
    addItem(
        QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size())));
  }
}

CalendarView CalendarViewComboBox::View() const {
  return kCalendarViews.at(static_cast<std::size_t>(currentIndex()));
}

void CalendarViewComboBox::SetView(CalendarView view) {
  const auto* const found = std::ranges::find(kCalendarViews, view);
  setCurrentIndex(
      static_cast<int>(std::distance(kCalendarViews.begin(), found)));
}
