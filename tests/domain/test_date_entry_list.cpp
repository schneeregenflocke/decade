#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <vector>

#include "domain/date.hpp"
#include "domain/date_entry.hpp"
#include "domain/date_entry_list.hpp"
#include "domain/date_group.hpp"
#include "domain/date_period.hpp"

// Characterisation test over DateEntryList::Assign — the funnel every entry
// passes through, whatever its source: the XML load, the CSV import, a table
// edit, a group deletion. Assign derives four things at once (it drops null
// periods, sorts, clamps groups, then numbers entries, gap periods and group
// numbers), and each derivation is silent: a wrong one shows up as a wrong
// calendar, never as an error.
//
// It freezes the behaviour of 2026-08-06 as the net for the restructuring
// around it (#26, #46, #55) — including the gaps. Where an expectation states
// something faulty, it says so and names the issue; whoever fixes that flips
// the expectation deliberately, in the same commit, instead of discovering it
// by way of a broken calendar.

namespace {

DateEntry MakeEntry(int begin_year, int begin_month, int begin_day,
                    int end_year, int end_month, int end_day, int group = 0) {
  DateEntry entry;
  entry.SetDateInterval(
      DatePeriod(Date::FromYmd(begin_year, begin_month, begin_day),
                 Date::FromYmd(end_year, end_month, end_day)));
  entry.SetGroup(group);
  return entry;
}

std::vector<DateGroup> MakeGroups(std::size_t count) {
  std::vector<DateGroup> groups;
  groups.reserve(count);
  for (std::size_t index = 0; index < count; ++index) {
    groups.emplace_back("group " + std::to_string(index));
  }
  return groups;
}

}  // namespace

// --- The order of the pipeline ---

// Null periods go before the sort, so numbering counts the survivors alone —
// not the positions of the input.
TEST(DateEntryListCharacterisation, NumbersCountSurvivorsNotInputPositions) {
  DateEntryList list;
  std::vector<DateEntry> incoming;
  incoming.push_back(MakeEntry(2030, 3, 1, 2030, 3, 5));
  incoming.push_back(MakeEntry(2030, 2, 1, 2030, 2, 1));  // null: end == begin
  incoming.push_back(MakeEntry(2030, 1, 1, 2030, 1, 5));
  list.Assign(incoming);

  ASSERT_EQ(list.Items().size(), 2U);
  EXPECT_EQ(list.Items()[0].GetNumber(), 0);
  EXPECT_EQ(list.Items()[1].GetNumber(), 1);
  // Sorted by Begin(), so January comes first despite standing last.
  EXPECT_EQ(list.Items()[0].GetDateInterval().Begin(),
            Date::FromYmd(2030, 1, 1));
  EXPECT_EQ(list.Items()[1].GetDateInterval().Begin(),
            Date::FromYmd(2030, 3, 1));
}

// An incoming number carries no weight — Assign overwrites it. The caller
// cannot pin an entry's position by handing one in.
TEST(DateEntryListCharacterisation, IncomingNumbersAreOverwritten) {
  DateEntryList list;
  DateEntry entry = MakeEntry(2030, 1, 1, 2030, 1, 5);
  entry.SetNumber(42);
  list.Assign({entry});

  ASSERT_EQ(list.Items().size(), 1U);
  EXPECT_EQ(list.Items()[0].GetNumber(), 0);
}

// --- The gap period between two entries ---

// The gap is half-open too: [previous end, next begin). Directly adjacent
// entries therefore get an empty, not a one-day gap.
TEST(DateEntryListCharacterisation, GapPeriodSpansPreviousEndToNextBegin) {
  DateEntryList list;
  list.Assign(
      {MakeEntry(2030, 1, 1, 2030, 1, 10), MakeEntry(2030, 2, 1, 2030, 2, 10)});

  ASSERT_EQ(list.Items().size(), 2U);
  const DatePeriod& gap = list.Items()[0].GetDateInterInterval();
  EXPECT_EQ(gap.Begin(), Date::FromYmd(2030, 1, 10));
  EXPECT_EQ(gap.End(), Date::FromYmd(2030, 2, 1));
  EXPECT_EQ(gap.LengthDays(), 22);
}

TEST(DateEntryListCharacterisation, AdjacentEntriesGetANullGap) {
  DateEntryList list;
  list.Assign({MakeEntry(2030, 1, 1, 2030, 1, 10),
               MakeEntry(2030, 1, 10, 2030, 1, 20)});

  ASSERT_EQ(list.Items().size(), 2U);
  EXPECT_TRUE(list.Items()[0].GetDateInterInterval().IsNull());
}

// Overlapping entries produce a *reversed* gap, and Assign lets it stand:
// AssignInterIntervals constructs the period unchecked, so end < begin. Only
// the consumer sees it as null.
TEST(DateEntryListCharacterisation, OverlappingEntriesLeaveAReversedGap) {
  DateEntryList list;
  list.Assign(
      {MakeEntry(2030, 1, 1, 2030, 3, 1), MakeEntry(2030, 2, 1, 2030, 4, 1)});

  ASSERT_EQ(list.Items().size(), 2U);
  const DatePeriod& gap = list.Items()[0].GetDateInterInterval();
  EXPECT_EQ(gap.Begin(), Date::FromYmd(2030, 3, 1));
  EXPECT_EQ(gap.End(), Date::FromYmd(2030, 2, 1));
  EXPECT_TRUE(gap.IsNull());
}

// The loop stops at the second to last entry, so the last one keeps whatever
// gap it brought along. Assign copies the incoming entries wholesale, and a
// stale gap out of a project file therefore survives the funnel.
TEST(DateEntryListCharacterisation, LastEntryKeepsItsIncomingGap) {
  DateEntryList list;
  DateEntry last = MakeEntry(2030, 5, 1, 2030, 5, 10);
  last.SetDateInterInterval(
      DatePeriod(Date::FromYmd(1999, 1, 1), Date::FromYmd(1999, 12, 31)));
  list.Assign({MakeEntry(2030, 1, 1, 2030, 1, 10), last});

  ASSERT_EQ(list.Items().size(), 2U);
  EXPECT_EQ(list.Items()[1].GetDateInterInterval().Begin(),
            Date::FromYmd(1999, 1, 1));
}

// --- The number within the group ---

// Counted per group and zero-based, in the sorted order.
TEST(DateEntryListCharacterisation, GroupNumbersCountPerGroupFromZero) {
  DateEntryList list;
  list.AssignDateGroups(MakeGroups(2));
  list.Assign({MakeEntry(2030, 1, 1, 2030, 1, 5, /*group=*/0),
               MakeEntry(2030, 2, 1, 2030, 2, 5, /*group=*/1),
               MakeEntry(2030, 3, 1, 2030, 3, 5, /*group=*/0),
               MakeEntry(2030, 4, 1, 2030, 4, 5, /*group=*/1)});

  ASSERT_EQ(list.Items().size(), 4U);
  EXPECT_EQ(list.Items()[0].GetGroupNumber(), 0);
  EXPECT_EQ(list.Items()[1].GetGroupNumber(), 0);
  EXPECT_EQ(list.Items()[2].GetGroupNumber(), 1);
  EXPECT_EQ(list.Items()[3].GetGroupNumber(), 1);
}

// --- Clamping the group ---

// Without groups GetGroupMax() is -1, so every non-negative group falls to 0.
TEST(DateEntryListCharacterisation, WithoutGroupsEveryGroupFallsToZero) {
  DateEntryList list;
  list.Assign({MakeEntry(2030, 1, 1, 2030, 1, 5, /*group=*/3)});

  ASSERT_EQ(list.Items().size(), 1U);
  EXPECT_EQ(list.Items()[0].GetGroup(), 0);
}

TEST(DateEntryListCharacterisation, GroupBeyondTheLastOneFallsToZero) {
  DateEntryList list;
  list.AssignDateGroups(MakeGroups(2));  // valid: 0 and 1
  list.Assign({MakeEntry(2030, 1, 1, 2030, 1, 5, /*group=*/1),
               MakeEntry(2030, 2, 1, 2030, 2, 5, /*group=*/2)});

  ASSERT_EQ(list.Items().size(), 2U);
  EXPECT_EQ(list.Items()[0].GetGroup(), 1);
  EXPECT_EQ(list.Items()[1].GetGroup(), 0);
}

// The gap of #26, frozen as it stands: the clamp tests the upper bound alone,
// so a negative group walks through untouched. It then reaches
// group_nodes.at(current_group) in calendar_section_builders.hpp and throws
// std::out_of_range, which nobody catches. Whoever closes #26 turns this
// expectation into GetGroup() == 0.
TEST(DateEntryListCharacterisation, NegativeGroupSurvivesTheClamp) {
  DateEntryList list;
  list.AssignDateGroups(MakeGroups(2));
  list.Assign({MakeEntry(2030, 1, 1, 2030, 1, 5, /*group=*/-3)});

  ASSERT_EQ(list.Items().size(), 1U);
  EXPECT_EQ(list.Items()[0].GetGroup(), -3);
}

// The clamp reads the groups known at that moment: a group assignment after
// Assign does not reach the entries already stored. That is the second half of
// #26 — DateEntryStore::ReceiveDateGroups does exactly this and re-runs no
// Assign, so shrinking the groups leaves stale group numbers behind until the
// next entry change.
TEST(DateEntryListCharacterisation, LaterGroupsDoNotReclampStoredEntries) {
  DateEntryList list;
  list.AssignDateGroups(MakeGroups(4));
  list.Assign({MakeEntry(2030, 1, 1, 2030, 1, 5, /*group=*/3)});
  ASSERT_EQ(list.Items()[0].GetGroup(), 3);

  list.AssignDateGroups(MakeGroups(1));  // group 3 no longer exists

  EXPECT_EQ(list.Items()[0].GetGroup(), 3);
}

// --- The year span ---

// LengthDays and the span read the half-open period: an entry ending on
// 2031-01-01 covers no day of 2031.
TEST(DateEntryListCharacterisation, SpanUsesTheLastDayNotTheEndDate) {
  DateEntryList list;
  list.Assign({MakeEntry(2030, 1, 1, 2031, 1, 1)});

  EXPECT_EQ(list.FirstYear(), 2030);
  EXPECT_EQ(list.LastYear(), 2030);
  EXPECT_EQ(list.YearSpan(), 1U);
}

TEST(DateEntryListCharacterisation, EmptyListReportsZeroYears) {
  DateEntryList list;
  list.Assign({});

  EXPECT_TRUE(list.IsEmpty());
  EXPECT_EQ(list.FirstYear(), 0);
  EXPECT_EQ(list.LastYear(), 0);
  EXPECT_EQ(list.YearSpan(), 0U);
}
