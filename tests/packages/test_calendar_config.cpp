#include <gtest/gtest.h>

#include <stdexcept>

#include "packages/calendar_config.hpp"

TEST(CalendarSpanTest, DefaultSpanIsValid) {
  CalendarSpan span;
  EXPECT_TRUE(span.IsValidSpan());
  EXPECT_GT(span.GetSpanLengthYears(), 0);
}

TEST(CalendarSpanTest, SetSpanClampsAndStores) {
  CalendarSpan span;
  span.SetSpan({.first_year = 2020, .last_year = 2025});
  ASSERT_TRUE(span.IsValidSpan());
  const auto limits = span.GetSpanLimitsYears();
  EXPECT_EQ(limits[0], 2020);
  EXPECT_EQ(limits[1], 2025);
}

TEST(CalendarSpanTest, IsInSpanRespectsBounds) {
  CalendarSpan span;
  span.SetSpan({.first_year = 2020, .last_year = 2025});
  EXPECT_TRUE(span.IsInSpan(2020));
  EXPECT_TRUE(span.IsInSpan(2025));
  EXPECT_FALSE(span.IsInSpan(2019));
  EXPECT_FALSE(span.IsInSpan(2026));
}

TEST(CalendarSpanTest, GetYearReturnsRelativeYear) {
  CalendarSpan span;
  span.SetSpan({.first_year = 2030, .last_year = 2032});
  EXPECT_EQ(span.GetYear(0), 2030);
  EXPECT_EQ(span.GetYear(2), 2032);
}

TEST(CalendarSpanTest, GetYearThrowsWhenOutOfRange) {
  CalendarSpan span;
  span.SetSpan({.first_year = 2030, .last_year = 2030});
  EXPECT_THROW((void)span.GetYear(5), std::logic_error);
}

TEST(CalendarConfigStorageTest, ReceiveCopiesAndEmits) {
  CalendarConfigStorage source;
  source.SetSpan({.first_year = 2040, .last_year = 2042});
  source.SetAutoCalendarSpan(false);

  CalendarConfigStorage target;
  int emissions = 0;
  target.SignalCalendarConfigStorage().connect(
      [&](const CalendarConfigStorage&) { ++emissions; });

  target.ReceiveCalendarConfigStorage(source);

  EXPECT_EQ(emissions, 1);
  EXPECT_FALSE(target.IsAutoCalendarSpan());
  EXPECT_EQ(target.GetSpanLimitsYears()[0], 2040);
}

TEST(CalendarConfigStorageTest, ReentryGuardBlocksRecursiveReceive) {
  CalendarConfigStorage primary;
  CalendarConfigStorage secondary;
  secondary.SetSpan({.first_year = 2050, .last_year = 2050});

  int emissions = 0;
  primary.SignalCalendarConfigStorage().connect(
      [&](const CalendarConfigStorage&) {
        ++emissions;
        if (emissions == 1) {
          primary.ReceiveCalendarConfigStorage(secondary);
        }
      });

  primary.ReceiveCalendarConfigStorage(secondary);

  EXPECT_EQ(emissions, 1);
}
