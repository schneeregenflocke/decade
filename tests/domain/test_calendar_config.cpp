#include <gtest/gtest.h>

#include <QtCore/QObject>
#include <stdexcept>

#include "domain/calendar_config.hpp"
#include "domain/calendar_config_store.hpp"
#include "domain/state_topics.hpp"

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

// Regression: SetSpan must never produce a null span — GetSpanLengthYears()
// would otherwise throw and the error escapes the Qt event handler uncaught
// (First Year > Last Year in the Timeframe tab, or both years = kMaxYear).
TEST(CalendarSpanTest, SetSpanNormalizesReversedYears) {
  CalendarSpan span;
  span.SetSpan({.first_year = 2030, .last_year = 2020});
  ASSERT_TRUE(span.IsValidSpan());
  EXPECT_EQ(span.GetSpanLengthYears(), 1U);
  EXPECT_EQ(span.GetSpanLimitsYears()[0], 2030);
}

TEST(CalendarSpanTest, SetSpanStaysValidAtMaxYear) {
  CalendarSpan span;
  span.SetSpan({.first_year = Date::kMaxYear, .last_year = Date::kMaxYear});
  ASSERT_TRUE(span.IsValidSpan());
  EXPECT_GE(span.GetSpanLengthYears(), 1U);
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

TEST(CalendarConfigStoreTest, ReceiveCopiesAndEmits) {
  CalendarConfig source;
  source.SetSpan({.first_year = 2040, .last_year = 2042});
  source.SetAutoCalendarSpan(false);

  domain::CalendarConfigTopic topic;
  CalendarConfigStore target(topic);
  int emissions = 0;
  QObject::connect(&topic, &domain::CalendarConfigTopic::Published,
                   [&](const CalendarConfig&) { ++emissions; });

  target.ReceiveCalendarConfig(source);

  EXPECT_EQ(emissions, 1);
  EXPECT_FALSE(target.Get().IsAutoCalendarSpan());
  EXPECT_EQ(target.Get().GetSpanLimitsYears()[0], 2040);
}

TEST(CalendarConfigStoreTest, ReentryGuardBlocksRecursiveReceive) {
  CalendarConfig secondary;
  secondary.SetSpan({.first_year = 2050, .last_year = 2050});

  domain::CalendarConfigTopic topic;
  CalendarConfigStore primary(topic);
  int emissions = 0;
  QObject::connect(&topic, &domain::CalendarConfigTopic::Published,
                   [&](const CalendarConfig&) {
                     ++emissions;
                     if (emissions == 1) {
                       primary.ReceiveCalendarConfig(secondary);
                     }
                   });

  primary.ReceiveCalendarConfig(secondary);

  EXPECT_EQ(emissions, 1);
}
