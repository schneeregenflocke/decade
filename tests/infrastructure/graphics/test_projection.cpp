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

// The projection fits the page into the viewport without distorting it: the
// side that runs out first decides, and the other keeps the page's proportion.
// The aspect ratio comes in as a parameter — it used to be read back from GL,
// which tied this to a current context and, at a rebuild, to a stale viewport
// (#72).
TEST(ProjectionTest, AWideViewportFitsThePageByHeight) {
  const RectF page(-100.0F, 100.0F, -50.0F, 50.0F);  // 200 x 100, ratio 2
  const glm::mat4 ortho = Projection::OrthoMatrix(page, 4.0F);

  // Ortho maps [-x_half, x_half] to [-1, 1]; the matrix diagonal holds 1/x_half
  // and 1/y_half. A viewport wider than the page keeps the page's height and
  // widens the visible world.
  EXPECT_FLOAT_EQ(1.0F / ortho[1][1], 50.0F);
  EXPECT_FLOAT_EQ(1.0F / ortho[0][0], 200.0F);
}

TEST(ProjectionTest, ATallViewportFitsThePageByWidth) {
  const RectF page(-100.0F, 100.0F, -50.0F, 50.0F);
  const glm::mat4 ortho = Projection::OrthoMatrix(page, 1.0F);

  EXPECT_FLOAT_EQ(1.0F / ortho[0][0], 100.0F);
  EXPECT_FLOAT_EQ(1.0F / ortho[1][1], 100.0F);
}
