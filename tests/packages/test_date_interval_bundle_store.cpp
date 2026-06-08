#include <gtest/gtest.h>

#include <boost/date_time/gregorian/greg_date.hpp>
#include <boost/date_time/period.hpp>
#include <vector>

#include "packages/date_group.hpp"
#include "packages/date_interval_bundle.hpp"
#include "packages/date_interval_bundle_bar_store.hpp"
#include "packages/date_interval_bundle_store.hpp"

namespace {

DateIntervalBundle MakeBundle(int year, int month_begin, int day_begin,
                              int month_end, int day_end) {
  DateIntervalBundle bundle;
  bundle.SetDateInterval(boost::gregorian::date_period(
      boost::gregorian::date(static_cast<unsigned short>(year),
                             static_cast<unsigned short>(month_begin),
                             static_cast<unsigned short>(day_begin)),
      boost::gregorian::date(static_cast<unsigned short>(year),
                             static_cast<unsigned short>(month_end),
                             static_cast<unsigned short>(day_end))));
  return bundle;
}

// `DateIntervalBundleStore::ReceiveDateIntervalBundles` runs
// `AdjustGroupExcludeFlag`, which dereferences `date_group_store.GetExclude(0)`
// and asserts a non-empty group list. In production the wiring guarantees a
// `Default` group is delivered before any bundle; tests must seed it manually.
void SeedDefaultGroup(DateIntervalBundleStore& store) {
  std::vector<DateGroup> groups;
  groups.emplace_back("Default");
  store.ReceiveDateGroups(groups);
}

}  // namespace

TEST(DateIntervalBundleStoreTest, ReceiveSortsByBeginDate) {
  DateIntervalBundleStore store;
  SeedDefaultGroup(store);
  std::vector<DateIntervalBundle> input;
  input.push_back(MakeBundle(2030, 6, 1, 6, 10));
  input.push_back(MakeBundle(2030, 1, 1, 1, 10));
  input.push_back(MakeBundle(2030, 3, 1, 3, 10));

  store.ReceiveDateIntervalBundles(input);

  const auto& stored = store.GetDateIntervalBundles();
  ASSERT_EQ(stored.size(), 3U);
  EXPECT_EQ(stored[0].GetDateInterval().begin().month(), 1U);
  EXPECT_EQ(stored[1].GetDateInterval().begin().month(), 3U);
  EXPECT_EQ(stored[2].GetDateInterval().begin().month(), 6U);
}

TEST(DateIntervalBundleStoreTest, ReceiveAssignsSequentialNumbers) {
  DateIntervalBundleStore store;
  SeedDefaultGroup(store);
  std::vector<DateIntervalBundle> input;
  input.push_back(MakeBundle(2030, 1, 1, 1, 10));
  input.push_back(MakeBundle(2030, 2, 1, 2, 10));
  input.push_back(MakeBundle(2030, 3, 1, 3, 10));

  store.ReceiveDateIntervalBundles(input);

  const auto& stored = store.GetDateIntervalBundles();
  ASSERT_EQ(stored.size(), 3U);
  EXPECT_EQ(stored[0].GetNumber(), 0);
  EXPECT_EQ(stored[1].GetNumber(), 1);
  EXPECT_EQ(stored[2].GetNumber(), 2);
}

TEST(DateIntervalBundleStoreTest, SpanReflectsFirstAndLastYear) {
  DateIntervalBundleStore store;
  SeedDefaultGroup(store);
  std::vector<DateIntervalBundle> input;
  input.push_back(MakeBundle(2030, 1, 1, 1, 10));
  input.push_back(MakeBundle(2034, 1, 1, 1, 10));
  store.ReceiveDateIntervalBundles(input);

  EXPECT_EQ(store.GetFirstYear(), 2030);
  EXPECT_EQ(store.GetLastYear(), 2034);
  EXPECT_EQ(store.GetSpan(), 5);
}

TEST(DateIntervalBundleStoreTest, EmitsSignalOnReceive) {
  DateIntervalBundleStore store;
  SeedDefaultGroup(store);
  int emissions = 0;
  store.SignalDateIntervalBundles().connect(
      [&](const std::vector<DateIntervalBundle>&) { ++emissions; });

  std::vector<DateIntervalBundle> input;
  input.push_back(MakeBundle(2030, 1, 1, 1, 10));
  store.ReceiveDateIntervalBundles(input);

  EXPECT_EQ(emissions, 1);
}

TEST(DateIntervalBundleStoreTest, ReentryGuardBlocksRecursiveReceive) {
  DateIntervalBundleStore store;
  SeedDefaultGroup(store);
  std::vector<DateIntervalBundle> recursive_input;
  recursive_input.push_back(MakeBundle(2099, 1, 1, 1, 10));

  int emissions = 0;
  store.SignalDateIntervalBundles().connect(
      [&](const std::vector<DateIntervalBundle>&) {
        ++emissions;
        if (emissions == 1) {
          store.ReceiveDateIntervalBundles(recursive_input);
        }
      });

  std::vector<DateIntervalBundle> input;
  input.push_back(MakeBundle(2030, 1, 1, 1, 10));
  store.ReceiveDateIntervalBundles(input);

  EXPECT_EQ(emissions, 1);
  ASSERT_EQ(store.GetDateIntervalBundles().size(), 1U);
  EXPECT_EQ(store.GetDateIntervalBundles()[0].GetDateInterval().begin().year(),
            2030U);
}

TEST(DateIntervalBundleBarStoreTest, ProducesOneBarPerIntervalWithinYear) {
  DateIntervalBundleBarStore store;
  SeedDefaultGroup(store);
  std::vector<DateIntervalBundle> input;
  input.push_back(MakeBundle(2030, 1, 1, 1, 10));
  input.push_back(MakeBundle(2030, 2, 1, 2, 10));

  store.ReceiveDateIntervalBundles(input);

  EXPECT_EQ(store.GetNumberBars(), 2U);
}
