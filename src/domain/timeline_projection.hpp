#ifndef TIMELINE_PROJECTION_HPP
#define TIMELINE_PROJECTION_HPP

// The seam between calendar time and calendar layout: which part of the span
// each row shows. The section builders place every element through it — a
// date sits as many days from the left edge as it lies after the first day of
// its row — so the rule that maps years onto rows stands here alone, once per
// CalendarView.

#include <cstddef>
#include <cstdint>
#include <vector>

#include "calendar_config.hpp"
#include "calendar_view.hpp"
#include "date.hpp"
#include "date_period.hpp"

// Splits a half-open period at every Jan 1 inside it. A period within a single
// year comes back unchanged as the only element.
//
// Precondition: `period` is non-null, so `Last().Year()` never lies before
// `Begin().Year()`.
[[nodiscard]] std::vector<DatePeriod> SplitAtYearBoundaries(
    const DatePeriod& period);

// What one column of the grid stands for, and so what the x-axis names.
enum class ColumnUnit : std::uint8_t { kMonth, kYear };

class TimelineProjection {
 public:
  explicit TimelineProjection(const CalendarConfig& config);

  [[nodiscard]] std::size_t RowCount() const;

  // Rows ascend from the first day of the span.
  [[nodiscard]] DatePeriod RowPeriod(std::size_t row) const;

  // Precondition: the date lies within the span.
  [[nodiscard]] std::size_t RowOf(const Date& date) const;

  // The days a row's width stands for. A year per row takes the longest year,
  // so every year starts at the left edge and a common year ends one day short
  // of the right one.
  [[nodiscard]] std::int64_t RowDays() const;

  [[nodiscard]] ColumnUnit Columns() const;

  // The parts of `period` inside the span, cut where a row ends; empty when
  // the period misses the span.
  [[nodiscard]] std::vector<DatePeriod> SplitAtRowBoundaries(
      const DatePeriod& period) const;

 private:
  static constexpr std::int64_t kDaysInLeapYear = 366;

  DatePeriod span_;
  CalendarView view_;
};

#endif  // TIMELINE_PROJECTION_HPP
