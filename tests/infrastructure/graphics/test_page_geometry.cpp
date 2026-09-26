#include <gtest/gtest.h>

#include "domain/page_setup_config.hpp"
#include "infrastructure/graphics/page_geometry.hpp"
#include "infrastructure/graphics/rect.hpp"

// The page panel stores the margins as {left, bottom, right, top}; four
// different values catch any two that trade places.
TEST(PageGeometryTest, MarginRectTakesEachMarginFromItsEdge) {
  PageSetupConfig config;
  config.SetMargins({10.0F, 20.0F, 30.0F, 40.0F});

  const RectF margins = PageMarginRect(config);

  EXPECT_FLOAT_EQ(margins.Left(), 10.0F);
  EXPECT_FLOAT_EQ(margins.Bottom(), 20.0F);
  EXPECT_FLOAT_EQ(margins.Right(), 30.0F);
  EXPECT_FLOAT_EQ(margins.Top(), 40.0F);
}
