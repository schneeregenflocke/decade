#include <gtest/gtest.h>

#include "infrastructure/graphics/rect.hpp"

TEST(RectTest, EnclosingHoldsBothRectangles) {
  const RectF page(-10.0F, 10.0F, -5.0F, 5.0F);
  const RectF overflow(-8.0F, 30.0F, -20.0F, 4.0F);

  const RectF enclosing = page.Enclosing(overflow);

  EXPECT_FLOAT_EQ(enclosing.Left(), -10.0F);
  EXPECT_FLOAT_EQ(enclosing.Right(), 30.0F);
  EXPECT_FLOAT_EQ(enclosing.Bottom(), -20.0F);
  EXPECT_FLOAT_EQ(enclosing.Top(), 5.0F);
}
