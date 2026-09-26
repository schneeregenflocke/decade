#ifndef TIMELINE_PROJECTION_HPP
#define TIMELINE_PROJECTION_HPP

// The seam between calendar time and calendar layout: which part of the span
// each row shows. The section builders place every element through it — a
// date sits as many days from the left edge as it lies after the first day of
// its row — so the rule that maps years onto rows stands here alone.

#include <cstddef>
#include <vector>

#include "calendar_config.hpp"
#include "date.hpp"
#include "date_period.hpp"

// Splits a half-open period at every Jan 1 inside it. A period within a single
// year comes back unchanged as the only element.
//
// Precondition: `period` is non-null, so `Last().Year()` never lies before
// `Begin().Year()`.
[[nodiscard]] std::vector<DatePeriod> SplitAtYearBoundaries(
    const DatePeriod& period);

class TimelineProjection {
 public:
  explicit TimelineProjection(const CalendarSpan& span);

  [[nodiscard]] std::size_t RowCount() const;

  // Rows ascend from the first day of the span.
  [[nodiscard]] DatePeriod RowPeriod(std::size_t row) const;

  // Precondition: the date lies within the span.
  [[nodiscard]] std::size_t RowOf(const Date& date) const;

  // The parts of `period` inside the span, cut where a row ends; empty when
  // the period misses the span.
  [[nodiscard]] std::vector<DatePeriod> SplitAtRowBoundaries(
      const DatePeriod& period) const;

 private:
  CalendarSpan span_;
};

#endif  // TIMELINE_PROJECTION_HPP
