#include <gtest/gtest.h>

#include "packages/date.hpp"

TEST(DateTest, DefaultConstructedIsInvalid) {
  const Date date;
  EXPECT_FALSE(date.IsValid());
  EXPECT_EQ(date.Year(), 0);
  EXPECT_EQ(date.Month(), 0);
  EXPECT_EQ(date.Day(), 0);
}

TEST(DateTest, FromYmdConstructsValidDate) {
  const Date date = Date::FromYmd(2030, 6, 10);
  ASSERT_TRUE(date.IsValid());
  EXPECT_EQ(date.Year(), 2030);
  EXPECT_EQ(date.Month(), 6);
  EXPECT_EQ(date.Day(), 10);
}

TEST(DateTest, FromYmdRejectsNonCalendarDates) {
  EXPECT_FALSE(Date::FromYmd(2030, 2, 30).IsValid());
  EXPECT_FALSE(Date::FromYmd(2030, 13, 1).IsValid());
  EXPECT_FALSE(Date::FromYmd(2030, 0, 1).IsValid());
  EXPECT_FALSE(Date::FromYmd(2030, 1, 0).IsValid());
  EXPECT_FALSE(Date::FromYmd(2030, 1, 32).IsValid());
}

TEST(DateTest, FromYmdRejectsYearsOutsideSupportedRange) {
  EXPECT_FALSE(Date::FromYmd(Date::kMinYear - 1, 6, 1).IsValid());
  EXPECT_FALSE(Date::FromYmd(Date::kMaxYear + 1, 6, 1).IsValid());
  EXPECT_TRUE(Date::FromYmd(Date::kMinYear, 1, 1).IsValid());
  EXPECT_TRUE(Date::FromYmd(Date::kMaxYear, 12, 31).IsValid());
}

TEST(DateTest, LeapYearRules) {
  EXPECT_TRUE(Date::FromYmd(2024, 2, 29).IsValid());   // divisible by 4
  EXPECT_FALSE(Date::FromYmd(2023, 2, 29).IsValid());  // common year
  EXPECT_TRUE(Date::FromYmd(2000, 2, 29).IsValid());   // divisible by 400
  EXPECT_FALSE(Date::FromYmd(1900, 2, 29).IsValid());  // century, not by 400
}

TEST(DateTest, DayOfYear) {
  EXPECT_EQ(Date::FromYmd(2023, 1, 1).DayOfYear(), 1);
  EXPECT_EQ(Date::FromYmd(2023, 12, 31).DayOfYear(), 365);
  EXPECT_EQ(Date::FromYmd(2024, 12, 31).DayOfYear(), 366);
  EXPECT_EQ(Date::FromYmd(2023, 3, 1).DayOfYear(), 60);
  EXPECT_EQ(Date::FromYmd(2024, 3, 1).DayOfYear(), 61);
  EXPECT_EQ(Date().DayOfYear(), 0);
}

TEST(DateTest, DayOfWeek) {
  EXPECT_EQ(Date::FromYmd(2026, 6, 12).DayOfWeek(), Weekday::kFriday);
  EXPECT_EQ(Date::FromYmd(2000, 1, 1).DayOfWeek(), Weekday::kSaturday);
  EXPECT_EQ(Date::FromYmd(1998, 9, 23).DayOfWeek(), Weekday::kWednesday);
  EXPECT_EQ(Date::FromYmd(2026, 6, 14).DayOfWeek(), Weekday::kSunday);
}

TEST(DateTest, AddDaysWithinMonth) {
  const Date date = Date::FromYmd(2030, 6, 10).AddDays(5);
  EXPECT_EQ(date, Date::FromYmd(2030, 6, 15));
}

TEST(DateTest, AddDaysAcrossYearAndLeapBoundary) {
  EXPECT_EQ(Date::FromYmd(2023, 12, 31).AddDays(1), Date::FromYmd(2024, 1, 1));
  EXPECT_EQ(Date::FromYmd(2024, 2, 28).AddDays(1), Date::FromYmd(2024, 2, 29));
  EXPECT_EQ(Date::FromYmd(2023, 2, 28).AddDays(1), Date::FromYmd(2023, 3, 1));
  EXPECT_EQ(Date::FromYmd(2024, 3, 1).AddDays(-1), Date::FromYmd(2024, 2, 29));
}

TEST(DateTest, AddDaysOnInvalidStaysInvalid) {
  EXPECT_FALSE(Date().AddDays(1).IsValid());
}

TEST(DateTest, AddDaysBeyondSupportedRangeIsInvalid) {
  EXPECT_FALSE(Date::FromYmd(Date::kMaxYear, 12, 31).AddDays(1).IsValid());
}

TEST(DateTest, AddMonthsPinsDayOfMonth) {
  EXPECT_EQ(Date::FromYmd(2024, 1, 31).AddMonths(1),
            Date::FromYmd(2024, 2, 29));
  EXPECT_EQ(Date::FromYmd(2023, 1, 31).AddMonths(1),
            Date::FromYmd(2023, 2, 28));
}

TEST(DateTest, AddMonthsAcrossYearBoundary) {
  EXPECT_EQ(Date::FromYmd(2030, 12, 15).AddMonths(1),
            Date::FromYmd(2031, 1, 15));
  EXPECT_EQ(Date::FromYmd(2030, 1, 1).AddMonths(12), Date::FromYmd(2031, 1, 1));
}

TEST(DateTest, DaysBetween) {
  EXPECT_EQ(
      Date::DaysBetween(Date::FromYmd(2030, 6, 10), Date::FromYmd(2030, 6, 10)),
      0);
  EXPECT_EQ(
      Date::DaysBetween(Date::FromYmd(2030, 6, 10), Date::FromYmd(2030, 6, 20)),
      10);
  EXPECT_EQ(
      Date::DaysBetween(Date::FromYmd(2030, 6, 20), Date::FromYmd(2030, 6, 10)),
      -10);
  // Across the leap day of 2024.
  EXPECT_EQ(
      Date::DaysBetween(Date::FromYmd(2024, 1, 1), Date::FromYmd(2025, 1, 1)),
      366);
  EXPECT_EQ(
      Date::DaysBetween(Date::FromYmd(2023, 1, 1), Date::FromYmd(2024, 1, 1)),
      365);
  EXPECT_EQ(Date::DaysBetween(Date(), Date::FromYmd(2030, 1, 1)), 0);
}

TEST(DateTest, ComparesLexicographically) {
  EXPECT_LT(Date::FromYmd(2029, 12, 31), Date::FromYmd(2030, 1, 1));
  EXPECT_LT(Date::FromYmd(2030, 1, 31), Date::FromYmd(2030, 2, 1));
  EXPECT_EQ(Date::FromYmd(2030, 6, 10), Date::FromYmd(2030, 6, 10));
  EXPECT_LT(Date(), Date::FromYmd(Date::kMinYear, 1, 1));
}

TEST(DateTest, DaysInYear) {
  EXPECT_EQ(DaysInYear(2023), 365);
  EXPECT_EQ(DaysInYear(2024), 366);
  EXPECT_EQ(DaysInYear(1900), 365);
  EXPECT_EQ(DaysInYear(2000), 366);
}
