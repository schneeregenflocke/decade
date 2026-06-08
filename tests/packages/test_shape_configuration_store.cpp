#include <gtest/gtest.h>

#include <glm/vec4.hpp>
#include <string>

#include "packages/shape_configuration.hpp"
#include "packages/shape_configuration_store.hpp"

TEST(ShapeConfigSetTest, DefaultsContainExpectedNames) {
  ShapeConfigSet set;
  ASSERT_GT(set.size(), 0U);
  EXPECT_EQ(set.GetNumberPersistentConfigurations(), set.size());

  // A handful of named entries we expect from the default set.
  const ShapeConfiguration page_margin =
      set.GetShapeConfiguration("Page Margin");
  EXPECT_EQ(page_margin.Name(), "Page Margin");
  EXPECT_TRUE(page_margin.OutlineVisible());

  const ShapeConfiguration day_shapes = set.GetShapeConfiguration("Day Shapes");
  EXPECT_EQ(day_shapes.Name(), "Day Shapes");
  EXPECT_TRUE(day_shapes.FillVisible());
}

TEST(ShapeConfigSetTest, GetShapeConfigurationReturnsBlankForUnknownName) {
  ShapeConfigSet set;
  const ShapeConfiguration unknown =
      set.GetShapeConfiguration("Does Not Exist");
  EXPECT_TRUE(unknown.Name().empty());
}

TEST(ShapeConfigSetTest, DynamicConfigurationNameMatchesFormat) {
  EXPECT_EQ(ShapeConfigSet::DynamicConfigurationName(0), "Bar Group 0");
  EXPECT_EQ(ShapeConfigSet::DynamicConfigurationName(7), "Bar Group 7");
}

TEST(ShapeConfigSetTest, GetDynamicConfigurationAddressesByIndex) {
  ShapeConfigSet set;
  const size_t persistent = set.GetNumberPersistentConfigurations();

  // Append two dynamic configurations, mirroring how the panel grows the set
  // when date groups are added.
  set.resize(persistent + 2);
  set[persistent + 1] = ShapeConfiguration{
      ShapeConfigSet::DynamicConfigurationName(1),
      /*outline_visible=*/true,
      /*fill_visible=*/true,
      0.5F,
      ShapeConfiguration::OutlineColorValue{glm::vec4{0.1F, 0.2F, 0.3F, 1.0F}},
      ShapeConfiguration::FillColorValue{glm::vec4{0.1F, 0.2F, 0.3F, 0.5F}}};

  const ShapeConfiguration second = set.GetDynamicConfiguration(1);
  EXPECT_EQ(second.Name(), "Bar Group 1");
  EXPECT_FLOAT_EQ(second.OutlineColorDisabled()[0], 0.1F);

  // Out-of-range indices return a blank configuration instead of throwing.
  const ShapeConfiguration missing = set.GetDynamicConfiguration(99);
  EXPECT_TRUE(missing.Name().empty());
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
  ShapeConfigurationStore target;

  int emissions = 0;
  target.SignalShapeConfigSet().connect(
      [&](const ShapeConfigSet&) { ++emissions; });

  target.ReceiveShapeConfigSet(source);

  EXPECT_EQ(emissions, 1);
  EXPECT_EQ(target.GetShapeConfigSet().size(), source.size());
}

TEST(ShapeConfigurationStoreTest, ReentryGuardBlocksRecursiveReceive) {
  ShapeConfigSet secondary;
  ShapeConfigurationStore primary;
  int emissions = 0;
  primary.SignalShapeConfigSet().connect([&](const ShapeConfigSet&) {
    ++emissions;
    if (emissions == 1) {
      primary.ReceiveShapeConfigSet(secondary);
    }
  });

  primary.ReceiveShapeConfigSet(secondary);

  EXPECT_EQ(emissions, 1);
}
