#ifndef CALENDAR_CONFIG_HPP
#define CALENDAR_CONFIG_HPP

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
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

  CalendarSpan()
      : span_(Date::FromYmd(kDefaultStartYear, 1, 1),
              Date::FromYmd(kDefaultEndYear, 1, 1)) {}

  void SetSpan(YearSpan span_years) {
    const int clamped_lower_year =
        std::clamp(span_years.first_year, Date::kMinYear, Date::kMaxYear);
    const int clamped_upper_year =
        std::clamp(span_years.last_year + 1, Date::kMinYear, Date::kMaxYear);

    span_ = DatePeriod(Date::FromYmd(clamped_lower_year, 1, 1),
                       Date::FromYmd(clamped_upper_year, 1, 1));
  }

  [[nodiscard]] bool IsValidSpan() const { return !span_.IsNull(); }

  [[nodiscard]] std::size_t GetSpanLengthYears() const {
    if (!IsValidSpan()) {
      throw std::runtime_error("Not valid calendar span!");
    }
    return static_cast<std::size_t>(span_.End().Year() - span_.Begin().Year());
  }

  [[nodiscard]] std::array<int, 2> GetSpanLimitsYears() const {
    return std::array<int, 2>{span_.Begin().Year(), span_.Last().Year()};
  }

  [[nodiscard]] std::array<Date, 2> GetSpanLimitsDate() const {
    return std::array<Date, 2>{span_.Begin(), span_.Last()};
  }

  [[nodiscard]] std::int64_t GetSpanLengthDays() const {
    return span_.LengthDays();
  }

  [[nodiscard]] int GetYear(const std::size_t index) const {
    const int year = span_.Begin().Year() + static_cast<int>(index);

    if (!IsInSpan(year)) {
      throw std::logic_error("Year not in span!");
    }

    return year;
  }

  [[nodiscard]] bool IsInSpan(const int year) const {
    return year >= span_.Begin().Year() && year <= span_.Last().Year();
  }

 private:
  static constexpr int kDefaultStartYear = 2000;
  static constexpr int kDefaultEndYear = 2010;

  DatePeriod span_;
};

// Pure domain value: the full calendar configuration. Rule of Zero (no signal,
// no hand-written copy/move) -> freely and correctly copyable.
class CalendarConfig : public CalendarSpan {
 public:
  [[nodiscard]] bool IsAutoCalendarSpan() const { return auto_calendar_span_; }
  void SetAutoCalendarSpan(bool auto_span) { auto_calendar_span_ = auto_span; }

  [[nodiscard]] const std::vector<float>& GetSpacingProportions() const {
    return spacing_proportions_;
  }
  std::vector<float>& MutableSpacingProportions() {
    return spacing_proportions_;
  }
  void SetSpacingProportions(const std::vector<float>& proportions) {
    spacing_proportions_ = proportions;
  }

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
