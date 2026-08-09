#include "date_period.hpp"

#include <cstdint>

#include "date.hpp"

DatePeriod::DatePeriod(const Date& begin, const Date& end)
    : begin_(begin), end_(end) {}

const Date& DatePeriod::Begin() const { return begin_; }

const Date& DatePeriod::End() const { return end_; }

Date DatePeriod::Last() const { return end_.AddDays(-1); }

bool DatePeriod::HasValidDates() const {
  return begin_.IsValid() && end_.IsValid();
}

bool DatePeriod::IsNull() const { return !HasValidDates() || end_ <= begin_; }

std::int64_t DatePeriod::LengthDays() const {
  return Date::DaysBetween(begin_, end_);
}

DatePeriod PeriodFromInclusiveDates(const Date& begin_date,
                                    const Date& last_date) {
  if (!begin_date.IsValid()) {
    return {};
  }
  const Date effective_last = last_date.IsValid() ? last_date : begin_date;
  if (effective_last < begin_date) {
    return {};
  }
  return {begin_date, effective_last.AddDays(1)};
}
