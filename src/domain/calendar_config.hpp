#ifndef CALENDAR_CONFIG_HPP
#define CALENDAR_CONFIG_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "date.hpp"
#include "date_period.hpp"

// Pure domain value: the year span of the calendar, stored as the half-open
// period [Jan 1 first_year, Jan 1 last_year + 1). There is no serialization or
// signal here -> copyable. Persistence lives non-intrusively in the
// infrastructure layer.
class CalendarSpan {
 public:
  struct YearSpan {
    int first_year;
    int last_year;
  };

  CalendarSpan();

  void SetSpan(YearSpan span_years);

  [[nodiscard]] bool IsValidSpan() const;

  [[nodiscard]] std::size_t GetSpanLengthYears() const;

  [[nodiscard]] std::array<int, 2> GetSpanLimitsYears() const;

  [[nodiscard]] std::array<Date, 2> GetSpanLimitsDate() const;

  [[nodiscard]] std::int64_t GetSpanLengthDays() const;

  [[nodiscard]] int GetYear(std::size_t index) const;

  [[nodiscard]] bool IsInSpan(int year) const;

 private:
  static constexpr int kDefaultStartYear = 2000;
  static constexpr int kDefaultEndYear = 2010;

  DatePeriod span_;
};

// Pure domain value: the full calendar configuration. Rule of Zero (no signal,
// no hand-written copy/move) -> freely and correctly copyable.
class CalendarConfig : public CalendarSpan {
 public:
  [[nodiscard]] bool IsAutoCalendarSpan() const;
  void SetAutoCalendarSpan(bool auto_span);

  [[nodiscard]] const std::vector<float>& GetSpacingProportions() const;
  void SetSpacingProportions(const std::vector<float>& proportions);

 private:
  static constexpr float kSpacingSmall = 25.0F;
  static constexpr float kSpacingMedium = 50.0F;
  static constexpr float kSpacingLarge = 100.0F;
  static constexpr std::array<float, 7> kDefaultSpacingProportions = {
      kSpacingSmall,  kSpacingLarge, kSpacingMedium, kSpacingLarge,
      kSpacingMedium, kSpacingLarge, kSpacingSmall};

  bool auto_calendar_span_{true};
  std::vector<float> spacing_proportions_{std::vector<float>(
      kDefaultSpacingProportions.begin(), kDefaultSpacingProportions.end())};
};
#endif  // CALENDAR_CONFIG_HPP
