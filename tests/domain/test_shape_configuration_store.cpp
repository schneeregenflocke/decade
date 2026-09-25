#include <gtest/gtest.h>

#include <QtCore/QObject>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <string>

#include "domain/shape_configuration.hpp"
#include "domain/shape_configuration_store.hpp"
#include "domain/state_topics.hpp"

TEST(ShapeConfigSetTest, DefaultsContainExpectedNames) {
  ShapeConfigSet set;
  ASSERT_FALSE(set.FixedConfigurations().empty());
  // The defaults are all fixed configurations: no category entries yet.
  EXPECT_TRUE(set.CategoryConfigurations().empty());
  EXPECT_TRUE(set.GetDynamicConfiguration(0).Key().empty());

  // A handful of named entries we expect from the default set.
  const ShapeConfiguration page_margin =
      set.GetShapeConfiguration(ShapeConfigSet::kPageMarginKey);
  EXPECT_EQ(page_margin.Key(), ShapeConfigSet::kPageMarginKey);
  EXPECT_TRUE(page_margin.OutlineVisible());

  const ShapeConfiguration day_shapes =
      set.GetShapeConfiguration(ShapeConfigSet::kDayShapesKey);
  EXPECT_EQ(day_shapes.Key(), ShapeConfigSet::kDayShapesKey);
  EXPECT_TRUE(day_shapes.FillVisible());
}

TEST(ShapeConfigSetTest, GetShapeConfigurationReturnsBlankForUnknownName) {
  ShapeConfigSet set;
  const ShapeConfiguration unknown =
      set.GetShapeConfiguration("Does Not Exist");
  EXPECT_TRUE(unknown.Key().empty());
}

TEST(ShapeConfigSetTest, DynamicConfigurationKeyMatchesFormat) {
  EXPECT_EQ(ShapeConfigSet::DynamicConfigurationKey(0), "category_0");
  EXPECT_EQ(ShapeConfigSet::DynamicConfigurationKey(7), "category_7");
}

TEST(ShapeConfigSetTest, SyncToDateCategoriesAddressesByIndex) {
  ShapeConfigSet set;

  // Grow the set to three categories, mirroring how the store reacts to date
  // categories being added.
  set.SyncToDateCategories(3);

  const ShapeConfiguration second = set.GetDynamicConfiguration(1);
  EXPECT_EQ(second.Key(), "category_1");

  // Out-of-range / absent indices return a blank configuration.
  EXPECT_TRUE(set.GetDynamicConfiguration(3).Key().empty());
}

TEST(ShapeConfigSetTest,
     SyncToDateCategoriesPreservesCustomisationAndDropsStale) {
  ShapeConfigSet set;
  set.SyncToDateCategories(3);

  // Customise the colour of the second category's configuration.
  ShapeConfiguration customised = set.GetDynamicConfiguration(1);
  ASSERT_EQ(customised.Key(), "category_1");
  customised.OutlineColor(glm::vec4{0.1F, 0.2F, 0.3F, 1.0F});
  ASSERT_TRUE(set.UpdateConfiguration(customised));

  // Shrinking then re-growing must keep the surviving category's customisation
  // and drop the entries past the new count.
  set.SyncToDateCategories(2);
  EXPECT_TRUE(set.GetDynamicConfiguration(2).Key().empty());
  const ShapeConfiguration kept = set.GetDynamicConfiguration(1);
  EXPECT_EQ(kept.Key(), "category_1");
  EXPECT_FLOAT_EQ(kept.OutlineColorDisabled()[0], 0.1F);
}

// The annual coverage aggregates the categories instead of being one, so
// neither adding nor removing a category may recolour it — least of all over a
// colour the user picked.
TEST(ShapeConfigSetTest,
     SyncToDateCategoriesKeepsTheAnnualCoverageConfiguration) {
  ShapeConfigSet set;
  const ShapeConfiguration initial =
      set.GetShapeConfiguration(ShapeConfigSet::kAnnualCoverageKey);
  ASSERT_EQ(initial.Key(), ShapeConfigSet::kAnnualCoverageKey);

  set.SyncToDateCategories(3);
  EXPECT_FLOAT_EQ(set.GetShapeConfiguration(ShapeConfigSet::kAnnualCoverageKey)
                      .FillColorDisabled()[0],
                  initial.FillColorDisabled()[0]);

  ShapeConfiguration customised =
      set.GetShapeConfiguration(ShapeConfigSet::kAnnualCoverageKey);
  customised.FillColor(glm::vec4{0.9F, 0.1F, 0.2F, 1.0F});
  ASSERT_TRUE(set.UpdateConfiguration(customised));

  set.SyncToDateCategories(5);
  set.SyncToDateCategories(2);

  const ShapeConfiguration kept =
      set.GetShapeConfiguration(ShapeConfigSet::kAnnualCoverageKey);
  EXPECT_FLOAT_EQ(kept.FillColorDisabled()[0], 0.9F);
  EXPECT_FLOAT_EQ(kept.FillColorDisabled()[1], 0.1F);
}

TEST(ShapeConfigurationTest, OutlineColorReturnsZeroWhenInvisible) {
  ShapeConfiguration shape(
      "test", /*outline_visible=*/false,
      /*fill_visible=*/false, 1.0F,
      ShapeConfiguration::OutlineColorValue{glm::vec4{1.0F, 0.0F, 0.0F, 1.0F}},
      ShapeConfiguration::FillColorValue{glm::vec4{0.0F, 1.0F, 0.0F, 1.0F}});
  const glm::vec4 outline = shape.OutlineColor();
  EXPECT_FLOAT_EQ(outline[0], 0.0F);
  EXPECT_FLOAT_EQ(outline[3], 0.0F);
  EXPECT_FLOAT_EQ(shape.LineWidth(), 0.0F);
}

TEST(ShapeConfigurationTest, OutlineColorReturnsValueWhenVisible) {
  ShapeConfiguration shape(
      "test", /*outline_visible=*/true,
      /*fill_visible=*/false, 2.5F,
      ShapeConfiguration::OutlineColorValue{glm::vec4{0.5F, 0.5F, 0.5F, 1.0F}},
      ShapeConfiguration::FillColorValue{glm::vec4{0.0F, 0.0F, 0.0F, 0.0F}});
  const glm::vec4 outline = shape.OutlineColor();
  EXPECT_FLOAT_EQ(outline[0], 0.5F);
  EXPECT_FLOAT_EQ(shape.LineWidth(), 2.5F);
}

TEST(ShapeConfigurationStoreTest, ReceiveEmitsAndCopiesContents) {
  ShapeConfigSet source;
  domain::ShapeConfigSetTopic topic;
  ShapeConfigurationStore target(topic);

  int emissions = 0;
  QObject::connect(&topic, &domain::ShapeConfigSetTopic::Published,
                   [&](const ShapeConfigSet&) { ++emissions; });

  target.ReceiveShapeConfigSet(source);

  EXPECT_EQ(emissions, 1);
  EXPECT_EQ(target.Get().FixedConfigurations().size(),
            source.FixedConfigurations().size());
}

TEST(ShapeConfigurationStoreTest, ReentryGuardBlocksRecursiveReceive) {
  ShapeConfigSet secondary;
  domain::ShapeConfigSetTopic topic;
  ShapeConfigurationStore primary(topic);
  int emissions = 0;
  QObject::connect(&topic, &domain::ShapeConfigSetTopic::Published,
                   [&](const ShapeConfigSet&) {
                     ++emissions;
                     if (emissions == 1) {
                       primary.ReceiveShapeConfigSet(secondary);
                     }
                   });

  primary.ReceiveShapeConfigSet(secondary);

  EXPECT_EQ(emissions, 1);
}

// No colour comes out of the index any more: every new category starts in the
// same light pastel blue until the user picks one.
TEST(ShapeConfigSetTest, EveryNewCategoryStartsInTheSamePastelBlue) {
  ShapeConfigSet set;

  set.SyncToDateCategories(3);

  const glm::vec4 first = set.GetDynamicConfiguration(0).FillColorDisabled();
  EXPECT_EQ(set.GetDynamicConfiguration(1).FillColorDisabled(), first);
  EXPECT_EQ(set.GetDynamicConfiguration(2).FillColorDisabled(), first);
  EXPECT_GT(first[2], first[0]);  // blue outweighs red
}

TEST(ShapeConfigSetTest, SetCategoryColorColoursOutlineAndFillOfThatCategory) {
  ShapeConfigSet set;
  set.SyncToDateCategories(2);
  const glm::vec3 teal{0.0F, 0.5F, 0.5F};

  ASSERT_TRUE(set.SetCategoryColor(1, teal));

  const ShapeConfiguration colored = set.GetDynamicConfiguration(1);
  EXPECT_EQ(glm::vec3(colored.OutlineColorDisabled()), teal);
  EXPECT_EQ(glm::vec3(colored.FillColorDisabled()), teal);
  EXPECT_GT(colored.OutlineColorDisabled()[3], colored.FillColorDisabled()[3]);
  EXPECT_NE(glm::vec3(set.GetDynamicConfiguration(0).FillColorDisabled()),
            teal);
}

TEST(ShapeConfigSetTest, SetCategoryColorRefusesAnUnknownCategory) {
  ShapeConfigSet set;
  set.SyncToDateCategories(1);

  EXPECT_FALSE(set.SetCategoryColor(1, glm::vec3{1.0F, 0.0F, 0.0F}));
}

TEST(ShapeConfigurationTest, SetColorKeepsEachOpacity) {
  ShapeConfiguration shape(
      "k", true, true, 1.0F,
      ShapeConfiguration::OutlineColorValue{glm::vec4{0.0F, 0.0F, 0.0F, 0.8F}},
      ShapeConfiguration::FillColorValue{glm::vec4{1.0F, 1.0F, 1.0F, 0.3F}});

  shape.SetColor(glm::vec3{0.2F, 0.4F, 0.6F});

  EXPECT_EQ(shape.OutlineColorDisabled(), (glm::vec4{0.2F, 0.4F, 0.6F, 0.8F}));
  EXPECT_EQ(shape.FillColorDisabled(), (glm::vec4{0.2F, 0.4F, 0.6F, 0.3F}));
}
