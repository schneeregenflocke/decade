#include <gtest/gtest.h>

#include <boost/date_time/gregorian/greg_date.hpp>
#include <boost/date_time/period.hpp>
#include <vector>

#include "packages/date_interval_bundle.hpp"
#include "packages/transform_date_interval_bundle.hpp"

namespace {

DateIntervalBundle MakeBundle(int year, int month, int day_begin, int day_end) {
  DateIntervalBundle bundle;
  bundle.SetDateInterval(boost::gregorian::date_period(
      boost::gregorian::date(static_cast<unsigned short>(year),
                             static_cast<unsigned short>(month),
                             static_cast<unsigned short>(day_begin)),
      boost::gregorian::date(static_cast<unsigned short>(year),
                             static_cast<unsigned short>(month),
                             static_cast<unsigned short>(day_end))));
  return bundle;
}

}  // namespace

TEST(TransformDateIntervalBundleTest,
     IdentityTransformLeavesIntervalsUnchanged) {
  TransformDateIntervalBundle transformer;
  transformer.SetTransform({.begin_days = 0, .end_days = 0});

  std::vector<DateIntervalBundle> captured;
  transformer.SignalTransformDateIntervalBundles().connect(
      [&](const std::vector<DateIntervalBundle>& value) { captured = value; });

  std::vector<DateIntervalBundle> input;
  input.push_back(MakeBundle(2030, 6, 10, 20));
  transformer.ReceiveDateIntervalBundles(input);

  ASSERT_EQ(captured.size(), 1U);
  EXPECT_EQ(captured[0].GetDateInterval().begin().day(), 10U);
  EXPECT_EQ(captured[0].GetDateInterval().end().day(), 20U);
}

TEST(TransformDateIntervalBundleTest, ShiftsBeginAndEndIndependently) {
  TransformDateIntervalBundle transformer;
  transformer.SetTransform({.begin_days = -2, .end_days = 3});

  std::vector<DateIntervalBundle> captured;
  transformer.SignalTransformDateIntervalBundles().connect(
      [&](const std::vector<DateIntervalBundle>& value) { captured = value; });

  std::vector<DateIntervalBundle> input;
  input.push_back(MakeBundle(2030, 6, 10, 20));
  transformer.ReceiveDateIntervalBundles(input);

  ASSERT_EQ(captured.size(), 1U);
  EXPECT_EQ(captured[0].GetDateInterval().begin().day(), 8U);
  EXPECT_EQ(captured[0].GetDateInterval().end().day(), 23U);
}

TEST(TransformDateIntervalBundleTest, InputTransformedInvertsShift) {
  TransformDateIntervalBundle transformer;
  transformer.SetTransform({.begin_days = 1, .end_days = 5});

  std::vector<DateIntervalBundle> captured;
  transformer.SignalDateIntervalBundles().connect(
      [&](const std::vector<DateIntervalBundle>& value) { captured = value; });

  std::vector<DateIntervalBundle> input;
  input.push_back(MakeBundle(2030, 6, 10, 20));
  transformer.InputTransformedDateIntervals(input);

  ASSERT_EQ(captured.size(), 1U);
  EXPECT_EQ(captured[0].GetDateInterval().begin().day(), 9U);
  EXPECT_EQ(captured[0].GetDateInterval().end().day(), 15U);
}

TEST(TransformDateIntervalBundleTest, ReentryGuardBlocksRecursiveReceive) {
  TransformDateIntervalBundle transformer;
  transformer.SetTransform({.begin_days = 0, .end_days = 1});

  std::vector<DateIntervalBundle> recursive_input;
  recursive_input.push_back(MakeBundle(2099, 1, 1, 10));

  int emissions = 0;
  transformer.SignalTransformDateIntervalBundles().connect(
      [&](const std::vector<DateIntervalBundle>&) {
        ++emissions;
        if (emissions == 1) {
          transformer.ReceiveDateIntervalBundles(recursive_input);
        }
      });

  std::vector<DateIntervalBundle> input;
  input.push_back(MakeBundle(2030, 1, 1, 10));
  transformer.ReceiveDateIntervalBundles(input);

  EXPECT_EQ(emissions, 1);
}
