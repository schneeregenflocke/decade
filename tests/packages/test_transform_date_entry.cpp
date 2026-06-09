#include <gtest/gtest.h>

#include <boost/date_time/gregorian/greg_date.hpp>
#include <boost/date_time/period.hpp>
#include <vector>

#include "packages/date_entry.hpp"
#include "packages/transform_date_entry.hpp"

namespace {

DateEntry MakeEntry(int year, int month, int day_begin, int day_end) {
  DateEntry entry;
  entry.SetDateInterval(boost::gregorian::date_period(
      boost::gregorian::date(static_cast<unsigned short>(year),
                             static_cast<unsigned short>(month),
                             static_cast<unsigned short>(day_begin)),
      boost::gregorian::date(static_cast<unsigned short>(year),
                             static_cast<unsigned short>(month),
                             static_cast<unsigned short>(day_end))));
  return entry;
}

}  // namespace

TEST(TransformDateEntryTest, IdentityTransformLeavesIntervalsUnchanged) {
  TransformDateEntry transformer;
  transformer.SetTransform({.begin_days = 0, .end_days = 0});

  std::vector<DateEntry> captured;
  transformer.SignalTransformDateEntries().connect(
      [&](const std::vector<DateEntry>& value) { captured = value; });

  std::vector<DateEntry> input;
  input.push_back(MakeEntry(2030, 6, 10, 20));
  transformer.ReceiveDateEntries(input);

  ASSERT_EQ(captured.size(), 1U);
  EXPECT_EQ(captured[0].GetDateInterval().begin().day(), 10U);
  EXPECT_EQ(captured[0].GetDateInterval().end().day(), 20U);
}

TEST(TransformDateEntryTest, ShiftsBeginAndEndIndependently) {
  TransformDateEntry transformer;
  transformer.SetTransform({.begin_days = -2, .end_days = 3});

  std::vector<DateEntry> captured;
  transformer.SignalTransformDateEntries().connect(
      [&](const std::vector<DateEntry>& value) { captured = value; });

  std::vector<DateEntry> input;
  input.push_back(MakeEntry(2030, 6, 10, 20));
  transformer.ReceiveDateEntries(input);

  ASSERT_EQ(captured.size(), 1U);
  EXPECT_EQ(captured[0].GetDateInterval().begin().day(), 8U);
  EXPECT_EQ(captured[0].GetDateInterval().end().day(), 23U);
}

TEST(TransformDateEntryTest, InputTransformedInvertsShift) {
  TransformDateEntry transformer;
  transformer.SetTransform({.begin_days = 1, .end_days = 5});

  std::vector<DateEntry> captured;
  transformer.SignalDateEntries().connect(
      [&](const std::vector<DateEntry>& value) { captured = value; });

  std::vector<DateEntry> input;
  input.push_back(MakeEntry(2030, 6, 10, 20));
  transformer.InputTransformedDateIntervals(input);

  ASSERT_EQ(captured.size(), 1U);
  EXPECT_EQ(captured[0].GetDateInterval().begin().day(), 9U);
  EXPECT_EQ(captured[0].GetDateInterval().end().day(), 15U);
}

TEST(TransformDateEntryTest, ReentryGuardBlocksRecursiveReceive) {
  TransformDateEntry transformer;
  transformer.SetTransform({.begin_days = 0, .end_days = 1});

  std::vector<DateEntry> recursive_input;
  recursive_input.push_back(MakeEntry(2099, 1, 1, 10));

  int emissions = 0;
  transformer.SignalTransformDateEntries().connect(
      [&](const std::vector<DateEntry>&) {
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
