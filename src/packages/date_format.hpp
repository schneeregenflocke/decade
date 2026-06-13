#ifndef DATE_FORMAT_HPP
#define DATE_FORMAT_HPP

// Locale-aware date text conversion for the GUI and CSV I/O, built on ICU's
// DateFormat. Replaces the previous hand-rolled locale detection that
// formatted a probe date with std::put_time("%x") and re-parsed it to guess
// the field order.
//
// Output always carries a 4-digit year (the locale's short pattern is widened
// from "yy" to "yyyy"), so round-trips are unambiguous for any year. Parsing
// uses the locale's original short pattern leniently: ICU accepts both 2-digit
// years (pivoted around the current date, e.g. "98" -> 1998) and full years
// ("1998" -> 1998).
//
// Like detail/icu_date_backend.hpp this header is deliberately ICU-coupled;
// it is the second (and last) replacement point when switching date libraries.

#include <unicode/datefmt.h>
#include <unicode/fieldpos.h>
#include <unicode/gregocal.h>
#include <unicode/locid.h>
#include <unicode/parsepos.h>
#include <unicode/smpdtfmt.h>
#include <unicode/stringpiece.h>
#include <unicode/timezone.h>
#include <unicode/ucal.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

#include "date.hpp"

class LocaleDateFormatter {
 public:
  // Uses the process default locale (as the previous "%x" approach did).
  LocaleDateFormatter() : LocaleDateFormatter(std::string()) {}

  // Explicit locale, e.g. "de_CH" — primarily for deterministic tests. An
  // empty name selects the process default locale.
  explicit LocaleDateFormatter(const std::string& locale_name) {
    const icu::Locale locale = locale_name.empty()
                                   ? icu::Locale::getDefault()
                                   : icu::Locale(locale_name.c_str());
    UErrorCode status = U_ZERO_ERROR;

    // The locale's short date pattern is the parse pattern; the format
    // pattern additionally gets the year widened to 4 digits.
    icu::UnicodeString pattern("yyyy-MM-dd");  // fallback
    const std::unique_ptr<icu::DateFormat> locale_format(
        icu::DateFormat::createDateInstance(icu::DateFormat::kShort, locale));
    if (auto* simple =
            dynamic_cast<icu::SimpleDateFormat*>(locale_format.get())) {
      simple->toPattern(pattern);
    }

    parse_formatter_ =
        std::make_unique<icu::SimpleDateFormat>(pattern, locale, status);
    format_formatter_ = std::make_unique<icu::SimpleDateFormat>(
        WidenYearPattern(pattern), locale, status);
    calendar_ = std::make_unique<icu::GregorianCalendar>(
        *icu::TimeZone::getGMT(), status);
    if (U_FAILURE(status) != 0) {
      throw std::runtime_error("ICU date formatter construction failed");
    }

    // Same conventions as detail/icu_date_backend.hpp: fixed GMT (exact day
    // arithmetic) and proleptic Gregorian for the whole supported year range.
    calendar_->setGregorianChange(std::numeric_limits<UDate>::lowest(), status);
    for (auto* formatter : {parse_formatter_.get(), format_formatter_.get()}) {
      formatter->setTimeZone(*icu::TimeZone::getGMT());
      formatter->setLenient(static_cast<UBool>(true));
      auto proleptic = std::make_unique<icu::GregorianCalendar>(
          *icu::TimeZone::getGMT(), status);
      proleptic->setGregorianChange(std::numeric_limits<UDate>::lowest(),
                                    status);
      formatter->adoptCalendar(proleptic.release());
    }
    if (U_FAILURE(status) != 0) {
      throw std::runtime_error("ICU date formatter setup failed");
    }
  }

  [[nodiscard]] std::string Format(const Date& date) {
    if (!date.IsValid()) {
      return "invalid date";
    }
    UErrorCode status = U_ZERO_ERROR;
    calendar_->clear();
    calendar_->set(date.Year(), date.Month() - 1, date.Day());
    const UDate millis = calendar_->getTime(status);
    if (U_FAILURE(status) != 0) {
      return "invalid date";
    }
    icu::UnicodeString target;
    icu::FieldPosition ignore(icu::FieldPosition::DONT_CARE);
    format_formatter_->format(millis, target, ignore);
    std::string result;
    target.toUTF8String(result);
    return result;
  }

  // Returns an invalid Date when the text is not parseable as a date in this
  // locale.
  [[nodiscard]] Date Parse(const std::string& text) {
    const icu::UnicodeString source =
        icu::UnicodeString::fromUTF8(icu::StringPiece(text));
    icu::ParsePosition position(0);
    const UDate millis = parse_formatter_->parse(source, position);
    if (position.getIndex() == 0) {
      return {};
    }
    UErrorCode status = U_ZERO_ERROR;
    calendar_->setTime(millis, status);
    const int year = calendar_->get(UCAL_YEAR, status);
    const int month = calendar_->get(UCAL_MONTH, status) + 1;
    const int day = calendar_->get(UCAL_DATE, status);
    if (U_FAILURE(status) != 0) {
      return {};
    }
    return Date::FromYmd(year, month, day);
  }

 private:
  // "dd.MM.yy" -> "dd.MM.yyyy"; patterns already carrying a 1- or 4-letter
  // year field ("y" formats the full year) pass through unchanged.
  [[nodiscard]] static icu::UnicodeString WidenYearPattern(
      const icu::UnicodeString& pattern) {
    const int32_t start = pattern.indexOf(u'y');
    if (start < 0) {
      return pattern;
    }
    int32_t end = start;
    while (end < pattern.length() && pattern.charAt(end) == u'y') {
      ++end;
    }
    if (end - start != 2) {
      return pattern;
    }
    icu::UnicodeString widened(pattern);
    widened.insert(start, icu::UnicodeString("yy"));
    return widened;
  }

  std::unique_ptr<icu::SimpleDateFormat> parse_formatter_;
  std::unique_ptr<icu::SimpleDateFormat> format_formatter_;
  std::unique_ptr<icu::GregorianCalendar> calendar_;
};

#endif  // DATE_FORMAT_HPP
