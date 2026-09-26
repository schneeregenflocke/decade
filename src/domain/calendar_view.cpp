#include "calendar_view.hpp"

#include <string_view>

std::string_view CalendarViewName(CalendarView view) {
  switch (view) {
    case CalendarView::kYearPerRow:
      return "One Year per Row";
    case CalendarView::kAllYearsInOneRow:
      return "All Years in One Row";
  }
  return {};
}
