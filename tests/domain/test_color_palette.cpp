#include <gtest/gtest.h>

#include <cstddef>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>

#include "domain/color_palette.hpp"

namespace {

constexpr std::size_t kEnoughGroups = 64;

bool InsideDisplayRange(const glm::vec3& color) {
  for (glm::vec3::length_type channel = 0; channel < 3; ++channel) {
    if (color[channel] < 0.0F || color[channel] > 1.0F) {
      return false;
    }
  }
  return true;
}

}  // namespace

// OkHSL states saturation relative to the gamut, so a hue rotation must never
// leave a channel needing to be clipped — that is the whole reason the palette
// rotates hue instead of holding chroma fixed.
TEST(ColorPaletteTest, EveryColorFitsInsideTheDisplayableRange) {
  for (std::size_t index = 0; index < kEnoughGroups; ++index) {
    EXPECT_TRUE(InsideDisplayRange(palette::CategoricalColor(index))) << index;
  }
}

TEST(ColorPaletteTest, AnIndexKeepsItsColorAsGroupsAreAdded) {
  const glm::vec3 third = palette::CategoricalColor(2);

  EXPECT_EQ(palette::CategoricalColor(2), third);
  EXPECT_NE(palette::CategoricalColor(3), third);
}

// Neighbouring indices are the pair a reader compares, and the golden-angle
// step exists to keep them apart.
TEST(ColorPaletteTest, NeighbouringIndicesDifferVisibly) {
  constexpr float kLeastNoticeableDistance = 0.2F;

  for (std::size_t index = 0; index + 1 < kEnoughGroups; ++index) {
    const glm::vec3 current = palette::CategoricalColor(index);
    const glm::vec3 next = palette::CategoricalColor(index + 1);
    const glm::vec3 difference = next - current;
    const float distance = glm::length(difference);

    EXPECT_GT(distance, kLeastNoticeableDistance) << index;
  }
}
