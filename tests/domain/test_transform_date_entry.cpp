#include <gtest/gtest.h>

#include <vector>

#include "domain/date.hpp"
#include "domain/date_entry.hpp"
#include "domain/date_period.hpp"
#include "domain/state_topic.hpp"
#include "domain/transform_date_entry.hpp"

namespace {

DateEntry MakeEntry(int year, int month, int day_begin, int day_end) {
  DateEntry entry;
  entry.SetDateInterval(DatePeriod(Date::FromYmd(year, month, day_begin),
                                   Date::FromYmd(year, month, day_end)));
  return entry;
}

}  // namespace

TEST(TransformDateEntryTest, IdentityTransformLeavesIntervalsUnchanged) {
  domain::StateTopic<std::vector<DateEntry>> topic;
  TransformDateEntry transformer(topic);
  transformer.SetTransform({.begin_days = 0, .end_days = 0});

  std::vector<DateEntry> captured;
  topic.connect([&](const std::vector<DateEntry>& value) { captured = value; });

  std::vector<DateEntry> input;
  input.push_back(MakeEntry(2030, 6, 10, 20));
  transformer.ReceiveDateEntries(input);

  ASSERT_EQ(captured.size(), 1U);
  EXPECT_EQ(captured[0].GetDateInterval().Begin().Day(), 10);
  EXPECT_EQ(captured[0].GetDateInterval().End().Day(), 20);
}

TEST(TransformDateEntryTest, ShiftsBeginAndEndIndependently) {
  domain::StateTopic<std::vector<DateEntry>> topic;
  TransformDateEntry transformer(topic);
  transformer.SetTransform({.begin_days = -2, .end_days = 3});

  std::vector<DateEntry> captured;
  topic.connect([&](const std::vector<DateEntry>& value) { captured = value; });

  std::vector<DateEntry> input;
  input.push_back(MakeEntry(2030, 6, 10, 20));
  transformer.ReceiveDateEntries(input);

  ASSERT_EQ(captured.size(), 1U);
  EXPECT_EQ(captured[0].GetDateInterval().Begin().Day(), 8);
  EXPECT_EQ(captured[0].GetDateInterval().End().Day(), 23);
}

TEST(TransformDateEntryTest, ReentryGuardBlocksRecursiveReceive) {
  domain::StateTopic<std::vector<DateEntry>> topic;
  TransformDateEntry transformer(topic);
  transformer.SetTransform({.begin_days = 0, .end_days = 1});

  std::vector<DateEntry> recursive_input;
  recursive_input.push_back(MakeEntry(2099, 1, 1, 10));

  int emissions = 0;
  topic.connect([&](const std::vector<DateEntry>&) {
    ++emissions;
    if (emissions == 1) {
      transformer.ReceiveDateEntries(recursive_input);
    }
  });

  std::vector<DateEntry> input;
  input.push_back(MakeEntry(2030, 1, 1, 10));
  transformer.ReceiveDateEntries(input);

  EXPECT_EQ(emissions, 1);
}
