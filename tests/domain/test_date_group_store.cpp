#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "domain/date_group.hpp"
#include "domain/date_group_store.hpp"

namespace {

std::vector<DateGroup> MakeGroups(std::initializer_list<const char*> names) {
  std::vector<DateGroup> groups;
  groups.reserve(names.size());
  for (const auto* name : names) {
    groups.emplace_back(std::string{name});
  }
  return groups;
}

}  // namespace

TEST(DateGroupStoreTest, ReceiveAssignsAndRenumbersGroups) {
  DateGroupStore store;
  store.ReceiveDateGroups(MakeGroups({"alpha", "beta", "gamma"}));

  const auto& groups = store.GetDateGroups();
  ASSERT_EQ(groups.size(), 3U);
  EXPECT_EQ(groups[0].GetNumber(), 0);
  EXPECT_EQ(groups[1].GetNumber(), 1);
  EXPECT_EQ(groups[2].GetNumber(), 2);
}

TEST(DateGroupStoreTest, ReceiveEmitsSignalToConnectedSlot) {
  DateGroupStore store;
  int call_count = 0;
  std::size_t observed_size = 0;
  store.SignalDateGroups().connect([&](const std::vector<DateGroup>& groups) {
    ++call_count;
    observed_size = groups.size();
  });

  store.ReceiveDateGroups(MakeGroups({"a", "b"}));

  EXPECT_EQ(call_count, 1);
  EXPECT_EQ(observed_size, 2U);
}

TEST(DateGroupStoreTest, GetNumberAndGetNameRoundTrip) {
  DateGroupStore store;
  store.ReceiveDateGroups(MakeGroups({"x", "y", "z"}));
  EXPECT_EQ(store.GetNumber("y"), 1);
  EXPECT_EQ(store.GetName(2), "z");
}

TEST(DateGroupStoreTest, GetNumberThrowsForUnknownName) {
  DateGroupStore store;
  store.ReceiveDateGroups(MakeGroups({"only"}));
  EXPECT_THROW((void)store.GetNumber("missing"), std::runtime_error);
}

TEST(DateGroupStoreTest, GetGroupMaxReflectsCurrentSize) {
  DateGroupStore store;
  EXPECT_EQ(store.GetGroupMax(), -1);
  store.ReceiveDateGroups(MakeGroups({"a"}));
  EXPECT_EQ(store.GetGroupMax(), 0);
  store.ReceiveDateGroups(MakeGroups({"a", "b", "c", "d"}));
  EXPECT_EQ(store.GetGroupMax(), 3);
}

TEST(DateGroupStoreTest, SendDefaultValuesEmitsOneDefaultGroup) {
  DateGroupStore store;
  std::vector<DateGroup> captured;
  store.SignalDateGroups().connect(
      [&](const std::vector<DateGroup>& groups) { captured = groups; });

  store.SendDefaultValues();

  ASSERT_EQ(captured.size(), 1U);
  EXPECT_EQ(captured[0].GetName(), "Default");
}

TEST(DateGroupStoreTest, ReentryGuardBlocksRecursiveReceive) {
  // A slot that re-calls Receive on the same store would cause an infinite
  // signal storm. The re-entry guard must suppress the inner call entirely.
  DateGroupStore store;
  int outer_emissions = 0;
  store.SignalDateGroups().connect(
      [&](const std::vector<DateGroup>& /*groups*/) {
        ++outer_emissions;
        if (outer_emissions == 1) {
          // Try to re-enter from inside the slot. The guard must drop this.
          store.ReceiveDateGroups(MakeGroups({"recursive"}));
        }
      });

  store.ReceiveDateGroups(MakeGroups({"initial"}));

  EXPECT_EQ(outer_emissions, 1);
  ASSERT_EQ(store.GetDateGroups().size(), 1U);
  EXPECT_EQ(store.GetDateGroups()[0].GetName(), "initial");
}
