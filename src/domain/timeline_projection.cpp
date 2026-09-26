#include "timeline_projection.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

#include "calendar_config.hpp"
#include "date.hpp"
#include "date_period.hpp"

std::vector<DatePeriod> SplitAtYearBoundaries(const DatePeriod& period) {
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

TimelineProjection::TimelineProjection(const CalendarSpan& span)
    : span_(span) {}

std::size_t TimelineProjection::RowCount() const { return span_.YearCount(); }

DatePeriod TimelineProjection::RowPeriod(std::size_t row) const {
  const int year = span_.FirstYear() + static_cast<int>(row);
  return {Date::FromYmd(year, 1, 1), Date::FromYmd(year + 1, 1, 1)};
}

std::size_t TimelineProjection::RowOf(const Date& date) const {
  return static_cast<std::size_t>(date.Year() - span_.FirstYear());
}

std::vector<DatePeriod> TimelineProjection::SplitAtRowBoundaries(
    const DatePeriod& period) const {
  const Date begin = std::max(period.Begin(), span_.Period().Begin());
  const Date end = std::min(period.End(), span_.Period().End());
  std::vector<DatePeriod> pieces;
  if (end <= begin) {
    return pieces;
  }
  for (std::size_t row = RowOf(begin); row <= RowOf(end.AddDays(-1)); ++row) {
    const DatePeriod row_period = RowPeriod(row);
    pieces.emplace_back(std::max(begin, row_period.Begin()),
                        std::min(end, row_period.End()));
  }
  return pieces;
}
