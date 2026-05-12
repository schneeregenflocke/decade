#include <gtest/gtest.h>

#include <glm/vec4.hpp>
#include <string>

#include "packages/shape_config.hpp"

TEST(ShapeConfigurationStorageTest, DefaultsContainExpectedNames) {
  ShapeConfigurationStorage storage;
  ASSERT_GT(storage.size(), 0U);
  EXPECT_EQ(storage.GetNumberPersistentConfigurations(), storage.size());

  // A handful of named entries we expect from the default set.
  const ShapeConfiguration page_margin =
      storage.GetShapeConfiguration("Page Margin");
  EXPECT_EQ(page_margin.Name(), "Page Margin");
  EXPECT_TRUE(page_margin.OutlineVisible());

  const ShapeConfiguration day_shapes =
      storage.GetShapeConfiguration("Day Shapes");
  EXPECT_EQ(day_shapes.Name(), "Day Shapes");
  EXPECT_TRUE(day_shapes.FillVisible());
}

TEST(ShapeConfigurationStorageTest,
     GetShapeConfigurationReturnsBlankForUnknownName) {
  ShapeConfigurationStorage storage;
  const ShapeConfiguration unknown =
      storage.GetShapeConfiguration("Does Not Exist");
  EXPECT_TRUE(unknown.Name().empty());
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

TEST(ShapeConfigurationStorageTest, ReceiveEmitsAndCopiesContents) {
  ShapeConfigurationStorage source;
  ShapeConfigurationStorage target;

  int emissions = 0;
  target.SignalShapeConfigurationStorage().connect(
      [&](const ShapeConfigurationStorage&) { ++emissions; });

  target.ReceiveShapeConfigurationStorage(source);

  EXPECT_EQ(emissions, 1);
  EXPECT_EQ(target.size(), source.size());
}

TEST(ShapeConfigurationStorageTest, ReentryGuardBlocksRecursiveReceive) {
  ShapeConfigurationStorage primary;
  ShapeConfigurationStorage secondary;
  int emissions = 0;
  primary.SignalShapeConfigurationStorage().connect(
      [&](const ShapeConfigurationStorage&) {
        ++emissions;
        if (emissions == 1) {
          primary.ReceiveShapeConfigurationStorage(secondary);
        }
      });

  primary.ReceiveShapeConfigurationStorage(secondary);

  EXPECT_EQ(emissions, 1);
}
