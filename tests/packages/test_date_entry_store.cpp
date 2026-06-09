#include <gtest/gtest.h>

#include <boost/date_time/gregorian/greg_date.hpp>
#include <boost/date_time/period.hpp>
#include <vector>

#include "packages/date_entry.hpp"
#include "packages/date_entry_bar_store.hpp"
#include "packages/date_entry_store.hpp"
#include "packages/date_group.hpp"

namespace {

DateEntry MakeEntry(int year, int month_begin, int day_begin, int month_end,
                    int day_end) {
  DateEntry entry;
  entry.SetDateInterval(boost::gregorian::date_period(
      boost::gregorian::date(static_cast<unsigned short>(year),
                             static_cast<unsigned short>(month_begin),
                             static_cast<unsigned short>(day_begin)),
      boost::gregorian::date(static_cast<unsigned short>(year),
                             static_cast<unsigned short>(month_end),
                             static_cast<unsigned short>(day_end))));
  return entry;
}

// In production the wiring guarantees a `Default` group is delivered before any
// entry; seeding it here mirrors that setup so the store sees the same initial
// state it does at runtime.
void SeedDefaultGroup(DateEntryStore& store) {
  std::vector<DateGroup> groups;
  groups.emplace_back("Default");
  store.ReceiveDateGroups(groups);
}

}  // namespace

TEST(DateEntryStoreTest, ReceiveSortsByBeginDate) {
  DateEntryStore store;
  SeedDefaultGroup(store);
  std::vector<DateEntry> input;
  input.push_back(MakeEntry(2030, 6, 1, 6, 10));
  input.push_back(MakeEntry(2030, 1, 1, 1, 10));
  input.push_back(MakeEntry(2030, 3, 1, 3, 10));

  store.ReceiveDateEntries(input);

  const auto& stored = store.GetDateEntries();
  ASSERT_EQ(stored.size(), 3U);
  EXPECT_EQ(stored[0].GetDateInterval().begin().month(), 1U);
  EXPECT_EQ(stored[1].GetDateInterval().begin().month(), 3U);
  EXPECT_EQ(stored[2].GetDateInterval().begin().month(), 6U);
}

TEST(DateEntryStoreTest, ReceiveAssignsSequentialNumbers) {
  DateEntryStore store;
  SeedDefaultGroup(store);
  std::vector<DateEntry> input;
  input.push_back(MakeEntry(2030, 1, 1, 1, 10));
  input.push_back(MakeEntry(2030, 2, 1, 2, 10));
  input.push_back(MakeEntry(2030, 3, 1, 3, 10));

  store.ReceiveDateEntries(input);

  const auto& stored = store.GetDateEntries();
  ASSERT_EQ(stored.size(), 3U);
  EXPECT_EQ(stored[0].GetNumber(), 0);
  EXPECT_EQ(stored[1].GetNumber(), 1);
  EXPECT_EQ(stored[2].GetNumber(), 2);
}

TEST(DateEntryStoreTest, SpanReflectsFirstAndLastYear) {
  DateEntryStore store;
  SeedDefaultGroup(store);
  std::vector<DateEntry> input;
  input.push_back(MakeEntry(2030, 1, 1, 1, 10));
  input.push_back(MakeEntry(2034, 1, 1, 1, 10));
  store.ReceiveDateEntries(input);

  EXPECT_EQ(store.GetFirstYear(), 2030);
  EXPECT_EQ(store.GetLastYear(), 2034);
  EXPECT_EQ(store.GetSpan(), 5);
}

TEST(DateEntryStoreTest, EmitsSignalOnReceive) {
  DateEntryStore store;
  SeedDefaultGroup(store);
  int emissions = 0;
  store.SignalDateEntries().connect(
      [&](const std::vector<DateEntry>&) { ++emissions; });

  std::vector<DateEntry> input;
  input.push_back(MakeEntry(2030, 1, 1, 1, 10));
  store.ReceiveDateEntries(input);

  EXPECT_EQ(emissions, 1);
}

TEST(DateEntryStoreTest, ReentryGuardBlocksRecursiveReceive) {
  DateEntryStore store;
  SeedDefaultGroup(store);
  std::vector<DateEntry> recursive_input;
  recursive_input.push_back(MakeEntry(2099, 1, 1, 1, 10));

  int emissions = 0;
  store.SignalDateEntries().connect([&](const std::vector<DateEntry>&) {
    ++emissions;
    if (emissions == 1) {
      store.ReceiveDateEntries(recursive_input);
    }
  });

  std::vector<DateEntry> input;
  input.push_back(MakeEntry(2030, 1, 1, 1, 10));
  store.ReceiveDateEntries(input);

  EXPECT_EQ(emissions, 1);
  ASSERT_EQ(store.GetDateEntries().size(), 1U);
  EXPECT_EQ(store.GetDateEntries()[0].GetDateInterval().begin().year(), 2030U);
}

TEST(DateEntryBarStoreTest, ProducesOneBarPerIntervalWithinYear) {
  DateEntryBarStore store;
  SeedDefaultGroup(store);
  std::vector<DateEntry> input;
  input.push_back(MakeEntry(2030, 1, 1, 1, 10));
  input.push_back(MakeEntry(2030, 2, 1, 2, 10));

  store.ReceiveDateEntries(input);

  EXPECT_EQ(store.GetNumberBars(), 2U);
}
