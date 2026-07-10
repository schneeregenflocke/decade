#ifndef DATE_PERIOD_HPP
#define DATE_PERIOD_HPP

// Domain value object: a date period, uniformly half-open [begin, end).
// `Last()` is the day before `end` (the last day inside the period),
// `LengthDays()` is `end - begin`, and a period that contains no day
// (`end <= begin`, or an invalid endpoint) is "null". A single-day entry is
// the period (d, d+1) with length 1 — null periods never carry data and are
// filtered out by the stores.
//
// The half-open form exists only inside the model. Users enter and see the
// end date *inclusively*; the conversion happens exactly once per direction
// at the user-facing boundaries (date table, CSV): PeriodFromInclusiveDates()
// on the way in, Last() on the way out.

#include <compare>
#include <cstdint>

#include "date.hpp"

class DatePeriod {
 public:
  DatePeriod() = default;  // both endpoints invalid

  DatePeriod(const Date& begin, const Date& end) : begin_(begin), end_(end) {}

  [[nodiscard]] const Date& Begin() const { return begin_; }
  [[nodiscard]] const Date& End() const { return end_; }

  // Last day inside the half-open period: the day before End().
  [[nodiscard]] Date Last() const { return end_.AddDays(-1); }

  [[nodiscard]] bool HasValidDates() const {
    return begin_.IsValid() && end_.IsValid();
  }

  // Boost semantics: a period is null when it contains no day (end <= begin).
  [[nodiscard]] bool IsNull() const {
    return !HasValidDates() || end_ <= begin_;
  }

  // Signed length `end - begin` in days; 0 if an endpoint is invalid.
  [[nodiscard]] std::int64_t LengthDays() const {
    return Date::DaysBetween(begin_, end_);
  }

  friend auto operator<=>(const DatePeriod&, const DatePeriod&) = default;

 private:
  Date begin_;
  Date end_;
};

// Boundary conversion from user-facing inclusive dates to the half-open
// model. The user thinks in "from .. to" where the to-date counts:
//   from=d,  to empty/invalid -> [d, d+1)        (single day)
//   from=d0, to=d1 >= d0      -> [d0, d1+1)
//   from invalid, or to < from -> null period    (unusable input)
[[nodiscard]] inline DatePeriod PeriodFromInclusiveDates(
    const Date& begin_date, const Date& last_date) {
  if (!begin_date.IsValid()) {
    return {};
  }
  const Date effective_last = last_date.IsValid() ? last_date : begin_date;
  if (effective_last < begin_date) {
    return {};
  }
  return {begin_date, effective_last.AddDays(1)};
}

#endif  // DATE_PERIOD_HPP
