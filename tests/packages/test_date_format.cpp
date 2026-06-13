#include <gtest/gtest.h>

#include <string>

#include "packages/date.hpp"
#include "packages/date_format.hpp"

// All tests pin an explicit locale so they are independent of the machine the
// suite runs on.

TEST(LocaleDateFormatterTest, FormatsWithFourDigitYear) {
  LocaleDateFormatter formatter("de_CH");
  EXPECT_EQ(formatter.Format(Date::FromYmd(2030, 6, 10)), "10.06.2030");
}

TEST(LocaleDateFormatterTest, FormatsInvalidDateAsPlaceholder) {
  LocaleDateFormatter formatter("de_CH");
  EXPECT_EQ(formatter.Format(Date()), "invalid date");
}

TEST(LocaleDateFormatterTest, ParsesFullYear) {
  LocaleDateFormatter formatter("de_CH");
  EXPECT_EQ(formatter.Parse("23.09.1998"), Date::FromYmd(1998, 9, 23));
}

TEST(LocaleDateFormatterTest, ParsesTwoDigitYearViaPivot) {
  LocaleDateFormatter formatter("de_CH");
  // ICU pivots 2-digit years into a window around the current date; "98" must
  // land on 1998 for the foreseeable future.
  EXPECT_EQ(formatter.Parse("23.09.98"), Date::FromYmd(1998, 9, 23));
}

TEST(LocaleDateFormatterTest, ParseRejectsGarbage) {
  LocaleDateFormatter formatter("de_CH");
  EXPECT_FALSE(formatter.Parse("not a date").IsValid());
  EXPECT_FALSE(formatter.Parse("").IsValid());
}

TEST(LocaleDateFormatterTest, RoundTripsAcrossLocales) {
  for (const auto* locale_name : {"de_CH", "en_US", "ja_JP"}) {
    LocaleDateFormatter formatter(locale_name);
    const Date date = Date::FromYmd(2031, 12, 1);
    EXPECT_EQ(formatter.Parse(formatter.Format(date)), date)
        << "locale: " << locale_name;
  }
}

TEST(LocaleDateFormatterTest, RoundTripsDistantYears) {
  LocaleDateFormatter formatter("de_CH");
  for (const int year : {1492, 1930, 2150}) {
    const Date date = Date::FromYmd(year, 7, 4);
    EXPECT_EQ(formatter.Parse(formatter.Format(date)), date)
        << "year: " << year;
  }
}
