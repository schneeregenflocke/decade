#include "date.hpp"

#include <cstdint>
#include <optional>

#include "detail/icu_date_backend.hpp"

Date Date::FromYmd(int year, int month, int day) {
  if (year < kMinYear || year > kMaxYear) {
    return {};
  }
  const domain::detail::Ymd ymd{.year = year, .month = month, .day = day};
  if (!domain::detail::IcuCalendarBackend::Instance().IsValidDate(ymd)) {
    return {};
  }
  Date date;
  date.year_ = year;
  date.month_ = month;
  date.day_ = day;
  return date;
}

bool Date::IsValid() const { return year_ != 0; }

int Date::Year() const { return year_; }

int Date::Month() const { return month_; }

int Date::Day() const { return day_; }

int Date::DayOfYear() const {
  if (!IsValid()) {
    return 0;
  }
  return domain::detail::IcuCalendarBackend::Instance().DayOfYear(ToYmd());
}

Weekday Date::DayOfWeek() const {
  if (!IsValid()) {
    return Weekday::kSunday;
  }
  return static_cast<Weekday>(
      domain::detail::IcuCalendarBackend::Instance().DayOfWeek(ToYmd()));
}

Date Date::AddDays(int days) const {
  return ShiftedDate(
      domain::detail::IcuCalendarBackend::Instance().AddDays(ToYmd(), days));
}

Date Date::AddMonths(int months) const {
  return ShiftedDate(domain::detail::IcuCalendarBackend::Instance().AddMonths(
      ToYmd(), months));
}

std::int64_t Date::DaysBetween(const Date& from, const Date& to) {
  if (!from.IsValid() || !to.IsValid()) {
    return 0;
  }
  return domain::detail::IcuCalendarBackend::Instance().DaysBetween(
      from.ToYmd(), to.ToYmd());
}

domain::detail::Ymd Date::ToYmd() const {
  return {.year = year_, .month = month_, .day = day_};
}

Date Date::ShiftedDate(
    const std::optional<domain::detail::Ymd>& shifted) const {
  if (!IsValid() || !shifted.has_value()) {
    return {};
  }
  return FromYmd(shifted->year, shifted->month, shifted->day);
}

std::int64_t DaysInYear(int year) {
  constexpr int kDecember = 12;
  constexpr int kLastDayOfDecember = 31;
  return Date::DaysBetween(Date::FromYmd(year, 1, 1),
                           Date::FromYmd(year, kDecember, kLastDayOfDecember)) +
         1;
}
