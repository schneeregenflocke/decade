#include "icu_date_backend.hpp"

#include <unicode/gregocal.h>
#include <unicode/timezone.h>
#include <unicode/ucal.h>
#include <unicode/umachine.h>
#include <unicode/utypes.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>

namespace domain::detail {

IcuCalendarBackend::IcuCalendarBackend() {
  UErrorCode status = U_ZERO_ERROR;
  calendar_ = std::make_unique<icu::GregorianCalendar>(*icu::TimeZone::getGMT(),
                                                       status);
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

IcuCalendarBackend& IcuCalendarBackend::Instance() {
  thread_local IcuCalendarBackend instance;
  return instance;
}

bool IcuCalendarBackend::IsValidDate(const Ymd& ymd) {
  UErrorCode status = U_ZERO_ERROR;
  SeedFields(ymd);
  calendar_->getTime(status);
  return U_SUCCESS(status) != 0;
}

std::optional<Ymd> IcuCalendarBackend::AddDays(const Ymd& ymd, int days) {
  return Shifted(ymd, UCAL_DATE, days);
}

std::optional<Ymd> IcuCalendarBackend::AddMonths(const Ymd& ymd, int months) {
  return Shifted(ymd, UCAL_MONTH, months);
}

int IcuCalendarBackend::DayOfYear(const Ymd& ymd) {
  UErrorCode status = U_ZERO_ERROR;
  SeedFields(ymd);
  const int day_of_year = calendar_->get(UCAL_DAY_OF_YEAR, status);
  return U_SUCCESS(status) != 0 ? day_of_year : 0;
}

int IcuCalendarBackend::DayOfWeek(const Ymd& ymd) {
  UErrorCode status = U_ZERO_ERROR;
  SeedFields(ymd);
  const int day_of_week = calendar_->get(UCAL_DAY_OF_WEEK, status);
  return U_SUCCESS(status) != 0 ? day_of_week - UCAL_SUNDAY : 0;
}

std::int64_t IcuCalendarBackend::DaysBetween(const Ymd& from, const Ymd& to) {
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

void IcuCalendarBackend::SeedFields(const Ymd& ymd) {
  calendar_->clear();
  // icu::Calendar months are 0-based.
  calendar_->set(ymd.year, ymd.month - 1, ymd.day);
}

std::optional<Ymd> IcuCalendarBackend::Shifted(const Ymd& ymd,
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

}  // namespace domain::detail
