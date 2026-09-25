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

  void SetYears(YearSpan years);

  [[nodiscard]] std::size_t YearCount() const;

  [[nodiscard]] int FirstYear() const;

  [[nodiscard]] int LastYear() const;

  [[nodiscard]] Date FirstDay() const;

  [[nodiscard]] std::int64_t DayCount() const;

  [[nodiscard]] int YearAt(std::size_t index) const;

  [[nodiscard]] bool ShowsYear(int year) const;

 private:
  static constexpr int kDefaultStartYear = 2000;
  static constexpr int kDefaultEndYear = 2010;

  DatePeriod span_;
};

// Pure domain value: the full calendar configuration. Rule of Zero (no signal,
// no hand-written copy/move) -> freely and correctly copyable.
class CalendarConfig : public CalendarSpan {
 public:
  [[nodiscard]] bool IsFitYearsToEntries() const;
  void SetFitYearsToEntries(bool fit);

  [[nodiscard]] const std::vector<float>& GetSpacingProportions() const;
  void SetSpacingProportions(const std::vector<float>& proportions);

 private:
  static constexpr float kSpacingSmall = 25.0F;
  static constexpr float kSpacingMedium = 50.0F;
  static constexpr float kSpacingLarge = 100.0F;
  static constexpr std::array<float, 7> kDefaultSpacingProportions = {
      kSpacingSmall,  kSpacingLarge, kSpacingMedium, kSpacingLarge,
      kSpacingMedium, kSpacingLarge, kSpacingSmall};

  bool fit_years_to_entries_{true};
  std::vector<float> spacing_proportions_{std::vector<float>(
      kDefaultSpacingProportions.begin(), kDefaultSpacingProportions.end())};
};
#endif  // CALENDAR_CONFIG_HPP
