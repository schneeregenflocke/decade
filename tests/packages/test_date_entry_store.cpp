#include <gtest/gtest.h>

#include <vector>

#include "packages/date.hpp"
#include "packages/date_entry.hpp"
#include "packages/date_entry_bar_store.hpp"
#include "packages/date_entry_store.hpp"
#include "packages/date_group.hpp"
#include "packages/date_period.hpp"

namespace {

DateEntry MakeEntry(int year, int month_begin, int day_begin, int month_end,
                    int day_end) {
  DateEntry entry;
  entry.SetDateInterval(DatePeriod(Date::FromYmd(year, month_begin, day_begin),
                                   Date::FromYmd(year, month_end, day_end)));
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
  EXPECT_EQ(stored[0].GetDateInterval().Begin().Month(), 1);
  EXPECT_EQ(stored[1].GetDateInterval().Begin().Month(), 3);
  EXPECT_EQ(stored[2].GetDateInterval().Begin().Month(), 6);
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
  EXPECT_EQ(store.GetDateEntries()[0].GetDateInterval().Begin().Year(), 2030);
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

TEST(DateEntryBarStoreTest, SplitsYearSpanningIntervalAtYearBoundary) {
  DateEntryBarStore store;
  SeedDefaultGroup(store);
  DateEntry entry;
  entry.SetDateInterval(
      DatePeriod(Date::FromYmd(2030, 12, 20), Date::FromYmd(2031, 1, 10)));
  std::vector<DateEntry> input;
  input.push_back(entry);

  store.ReceiveDateEntries(input);

  ASSERT_EQ(store.GetNumberBars(), 2U);
  EXPECT_EQ(store.GetBar(0).GetYear(), 2030);
  EXPECT_EQ(store.GetBar(1).GetYear(), 2031);
  EXPECT_EQ(store.GetBar(0).GetLength() + store.GetBar(1).GetLength(),
            Date::DaysBetween(Date::FromYmd(2030, 12, 20),
                              Date::FromYmd(2031, 1, 10)));
}

// A single day on January 1st is the half-open period [Jan 1, Jan 2): one
// bar of length 1, entirely inside its year. (Under the old inclusive-end
// model this input was the null period (d, d) whose Last() fell into the
// previous year and broke the split loop.)
TEST(DateEntryBarStoreTest, SingleDayOnJanuaryFirstProducesOneBar) {
  DateEntryBarStore store;
  SeedDefaultGroup(store);
  DateEntry entry;
  entry.SetDateInterval(
      DatePeriod(Date::FromYmd(2030, 1, 1), Date::FromYmd(2030, 1, 2)));
  std::vector<DateEntry> input;
  input.push_back(entry);

  store.ReceiveDateEntries(input);

  ASSERT_EQ(store.GetNumberBars(), 1U);
  EXPECT_EQ(store.GetBar(0).GetYear(), 2030);
  EXPECT_EQ(store.GetBar(0).GetLength(), 1);
  EXPECT_EQ(store.GetAnnualTotal(0), 1);
}

// Null periods (no contained day) carry no data and are filtered out.
TEST(DateEntryStoreTest, DropsNullPeriodEntries) {
  DateEntryStore store;
  SeedDefaultGroup(store);
  DateEntry null_entry;
  null_entry.SetDateInterval(
      DatePeriod(Date::FromYmd(2030, 1, 1), Date::FromYmd(2030, 1, 1)));
  std::vector<DateEntry> input;
  input.push_back(null_entry);
  input.push_back(MakeEntry(2030, 2, 1, 2, 10));

  store.ReceiveDateEntries(input);

  EXPECT_EQ(store.GetDateEntries().size(), 1U);
}
