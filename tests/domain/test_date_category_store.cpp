#include <gtest/gtest.h>

#include <QtCore/QObject>
#include <stdexcept>
#include <string>
#include <vector>

#include "domain/date_category.hpp"
#include "domain/date_category_store.hpp"
#include "domain/state_topics.hpp"

namespace {

std::vector<DateCategory> MakeCategories(
    std::initializer_list<const char*> names) {
  std::vector<DateCategory> categories;
  categories.reserve(names.size());
  for (const auto* name : names) {
    categories.emplace_back(std::string{name});
  }
  return categories;
}

}  // namespace

TEST(DateCategoryStoreTest, ReceiveAssignsAndRenumbersCategories) {
  domain::DateCategoriesTopic topic;
  DateCategoryStore store(topic);
  store.ReceiveDateCategories(MakeCategories({"alpha", "beta", "gamma"}));

  const auto& categories = store.Get().Items();
  ASSERT_EQ(categories.size(), 3U);
  EXPECT_EQ(categories[0].GetNumber(), 0);
  EXPECT_EQ(categories[1].GetNumber(), 1);
  EXPECT_EQ(categories[2].GetNumber(), 2);
}

TEST(DateCategoryStoreTest, ReceiveEmitsSignalToConnectedSlot) {
  domain::DateCategoriesTopic topic;
  DateCategoryStore store(topic);
  int call_count = 0;
  std::size_t observed_size = 0;
  QObject::connect(&topic, &domain::DateCategoriesTopic::Published,
                   [&](const std::vector<DateCategory>& categories) {
                     ++call_count;
                     observed_size = categories.size();
                   });

  store.ReceiveDateCategories(MakeCategories({"a", "b"}));

  EXPECT_EQ(call_count, 1);
  EXPECT_EQ(observed_size, 2U);
}

TEST(DateCategoryStoreTest, GetNumberAndGetNameRoundTrip) {
  domain::DateCategoriesTopic topic;
  DateCategoryStore store(topic);
  store.ReceiveDateCategories(MakeCategories({"x", "y", "z"}));
  EXPECT_EQ(store.Get().GetNumber("y"), 1);
  EXPECT_EQ(store.Get().GetName(2), "z");
}

TEST(DateCategoryStoreTest, GetNumberThrowsForUnknownName) {
  domain::DateCategoriesTopic topic;
  DateCategoryStore store(topic);
  store.ReceiveDateCategories(MakeCategories({"only"}));
  EXPECT_THROW((void)store.Get().GetNumber("missing"), std::runtime_error);
}

TEST(DateCategoryStoreTest, GetCategoryMaxReflectsCurrentSize) {
  domain::DateCategoriesTopic topic;
  DateCategoryStore store(topic);
  EXPECT_EQ(store.Get().GetCategoryMax(), -1);
  store.ReceiveDateCategories(MakeCategories({"a"}));
  EXPECT_EQ(store.Get().GetCategoryMax(), 0);
  store.ReceiveDateCategories(MakeCategories({"a", "b", "c", "d"}));
  EXPECT_EQ(store.Get().GetCategoryMax(), 3);
}

TEST(DateCategoryStoreTest, SendDefaultValuesEmitsOneDefaultCategory) {
  domain::DateCategoriesTopic topic;
  DateCategoryStore store(topic);
  std::vector<DateCategory> captured;
  QObject::connect(&topic, &domain::DateCategoriesTopic::Published,
                   [&](const std::vector<DateCategory>& categories) {
                     captured = categories;
                   });

  store.SendDefaultValues();

  ASSERT_EQ(captured.size(), 1U);
  EXPECT_EQ(captured[0].GetName(), "Default");
}

TEST(DateCategoryStoreTest, ReentryGuardBlocksRecursiveReceive) {
  // A slot that re-calls Receive on the same store would cause an infinite
  // signal storm. The re-entry guard must suppress the inner call entirely.
  domain::DateCategoriesTopic topic;
  DateCategoryStore store(topic);
  int outer_emissions = 0;
  QObject::connect(
      &topic, &domain::DateCategoriesTopic::Published,
      [&](const std::vector<DateCategory>& /*categories*/) {
        ++outer_emissions;
        if (outer_emissions == 1) {
          // Try to re-enter from inside the slot. The guard must
          // drop this.
          store.ReceiveDateCategories(MakeCategories({"recursive"}));
        }
      });

  store.ReceiveDateCategories(MakeCategories({"initial"}));

  EXPECT_EQ(outer_emissions, 1);
  ASSERT_EQ(store.Get().Items().size(), 1U);
  EXPECT_EQ(store.Get().Items()[0].GetName(), "initial");
}
