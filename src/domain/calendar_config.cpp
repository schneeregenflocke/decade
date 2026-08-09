#include "calendar_config.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "date.hpp"
#include "date_period.hpp"

CalendarSpan::CalendarSpan()
    : span_(Date::FromYmd(kDefaultStartYear, 1, 1),
            Date::FromYmd(kDefaultEndYear, 1, 1)) {}

void CalendarSpan::SetSpan(YearSpan span_years) {
  // It never produces a null span: the half-open end Jan 1 (last + 1) needs a
  // representable year — the last selectable calendar year is therefore
  // kMaxYear - 1, and a last year before the first year gets raised to the
  // first year (a span of exactly one year).
  const int first_year =
      std::clamp(span_years.first_year, Date::kMinYear, Date::kMaxYear - 1);
  const int last_year =
      std::clamp(span_years.last_year, first_year, Date::kMaxYear - 1);

  span_ = DatePeriod(Date::FromYmd(first_year, 1, 1),
                     Date::FromYmd(last_year + 1, 1, 1));
}

bool CalendarSpan::IsValidSpan() const { return !span_.IsNull(); }

std::size_t CalendarSpan::GetSpanLengthYears() const {
  if (!IsValidSpan()) {
    throw std::runtime_error("Not valid calendar span!");
  }
  return static_cast<std::size_t>(span_.End().Year() - span_.Begin().Year());
}

std::array<int, 2> CalendarSpan::GetSpanLimitsYears() const {
  return std::array<int, 2>{span_.Begin().Year(), span_.Last().Year()};
}

std::array<Date, 2> CalendarSpan::GetSpanLimitsDate() const {
  return std::array<Date, 2>{span_.Begin(), span_.Last()};
}

std::int64_t CalendarSpan::GetSpanLengthDays() const {
  return span_.LengthDays();
}

int CalendarSpan::GetYear(const std::size_t index) const {
  const int year = span_.Begin().Year() + static_cast<int>(index);

  if (!IsInSpan(year)) {
    throw std::logic_error("Year not in span!");
  }

  return year;
}

bool CalendarSpan::IsInSpan(const int year) const {
  return year >= span_.Begin().Year() && year <= span_.Last().Year();
}

bool CalendarConfig::IsAutoCalendarSpan() const { return auto_calendar_span_; }

void CalendarConfig::SetAutoCalendarSpan(bool auto_span) {
  auto_calendar_span_ = auto_span;
}

const std::vector<float>& CalendarConfig::GetSpacingProportions() const {
  return spacing_proportions_;
}

void CalendarConfig::SetSpacingProportions(
    const std::vector<float>& proportions) {
  spacing_proportions_ = proportions;
}
