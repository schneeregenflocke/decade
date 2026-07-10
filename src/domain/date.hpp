#ifndef DATE_HPP
#define DATE_HPP

// Domain value object: a calendar date (proleptic Gregorian). The interface is
// date-library-free; all calendrical computation is delegated to the backend
// in detail/icu_date_backend.hpp (currently ICU), which is the single
// replacement point for switching libraries.
//
// A default-constructed Date is the explicit "invalid" state (the successor of
// boost's not_a_date_time): queries on it return neutral values, arithmetic on
// it stays invalid, and it compares less than every valid date.

#include <compare>
#include <cstdint>
#include <optional>

#include "detail/icu_date_backend.hpp"

enum class Weekday : std::uint8_t {
  kSunday = 0,
  kMonday,
  kTuesday,
  kWednesday,
  kThursday,
  kFriday,
  kSaturday
};

class Date {
 public:
  // Supported year range (kept from the previous Boost.DateTime backend so
  // existing projects and GUI limits keep their bounds).
  static constexpr int kMinYear = 1400;
  static constexpr int kMaxYear = 9999;

  Date() = default;  // invalid

  // Returns an invalid Date if the triple is no real calendar date or the
  // year lies outside [kMinYear, kMaxYear].
  [[nodiscard]] static Date FromYmd(int year, int month, int day) {
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

  [[nodiscard]] bool IsValid() const { return year_ != 0; }

  [[nodiscard]] int Year() const { return year_; }
  [[nodiscard]] int Month() const { return month_; }
  [[nodiscard]] int Day() const { return day_; }

  // 1-based (January 1st -> 1); 0 for an invalid date.
  [[nodiscard]] int DayOfYear() const {
    if (!IsValid()) {
      return 0;
    }
    return domain::detail::IcuCalendarBackend::Instance().DayOfYear(ToYmd());
  }

  // Precondition: IsValid(). An invalid date reports kSunday.
  [[nodiscard]] Weekday DayOfWeek() const {
    if (!IsValid()) {
      return Weekday::kSunday;
    }
    return static_cast<Weekday>(
        domain::detail::IcuCalendarBackend::Instance().DayOfWeek(ToYmd()));
  }

  // Invalid stays invalid; a result outside the supported year range is
  // invalid as well.
  [[nodiscard]] Date AddDays(int days) const {
    return ShiftedDate(domain::detail::IcuCalendarBackend::Instance().AddDays(
        ToYmd(), days));
  }

  // Day-of-month is pinned to the target month's length (Jan 31 + 1 month ->
  // Feb 28/29), matching the previous Boost.DateTime behavior.
  [[nodiscard]] Date AddMonths(int months) const {
    return ShiftedDate(
        domain::detail::IcuCalendarBackend::Instance().AddMonths(ToYmd(),
                                                                   months));
  }

  // Signed day distance `to - from`; 0 if either date is invalid.
  [[nodiscard]] static std::int64_t DaysBetween(const Date& from,
                                                const Date& to) {
    if (!from.IsValid() || !to.IsValid()) {
      return 0;
    }
    return domain::detail::IcuCalendarBackend::Instance().DaysBetween(
        from.ToYmd(), to.ToYmd());
  }

  // Lexicographic on (year, month, day); the invalid date is (0, 0, 0) and
  // therefore orders before every valid date.
  friend auto operator<=>(const Date&, const Date&) = default;

 private:
  [[nodiscard]] domain::detail::Ymd ToYmd() const {
    return {.year = year_, .month = month_, .day = day_};
  }

  [[nodiscard]] Date ShiftedDate(
      const std::optional<domain::detail::Ymd>& shifted) const {
    if (!IsValid() || !shifted.has_value()) {
      return {};
    }
    return FromYmd(shifted->year, shifted->month, shifted->day);
  }

  int year_{0};
  int month_{0};
  int day_{0};
};

// Number of days in a calendar year (365 or 366).
[[nodiscard]] inline std::int64_t DaysInYear(int year) {
  constexpr int kDecember = 12;
  constexpr int kLastDayOfDecember = 31;
  return Date::DaysBetween(Date::FromYmd(year, 1, 1),
                           Date::FromYmd(year, kDecember, kLastDayOfDecember)) +
         1;
}

#endif  // DATE_HPP
