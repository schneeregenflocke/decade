#include <gtest/gtest.h>

#include <QtCore/QObject>
#include <vector>

#include "domain/calendar_config.hpp"
#include "domain/calendar_config_store.hpp"
#include "domain/date.hpp"
#include "domain/date_entry.hpp"
#include "domain/date_period.hpp"
#include "domain/state_topics.hpp"

TEST(CalendarSpanTest, DefaultSpanIsValid) {
  CalendarSpan span;
  EXPECT_GT(span.YearCount(), 0);
}

TEST(CalendarSpanTest, SetYearsClampsAndStores) {
  CalendarSpan span;
  span.SetYears({.first_year = 2020, .last_year = 2025});
  EXPECT_EQ(span.FirstYear(), 2020);
  EXPECT_EQ(span.LastYear(), 2025);
}

// Regression: SetYears must never produce a null span — the calendar would lose
// every year row (First Year > Last Year in the Timeframe tab, or both years =
// kMaxYear).
TEST(CalendarSpanTest, SetYearsNormalizesReversedYears) {
  CalendarSpan span;
  span.SetYears({.first_year = 2030, .last_year = 2020});
  EXPECT_EQ(span.YearCount(), 1U);
  EXPECT_EQ(span.FirstYear(), 2030);
}

TEST(CalendarSpanTest, SetYearsStaysValidAtMaxYear) {
  CalendarSpan span;
  span.SetYears({.first_year = Date::kMaxYear, .last_year = Date::kMaxYear});
  EXPECT_GE(span.YearCount(), 1U);
}

TEST(CalendarSpanTest, ShowsYearRespectsBounds) {
  CalendarSpan span;
  span.SetYears({.first_year = 2020, .last_year = 2025});
  EXPECT_TRUE(span.ShowsYear(2020));
  EXPECT_TRUE(span.ShowsYear(2025));
  EXPECT_FALSE(span.ShowsYear(2019));
  EXPECT_FALSE(span.ShowsYear(2026));
}

TEST(CalendarSpanTest, PeriodRunsFromFirstJanuaryToTheNextAfterLastYear) {
  CalendarSpan span;
  span.SetYears({.first_year = 2030, .last_year = 2032});
  EXPECT_EQ(span.Period().Begin(), Date::FromYmd(2030, 1, 1));
  EXPECT_EQ(span.Period().End(), Date::FromYmd(2033, 1, 1));
}

TEST(CalendarConfigTest, HiddenAnnualCoverageLaysOutNoSubrow) {
  CalendarConfig config;
  const std::vector<float> stored = config.GetSpacingProportions();
  ASSERT_GT(stored[CalendarConfig::kAnnualCoverageSpacingIndex], 0.0F);
  EXPECT_EQ(config.LaidOutSpacingProportions(), stored);

  config.SetShowsAnnualCoverage(false);

  std::vector<float> expected = stored;
  expected[CalendarConfig::kAnnualCoverageSpacingIndex] = 0.0F;
  EXPECT_EQ(config.LaidOutSpacingProportions(), expected);
  EXPECT_EQ(config.GetSpacingProportions(), stored);
}

TEST(CalendarConfigStoreTest, ReceiveCopiesAndEmits) {
  CalendarConfig source;
  source.SetYears({.first_year = 2040, .last_year = 2042});
  source.SetFitYearsToEntries(false);

  domain::CalendarConfigTopic topic;
  CalendarConfigStore target(topic);
  int emissions = 0;
  QObject::connect(&topic, &domain::CalendarConfigTopic::Published,
                   [&](const CalendarConfig&) { ++emissions; });

  target.ReceiveCalendarConfig(source);

  EXPECT_EQ(emissions, 1);
  EXPECT_FALSE(target.Get().IsFitYearsToEntries());
  EXPECT_EQ(target.Get().FirstYear(), 2040);
}

TEST(CalendarConfigStoreTest, ReentryGuardBlocksRecursiveReceive) {
  CalendarConfig secondary;
  secondary.SetYears({.first_year = 2050, .last_year = 2050});

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

namespace {

DateEntry EntryBetween(int first_year, int last_year) {
  DateEntry entry;
  entry.SetDateInterval(DatePeriod(Date::FromYmd(first_year, 3, 1),
                                   Date::FromYmd(last_year, 9, 1)));
  return entry;
}

}  // namespace

TEST(CalendarConfigStoreTest, FittedYearsFollowTheEntries) {
  domain::CalendarConfigTopic topic;
  CalendarConfigStore store(topic);
  int emissions = 0;
  QObject::connect(&topic, &domain::CalendarConfigTopic::Published,
                   [&](const CalendarConfig&) { ++emissions; });

  store.ReceiveDateEntries(
      {EntryBetween(2004, 2005), EntryBetween(1998, 2023)});

  EXPECT_EQ(emissions, 1);
  EXPECT_EQ(store.Get().FirstYear(), 1998);
  EXPECT_EQ(store.Get().LastYear(), 2023);
}

TEST(CalendarConfigStoreTest, FittedYearsOverrideAnIncomingSpan) {
  domain::CalendarConfigTopic topic;
  CalendarConfigStore store(topic);
  store.ReceiveDateEntries({EntryBetween(1998, 2023)});

  CalendarConfig edited;
  edited.SetYears({.first_year = 2040, .last_year = 2041});
  store.ReceiveCalendarConfig(edited);

  EXPECT_EQ(store.Get().FirstYear(), 1998);
  EXPECT_EQ(store.Get().LastYear(), 2023);
}

TEST(CalendarConfigStoreTest, ManualSpanIgnoresTheEntries) {
  domain::CalendarConfigTopic topic;
  CalendarConfigStore store(topic);
  CalendarConfig manual;
  manual.SetFitYearsToEntries(false);
  manual.SetYears({.first_year = 2040, .last_year = 2041});
  store.ReceiveCalendarConfig(manual);

  store.ReceiveDateEntries({EntryBetween(1998, 2023)});

  EXPECT_EQ(store.Get().FirstYear(), 2040);
  EXPECT_EQ(store.Get().LastYear(), 2041);
}

TEST(CalendarConfigStoreTest, EntriesWithinTheSameYearsPublishNothing) {
  domain::CalendarConfigTopic topic;
  CalendarConfigStore store(topic);
  store.ReceiveDateEntries({EntryBetween(1998, 2023)});
  int emissions = 0;
  QObject::connect(&topic, &domain::CalendarConfigTopic::Published,
                   [&](const CalendarConfig&) { ++emissions; });

  store.ReceiveDateEntries(
      {EntryBetween(1998, 2000), EntryBetween(2010, 2023)});

  EXPECT_EQ(emissions, 0);
}
