#include <gtest/gtest.h>

#include "infrastructure/graphics/projection.hpp"

// Regression: a degenerate viewport must carry no inf or NaN into the matrices.
// A height of 0 arises from a window pulled extremely flat — the splitter
// protects the width alone.
TEST(ProjectionTest, DegenerateViewportYieldsFiniteAspectRatio) {
  EXPECT_FLOAT_EQ(Projection::AspectRatioOf(1600, 0), 1.0F);
  EXPECT_FLOAT_EQ(Projection::AspectRatioOf(0, 900), 1.0F);
  EXPECT_FLOAT_EQ(Projection::AspectRatioOf(0, 0), 1.0F);
  EXPECT_FLOAT_EQ(Projection::AspectRatioOf(-10, 900), 1.0F);
}

TEST(ProjectionTest, ValidViewportYieldsWidthOverHeight) {
  EXPECT_FLOAT_EQ(Projection::AspectRatioOf(1600, 800), 2.0F);
  EXPECT_FLOAT_EQ(Projection::AspectRatioOf(900, 1800), 0.5F);
}
