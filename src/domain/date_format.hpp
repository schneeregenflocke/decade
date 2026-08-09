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
// Like detail/icu_date_backend.hpp this component is deliberately ICU-coupled;
// it is the second (and last) replacement point when switching date libraries.

#include <unicode/gregocal.h>
#include <unicode/smpdtfmt.h>
#include <unicode/unistr.h>

#include <memory>
#include <string>

#include "date.hpp"

class LocaleDateFormatter {
 public:
  // Uses the process default locale (as the previous "%x" approach did).
  LocaleDateFormatter();

  // Explicit locale, e.g. "de_CH" — primarily for deterministic tests. An
  // empty name selects the process default locale.
  explicit LocaleDateFormatter(const std::string& locale_name);

  [[nodiscard]] std::string Format(const Date& date);

  // Returns an invalid Date when the text is not parseable as a date in this
  // locale.
  [[nodiscard]] Date Parse(const std::string& text);

 private:
  // "dd.MM.yy" -> "dd.MM.yyyy"; patterns already carrying a 1- or 4-letter
  // year field ("y" formats the full year) pass through unchanged.
  [[nodiscard]] static icu::UnicodeString WidenYearPattern(
      const icu::UnicodeString& pattern);

  std::unique_ptr<icu::SimpleDateFormat> parse_formatter_;
  std::unique_ptr<icu::SimpleDateFormat> format_formatter_;
  std::unique_ptr<icu::GregorianCalendar> calendar_;
};

#endif  // DATE_FORMAT_HPP
