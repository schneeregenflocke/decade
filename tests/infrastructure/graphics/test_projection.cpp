#include <gtest/gtest.h>

#include "infrastructure/graphics/projection.hpp"

// Regression: ein entartetes Viewport darf kein inf/NaN in die Matrizen
// tragen. Höhe 0 entsteht durch ein extrem flach gezogenes Fenster — der
// Splitter schützt nur die Breite.
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
