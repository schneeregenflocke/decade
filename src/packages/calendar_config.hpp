#ifndef CALENDAR_CONFIG_HPP
#define CALENDAR_CONFIG_HPP

#include <algorithm>
#include <array>
#include <boost/serialization/split_free.hpp>
// split_free.hpp must precede greg_serialize.hpp: the latter expands
// BOOST_DATE_TIME_SPLIT_FREE, which references boost::serialization::split_free
// without including its declaration itself.
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <cstdint>
#include <sigslot/signal.hpp>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../date_utils.hpp"
#include "detail/reentry_guard.hpp"
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

  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& archive, const unsigned int version) {
    (void)version;
    archive& BOOST_SERIALIZATION_NVP(span);
    archive& BOOST_SERIALIZATION_NVP(valid_id);
  }
};

class CalendarConfigStorage : public CalendarSpan {
 public:
  CalendarConfigStorage() = default;
  CalendarConfigStorage(const CalendarConfigStorage& other)
      : CalendarSpan(other),
        auto_calendar_span(other.auto_calendar_span),
        spacing_proportions(other.spacing_proportions) {}
  CalendarConfigStorage(CalendarConfigStorage&& other) noexcept
      : CalendarSpan(other),
        auto_calendar_span(other.auto_calendar_span),
        spacing_proportions(std::move(other.spacing_proportions)) {}
  CalendarConfigStorage& operator=(CalendarConfigStorage&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    auto_calendar_span = other.auto_calendar_span;
    spacing_proportions = std::move(other.spacing_proportions);
    CalendarSpan::operator=(other);
    return *this;
  }
  ~CalendarConfigStorage() = default;

  void ReceiveCalendarConfigStorage(
      const CalendarConfigStorage& calendar_config_storage) {
    if (emitting_) {
      return;
    }
    const packages::detail::ScopedReentryFlag guard(emitting_);
    *this = calendar_config_storage;
    signal_calendar_config_storage(*this);
  }

  void SendCalendarConfigStorage() { signal_calendar_config_storage(*this); }

  CalendarConfigStorage& operator=(const CalendarConfigStorage& other) {
    if (this == &other) {
      return *this;
    }
    auto_calendar_span = other.auto_calendar_span;
    spacing_proportions = other.spacing_proportions;

    CalendarSpan::operator=(other);

    return *this;
  }

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

  sigslot::signal<const CalendarConfigStorage&>& SignalCalendarConfigStorage() {
    return signal_calendar_config_storage;
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

  sigslot::signal<const CalendarConfigStorage&> signal_calendar_config_storage;
  bool emitting_{false};

  friend class boost::serialization::access;
  template <class Archive>
  void save(Archive& archive, const unsigned int version) const {
    (void)version;
    archive& BOOST_SERIALIZATION_NVP(auto_calendar_span);
    archive& BOOST_SERIALIZATION_NVP(spacing_proportions);
    // boost::serialization::base_object<CalendarSpan>(*this);
    archive& BOOST_SERIALIZATION_BASE_OBJECT_NVP(CalendarSpan);
  }
  template <class Archive>
  void load(Archive& archive, const unsigned int version) {
    (void)version;
    archive& BOOST_SERIALIZATION_NVP(auto_calendar_span);
    archive& BOOST_SERIALIZATION_NVP(spacing_proportions);
    // boost::serialization::base_object<CalendarSpan>(*this);
    archive& BOOST_SERIALIZATION_BASE_OBJECT_NVP(CalendarSpan);
    SendCalendarConfigStorage();
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()
};
#endif  // CALENDAR_CONFIG_HPP
