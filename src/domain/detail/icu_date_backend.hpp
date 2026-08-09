#ifndef ICU_DATE_BACKEND_HPP
#define ICU_DATE_BACKEND_HPP

// Calendar-arithmetic backend for the domain date types (`Date`/`DatePeriod`).
//
// This header is the ONLY place in the domain layer that talks to a concrete
// date library (ICU). `date.hpp` exposes a library-free interface and delegates
// every calendrical computation to the free functions below; swapping the date
// library means reimplementing this one component, nothing else.
//
// Semantics: proleptic Gregorian calendar (the Gregorian rules extended
// backwards before 1582), evaluated in a fixed GMT timezone so day arithmetic
// is exact and never crosses DST boundaries. The calendar is non-lenient, so
// out-of-range field combinations (Feb 30, month 13, ...) are rejected instead
// of being normalised.

#include <unicode/gregocal.h>
#include <unicode/ucal.h>

#include <cstdint>
#include <memory>
#include <optional>

namespace domain::detail {

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
  IcuCalendarBackend();

  static IcuCalendarBackend& Instance();

  [[nodiscard]] bool IsValidDate(const Ymd& ymd);

  // Both functions return std::nullopt when ICU reports a computation error;
  // callers (Date) map that to an invalid date.
  [[nodiscard]] std::optional<Ymd> AddDays(const Ymd& ymd, int days);

  [[nodiscard]] std::optional<Ymd> AddMonths(const Ymd& ymd, int months);

  [[nodiscard]] int DayOfYear(const Ymd& ymd);

  // 0 = Sunday ... 6 = Saturday (ICU's UCAL_SUNDAY == 1, shifted down).
  [[nodiscard]] int DayOfWeek(const Ymd& ymd);

  [[nodiscard]] std::int64_t DaysBetween(const Ymd& from, const Ymd& to);

 private:
  static constexpr double kMillisPerDay = 86'400'000.0;

  void SeedFields(const Ymd& ymd);

  [[nodiscard]] std::optional<Ymd> Shifted(const Ymd& ymd,
                                           UCalendarDateFields field,
                                           int amount);

  std::unique_ptr<icu::GregorianCalendar> calendar_;
};

}  // namespace domain::detail

#endif  // ICU_DATE_BACKEND_HPP
