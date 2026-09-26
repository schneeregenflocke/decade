#ifndef CALENDAR_VIEW_HPP
#define CALENDAR_VIEW_HPP

#include <array>
#include <cstdint>
#include <string_view>

// The ways the calendar lays out the same entries. They differ in how many
// years share a row; TimelineProjection holds what follows from that.
enum class CalendarView : std::uint8_t { kYearPerRow, kAllYearsInOneRow };

// Every view, in the order a chooser offers them.
inline constexpr std::array kCalendarViews{CalendarView::kYearPerRow,
                                           CalendarView::kAllYearsInOneRow};

[[nodiscard]] std::string_view CalendarViewName(CalendarView view);

#endif  // CALENDAR_VIEW_HPP
