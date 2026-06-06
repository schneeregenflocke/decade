#ifndef CALENDAR_CONFIG_HPP
#define CALENDAR_CONFIG_HPP

#include <algorithm>
#include <array>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <cstdint>
#include <sigslot/signal.hpp>
#include <stdexcept>
#include <vector>

#include "../date_utils.hpp"
#include "detail/reentry_guard.hpp"

// Pure domain value: the year span of the calendar. boost::gregorian is the
// domain data representation; there is no serialization or signal here ->
// copyable. Persistence lives non-intrusively in the infrastructure layer.
class CalendarSpan {
 public:
  struct YearSpan {
    int first_year;
    int last_year;
  };

  CalendarSpan()
      : span(boost::gregorian::date(kDefaultStartYear, 1, 1),
             boost::gregorian::date(kDefaultEndYear, 1, 1)),
        valid_id(CheckAndAdjustDateInterval(&span)) {}

  void SetSpan(YearSpan span_years) {
    const boost::gregorian::date min_date =
        boost::gregorian::date(boost::gregorian::min_date_time);
    const boost::gregorian::date max_date =
        boost::gregorian::date(boost::gregorian::max_date_time);

    auto clamped_lower_year =
        std::clamp(span_years.first_year, static_cast<int>(min_date.year()),
                   static_cast<int>(max_date.year()));
    auto clamped_upper_year =
        std::clamp(span_years.last_year + 1, static_cast<int>(min_date.year()),
                   static_cast<int>(max_date.year()));

    span = boost::gregorian::date_period(
        boost::gregorian::date(static_cast<unsigned short>(clamped_lower_year),
                               1, 1),
        boost::gregorian::date(static_cast<unsigned short>(clamped_upper_year),
                               1, 1));

    valid_id = CheckAndAdjustDateInterval(&span);
  }

  [[nodiscard]] bool IsValidSpan() const {
    return CheckDateInterval(span.begin(), span.end()) == 1;
  }

  [[nodiscard]] std::size_t GetSpanLengthYears() const {
    if (!IsValidSpan()) {
      throw std::runtime_error("Not valid calendar span!");
    }
    return static_cast<std::size_t>(span.end().year() - span.begin().year());
  }

  [[nodiscard]] std::array<int, 2> GetSpanLimitsYears() const {
    return std::array<int, 2>{span.begin().year(), span.last().year()};
  }

  [[nodiscard]] std::array<boost::gregorian::date, 2> GetSpanLimitsDate()
      const {
    return std::array<boost::gregorian::date, 2>{span.begin(), span.last()};
  }

  [[nodiscard]] std::int64_t GetSpanLengthDays() const {
    return span.length().days();
  }

  [[nodiscard]] int GetYear(const std::size_t index) const {
    const int year = span.begin().year() + static_cast<int>(index);

    if (!IsInSpan(year)) {
      throw std::logic_error("Year not in span!");
    }

    return year;
  }

  [[nodiscard]] bool IsInSpan(const int year) const {
    // NOLINTNEXTLINE(modernize-use-integer-sign-comparison) — greg_year is not
    // a built-in integer type.
    return year >= span.begin().year() && year <= span.last().year();
  }

 private:
  static constexpr int kDefaultStartYear = 2000;
  static constexpr int kDefaultEndYear = 2010;

  boost::gregorian::date_period span;
  int valid_id{0};
};

// Pure domain value: the full calendar configuration. Rule of Zero (no signal,
// no hand-written copy/move) -> freely and correctly copyable.
class CalendarConfig : public CalendarSpan {
 public:
  [[nodiscard]] bool IsAutoCalendarSpan() const { return auto_calendar_span; }
  void SetAutoCalendarSpan(bool auto_span) { auto_calendar_span = auto_span; }

  [[nodiscard]] const std::vector<float>& GetSpacingProportions() const {
    return spacing_proportions;
  }
  std::vector<float>& MutableSpacingProportions() {
    return spacing_proportions;
  }
  void SetSpacingProportions(const std::vector<float>& proportions) {
    spacing_proportions = proportions;
  }

 private:
  static constexpr float kSpacingSmall = 25.0F;
  static constexpr float kSpacingMedium = 50.0F;
  static constexpr float kSpacingLarge = 100.0F;
  static constexpr std::array<float, 7> kDefaultSpacingProportions = {
      kSpacingSmall,  kSpacingLarge, kSpacingMedium, kSpacingLarge,
      kSpacingMedium, kSpacingLarge, kSpacingSmall};

  bool auto_calendar_span{true};
  std::vector<float> spacing_proportions{std::vector<float>(
      kDefaultSpacingProportions.begin(), kDefaultSpacingProportions.end())};
};

// Owns a CalendarConfig value plus the change signal and re-entry guard. Has
// identity -> non-copyable. Delegates the value API so callers stay stable;
// carries no serialization code.
class CalendarConfigStorage {
 public:
  CalendarConfigStorage() = default;
  ~CalendarConfigStorage() = default;
  CalendarConfigStorage(const CalendarConfigStorage&) = delete;
  CalendarConfigStorage(CalendarConfigStorage&&) = delete;
  CalendarConfigStorage& operator=(const CalendarConfigStorage&) = delete;
  CalendarConfigStorage& operator=(CalendarConfigStorage&&) = delete;

  void ReceiveCalendarConfigStorage(
      const CalendarConfigStorage& calendar_config_storage) {
    if (emitting_) {
      return;
    }
    const packages::detail::ScopedReentryFlag guard(emitting_);
    CopyFrom(calendar_config_storage);
    signal_calendar_config_storage(*this);
  }

  void SendCalendarConfigStorage() { signal_calendar_config_storage(*this); }

  void CopyFrom(const CalendarConfigStorage& other) {
    calendar_config = other.calendar_config;
  }

  // --- CalendarSpan delegation ---
  void SetSpan(CalendarSpan::YearSpan span_years) {
    calendar_config.SetSpan(span_years);
  }
  [[nodiscard]] bool IsValidSpan() const {
    return calendar_config.IsValidSpan();
  }
  [[nodiscard]] std::size_t GetSpanLengthYears() const {
    return calendar_config.GetSpanLengthYears();
  }
  [[nodiscard]] std::array<int, 2> GetSpanLimitsYears() const {
    return calendar_config.GetSpanLimitsYears();
  }
  [[nodiscard]] std::array<boost::gregorian::date, 2> GetSpanLimitsDate()
      const {
    return calendar_config.GetSpanLimitsDate();
  }
  [[nodiscard]] std::int64_t GetSpanLengthDays() const {
    return calendar_config.GetSpanLengthDays();
  }
  [[nodiscard]] int GetYear(const std::size_t index) const {
    return calendar_config.GetYear(index);
  }
  [[nodiscard]] bool IsInSpan(const int year) const {
    return calendar_config.IsInSpan(year);
  }

  // --- CalendarConfig delegation ---
  [[nodiscard]] bool IsAutoCalendarSpan() const {
    return calendar_config.IsAutoCalendarSpan();
  }
  void SetAutoCalendarSpan(bool auto_span) {
    calendar_config.SetAutoCalendarSpan(auto_span);
  }
  [[nodiscard]] const std::vector<float>& GetSpacingProportions() const {
    return calendar_config.GetSpacingProportions();
  }
  std::vector<float>& MutableSpacingProportions() {
    return calendar_config.MutableSpacingProportions();
  }
  void SetSpacingProportions(const std::vector<float>& proportions) {
    calendar_config.SetSpacingProportions(proportions);
  }

  [[nodiscard]] const CalendarConfig& Value() const { return calendar_config; }

  void SetValue(const CalendarConfig& value) {
    calendar_config = value;
    signal_calendar_config_storage(*this);
  }

  sigslot::signal<const CalendarConfigStorage&>& SignalCalendarConfigStorage() {
    return signal_calendar_config_storage;
  }

 private:
  CalendarConfig calendar_config;
  sigslot::signal<const CalendarConfigStorage&> signal_calendar_config_storage;
  bool emitting_{false};
};
#endif  // CALENDAR_CONFIG_HPP
