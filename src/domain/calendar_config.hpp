#ifndef CALENDAR_CONFIG_HPP
#define CALENDAR_CONFIG_HPP

#include <array>
#include <cstddef>

#include "band_part.hpp"
#include "calendar_sizing.hpp"
#include "calendar_view.hpp"
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

  [[nodiscard]] const DatePeriod& Period() const;

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
  [[nodiscard]] CalendarView View() const;
  void SetView(CalendarView view);

  [[nodiscard]] bool IsFitYearsToEntries() const;
  void SetFitYearsToEntries(bool fit);

  [[nodiscard]] bool ShowsAnnualCoverage() const;
  void SetShowsAnnualCoverage(bool shows);

  [[nodiscard]] const BandProportions& GetBandProportions() const;
  void SetBandProportions(const BandProportions& proportions);

  // What the layout divides a band by: a hidden annual coverage gives its part
  // back, while the stored proportions keep the value for later.
  [[nodiscard]] BandProportions LaidOutBandProportions() const;

  // The share of a band's height the part takes as laid out.
  [[nodiscard]] float LaidOutShare(BandPart part) const;

  [[nodiscard]] const CalendarSizing& Sizing() const;
  void SetSizing(const CalendarSizing& sizing);

 private:
  static constexpr float kProportionSmall = 25.0F;
  static constexpr float kProportionMedium = 50.0F;
  static constexpr float kProportionLarge = 100.0F;

  CalendarView view_{CalendarView::kYearPerRow};
  bool fit_years_to_entries_{true};
  bool shows_annual_coverage_{true};
  BandProportions band_proportions_{
      kProportionSmall,  kProportionLarge, kProportionMedium, kProportionLarge,
      kProportionMedium, kProportionLarge, kProportionSmall};
  CalendarSizing sizing_;
};
#endif  // CALENDAR_CONFIG_HPP
