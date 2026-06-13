#ifndef ICU_DATE_BACKEND_HPP
#define ICU_DATE_BACKEND_HPP

// Calendar-arithmetic backend for the domain date types (`Date`/`DatePeriod`).
//
// This header is the ONLY place in the domain layer that talks to a concrete
// date library (ICU). `date.hpp` exposes a library-free interface and delegates
// every calendrical computation to the free functions below; swapping the date
// library means reimplementing this one header, nothing else.
//
// Semantics: proleptic Gregorian calendar (the Gregorian rules extended
// backwards before 1582), evaluated in a fixed GMT timezone so day arithmetic
// is exact and never crosses DST boundaries. The calendar is non-lenient, so
// out-of-range field combinations (Feb 30, month 13, ...) are rejected instead
// of being normalised.

#include <unicode/calendar.h>
#include <unicode/gregocal.h>
#include <unicode/timezone.h>
#include <unicode/ucal.h>
#include <unicode/utypes.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>

namespace packages::detail {

// Plain year/month/day triple exchanged with the backend. `month` and `day`
// are 1-based (January = 1), matching the domain convention.
struct Ymd {
  int year;
  int month;
  int day;
};

// Owns one reusable ICU calendar instance. Every operation re-seeds the
// calendar fields, so calls are independent; the instance is thread_local
// (see Instance()) because icu::Calendar is not thread-safe.
class IcuCalendarBackend {
 public:
  IcuCalendarBackend() {
    UErrorCode status = U_ZERO_ERROR;
    calendar_ = std::make_unique<icu::GregorianCalendar>(
        *icu::TimeZone::getGMT(), status);
    if (U_FAILURE(status) != 0) {
      throw std::runtime_error("ICU GregorianCalendar construction failed");
    }
    // Push the Julian->Gregorian cutover to the far past: pure proleptic
    // Gregorian, consistent for the whole supported year range.
    calendar_->setGregorianChange(std::numeric_limits<UDate>::lowest(), status);
    if (U_FAILURE(status) != 0) {
      throw std::runtime_error("ICU setGregorianChange failed");
    }
    calendar_->setLenient(static_cast<UBool>(false));
  }

  static IcuCalendarBackend& Instance() {
    thread_local IcuCalendarBackend instance;
    return instance;
  }

  [[nodiscard]] bool IsValidDate(const Ymd& ymd) {
    UErrorCode status = U_ZERO_ERROR;
    SeedFields(ymd);
    calendar_->getTime(status);
    return U_SUCCESS(status) != 0;
  }

  // Both functions return std::nullopt when ICU reports a computation error;
  // callers (Date) map that to an invalid date.
  [[nodiscard]] std::optional<Ymd> AddDays(const Ymd& ymd, int days) {
    return Shifted(ymd, UCAL_DATE, days);
  }

  [[nodiscard]] std::optional<Ymd> AddMonths(const Ymd& ymd, int months) {
    return Shifted(ymd, UCAL_MONTH, months);
  }

  [[nodiscard]] int DayOfYear(const Ymd& ymd) {
    UErrorCode status = U_ZERO_ERROR;
    SeedFields(ymd);
    const int day_of_year = calendar_->get(UCAL_DAY_OF_YEAR, status);
    return U_SUCCESS(status) != 0 ? day_of_year : 0;
  }

  // 0 = Sunday ... 6 = Saturday (ICU's UCAL_SUNDAY == 1, shifted down).
  [[nodiscard]] int DayOfWeek(const Ymd& ymd) {
    UErrorCode status = U_ZERO_ERROR;
    SeedFields(ymd);
    const int day_of_week = calendar_->get(UCAL_DAY_OF_WEEK, status);
    return U_SUCCESS(status) != 0 ? day_of_week - UCAL_SUNDAY : 0;
  }

  [[nodiscard]] std::int64_t DaysBetween(const Ymd& from, const Ymd& to) {
    UErrorCode status = U_ZERO_ERROR;
    SeedFields(from);
    const UDate from_millis = calendar_->getTime(status);
    SeedFields(to);
    const UDate to_millis = calendar_->getTime(status);
    if (U_FAILURE(status) != 0) {
      return 0;
    }
    // GMT has no DST transitions, so the difference is an exact multiple of
    // a day; llround only absorbs floating-point representation error.
    return std::llround((to_millis - from_millis) / kMillisPerDay);
  }

 private:
  static constexpr double kMillisPerDay = 86'400'000.0;

  void SeedFields(const Ymd& ymd) {
    calendar_->clear();
    // icu::Calendar months are 0-based.
    calendar_->set(ymd.year, ymd.month - 1, ymd.day);
  }

  [[nodiscard]] std::optional<Ymd> Shifted(const Ymd& ymd,
                                           UCalendarDateFields field,
                                           int amount) {
    UErrorCode status = U_ZERO_ERROR;
    SeedFields(ymd);
    calendar_->add(field, amount, status);
    const int year = calendar_->get(UCAL_YEAR, status);
    const int month = calendar_->get(UCAL_MONTH, status) + 1;
    const int day = calendar_->get(UCAL_DATE, status);
    if (U_FAILURE(status) != 0) {
      return std::nullopt;
    }
    return Ymd{.year = year, .month = month, .day = day};
  }

  std::unique_ptr<icu::GregorianCalendar> calendar_;
};

}  // namespace packages::detail

#endif  // ICU_DATE_BACKEND_HPP
