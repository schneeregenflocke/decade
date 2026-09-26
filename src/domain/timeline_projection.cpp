#include "timeline_projection.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "calendar_config.hpp"
#include "calendar_view.hpp"
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

TimelineProjection::TimelineProjection(const CalendarConfig& config)
    : span_(config.Period()), view_(config.View()) {}

std::size_t TimelineProjection::RowCount() const {
  switch (view_) {
    case CalendarView::kYearPerRow:
      return static_cast<std::size_t>(span_.End().Year() -
                                      span_.Begin().Year());
    case CalendarView::kAllYearsInOneRow:
      return 1;
  }
  return 0;
}

DatePeriod TimelineProjection::RowPeriod(std::size_t row) const {
  switch (view_) {
    case CalendarView::kYearPerRow: {
      const int year = span_.Begin().Year() + static_cast<int>(row);
      return {Date::FromYmd(year, 1, 1), Date::FromYmd(year + 1, 1, 1)};
    }
    case CalendarView::kAllYearsInOneRow:
      return span_;
  }
  return {};
}

std::size_t TimelineProjection::RowOf(const Date& date) const {
  switch (view_) {
    case CalendarView::kYearPerRow:
      return static_cast<std::size_t>(date.Year() - span_.Begin().Year());
    case CalendarView::kAllYearsInOneRow:
      return 0;
  }
  return 0;
}

std::int64_t TimelineProjection::RowDays() const {
  switch (view_) {
    case CalendarView::kYearPerRow:
      return kDaysInLeapYear;
    case CalendarView::kAllYearsInOneRow:
      return span_.LengthDays();
  }
  return 0;
}

ColumnUnit TimelineProjection::Columns() const {
  switch (view_) {
    case CalendarView::kYearPerRow:
      return ColumnUnit::kMonth;
    case CalendarView::kAllYearsInOneRow:
      return ColumnUnit::kYear;
  }
  return ColumnUnit::kMonth;
}

std::vector<DatePeriod> TimelineProjection::SplitAtRowBoundaries(
    const DatePeriod& period) const {
  const Date begin = std::max(period.Begin(), span_.Begin());
  const Date end = std::min(period.End(), span_.End());
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
