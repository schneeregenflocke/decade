#include "timeline_projection.hpp"

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

std::size_t TimelineProjection::RowCount() const {
  return span_.GetSpanLengthYears();
}

int TimelineProjection::YearForRow(std::size_t row) const {
  return span_.GetYear(row);
}

std::size_t TimelineProjection::RowForYear(int year) const {
  return static_cast<std::size_t>(year - span_.GetSpanLimitsYears().at(0));
}
