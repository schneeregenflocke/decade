#ifndef TIMELINE_PROJECTION_HPP
#define TIMELINE_PROJECTION_HPP

// Domain layer (UI-agnostic, Boost-free).
//
// TimelineProjection is the seam between calendar *time* and calendar
// *layout*: it answers "which row does this date/year belong to" without the
// scene builder hard-coding the rule. In this first phase the rule is fixed to
// the historical behaviour — one row per calendar year, ascending — so the
// rendered output is unchanged. Later phases vary the row period (quarter,
// week, multi-year) and the axis direction behind this same interface.
//
// SplitAtYearBoundaries is the companion period operation: a half-open period
// that crosses one or more New Year boundaries is cut into per-year segments.
// It was previously inlined in DateEntryBarStore::ProcessBars, where it mixed
// layout knowledge into a domain store; it lives here now as the row-period
// split rule (currently: one calendar year).

#include <cstddef>
#include <vector>

#include "calendar_config.hpp"
#include "date.hpp"
#include "date_period.hpp"

// Splits a half-open period at calendar-year boundaries (each Jan 1) into
// consecutive per-year half-open sub-periods. A period contained within a
// single year is returned unchanged as the only element.
//
// Precondition: `period` is non-null (callers pass stored intervals, which are
// filtered to be well-formed). For a non-null period `Last().Year()` is never
// before `Begin().Year()`, so the segment count is non-negative.
[[nodiscard]] inline std::vector<DatePeriod> SplitAtYearBoundaries(
    const DatePeriod& period) {
  const auto span =
      static_cast<std::size_t>(period.Last().Year() - period.Begin().Year());

  std::vector<DatePeriod> split_periods;
  split_periods.push_back(period);

  for (std::size_t sub_index = 0; sub_index < span; ++sub_index) {
    const Date split_date =
        Date::FromYmd(split_periods[sub_index].Begin().Year() + 1, 1, 1);
    split_periods.emplace_back(split_date, split_periods[sub_index].End());
    split_periods[sub_index] =
        DatePeriod(split_periods[sub_index].Begin(), split_date);
  }

  return split_periods;
}

// Maps calendar years onto layout rows for a given span. Currently a thin,
// behaviour-preserving wrapper over CalendarSpan (row index == year offset,
// ascending); it exists to localise the row-vs-year coupling so later phases
// can change the mapping in one place.
class TimelineProjection {
 public:
  explicit TimelineProjection(const CalendarSpan& span) : span_(span) {}

  // Number of layout rows = number of years in the span.
  [[nodiscard]] std::size_t RowCount() const {
    return span_.GetSpanLengthYears();
  }

  // Calendar year shown in the given row (row 0 == first year of the span).
  [[nodiscard]] int YearForRow(std::size_t row) const {
    return span_.GetYear(row);
  }

  // Row index for a calendar year. Precondition: the year lies within the span.
  [[nodiscard]] std::size_t RowForYear(int year) const {
    return static_cast<std::size_t>(year - span_.GetSpanLimitsYears().at(0));
  }

 private:
  CalendarSpan span_;
};

#endif  // TIMELINE_PROJECTION_HPP
