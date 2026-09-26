#include "calendar_config.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

#include "calendar_view.hpp"
#include "date.hpp"
#include "date_period.hpp"

CalendarSpan::CalendarSpan()
    : span_(Date::FromYmd(kDefaultStartYear, 1, 1),
            Date::FromYmd(kDefaultEndYear, 1, 1)) {}

void CalendarSpan::SetYears(YearSpan years) {
  // It never produces a null span: the half-open end Jan 1 (last + 1) needs a
  // representable year — the last selectable calendar year is therefore
  // kMaxYear - 1, and a last year before the first year gets raised to the
  // first year (a span of exactly one year).
  const int first_year =
      std::clamp(years.first_year, Date::kMinYear, Date::kMaxYear - 1);
  const int last_year =
      std::clamp(years.last_year, first_year, Date::kMaxYear - 1);

  span_ = DatePeriod(Date::FromYmd(first_year, 1, 1),
                     Date::FromYmd(last_year + 1, 1, 1));
}

std::size_t CalendarSpan::YearCount() const {
  return static_cast<std::size_t>(span_.End().Year() - span_.Begin().Year());
}

int CalendarSpan::FirstYear() const { return span_.Begin().Year(); }

int CalendarSpan::LastYear() const { return span_.Last().Year(); }

const DatePeriod& CalendarSpan::Period() const { return span_; }

bool CalendarSpan::ShowsYear(const int year) const {
  return year >= span_.Begin().Year() && year <= span_.Last().Year();
}

CalendarView CalendarConfig::View() const { return view_; }

void CalendarConfig::SetView(CalendarView view) { view_ = view; }

bool CalendarConfig::IsFitYearsToEntries() const {
  return fit_years_to_entries_;
}

void CalendarConfig::SetFitYearsToEntries(bool fit) {
  fit_years_to_entries_ = fit;
}

bool CalendarConfig::ShowsAnnualCoverage() const {
  return shows_annual_coverage_;
}

void CalendarConfig::SetShowsAnnualCoverage(bool shows) {
  shows_annual_coverage_ = shows;
}

const std::vector<float>& CalendarConfig::GetSpacingProportions() const {
  return spacing_proportions_;
}

void CalendarConfig::SetSpacingProportions(
    const std::vector<float>& proportions) {
  spacing_proportions_ = proportions;
}

std::vector<float> CalendarConfig::LaidOutSpacingProportions() const {
  std::vector<float> proportions = spacing_proportions_;
  if (!shows_annual_coverage_ &&
      kAnnualCoverageSpacingIndex < proportions.size()) {
    proportions[kAnnualCoverageSpacingIndex] = 0.0F;
  }
  return proportions;
}
