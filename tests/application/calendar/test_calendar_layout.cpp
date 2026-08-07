#include <gtest/gtest.h>

#include <vector>

#include "application/calendar/calendar_layout.hpp"
#include "infrastructure/graphics/rect.hpp"

namespace {

// A4-ish page with deliberately asymmetric margins (l/r/b/t all different) so a
// swapped axis would be caught, plus a 3-year span and a 7-entry proportion set
// (gap/sub/gap/sub/gap/sub/gap -> 3 sub-frames per row).
CalendarLayout MakeLayout() {
  const RectF page =
      RectF::FromDimension(RectF::Dimension{.width = 200.0F, .height = 300.0F});
  const RectF margin(10.0F, 20.0F, 30.0F, 40.0F);
  const std::vector<float> proportions(7, 1.0F);
  return CalendarLayout(page, margin, /*title_frame_height=*/15.0F,
                        /*span_length_years=*/3, proportions);
}

constexpr float kTol = 1.0e-3F;

TEST(CalendarLayoutTest, PrintAreaIsPageMinusMarginsShiftedToOrigin) {
  const CalendarLayout layout = MakeLayout();

  // Origin = bottom-left of the un-shifted print area (page reduced by
  // margins).
  EXPECT_NEAR(layout.PrintAreaOrigin().x, -90.0F, kTol);
  EXPECT_NEAR(layout.PrintAreaOrigin().y, -120.0F, kTol);

  // After the shift the print area sits at the local origin.
  EXPECT_NEAR(layout.PrintArea().Left(), 0.0F, kTol);
  EXPECT_NEAR(layout.PrintArea().Bottom(), 0.0F, kTol);
  EXPECT_NEAR(layout.PrintArea().Width(), 170.0F, kTol);   // 200 - 10 - 20
  EXPECT_NEAR(layout.PrintArea().Height(), 230.0F, kTol);  // 300 - 30 - 40
}

TEST(CalendarLayoutTest, TitleFrameSitsAtTopWithGivenHeight) {
  const CalendarLayout layout = MakeLayout();

  EXPECT_NEAR(layout.TitleFrame().Top(), layout.PrintArea().Top(), kTol);
  EXPECT_NEAR(layout.TitleFrame().Height(), 15.0F, kTol);
  EXPECT_NEAR(layout.TitleFrame().Left(), layout.PrintArea().Left(), kTol);
  EXPECT_NEAR(layout.TitleFrame().Right(), layout.PrintArea().Right(), kTol);
}

TEST(CalendarLayoutTest, CalendarFrameIsBelowTitleAndRightMargined) {
  const CalendarLayout layout = MakeLayout();

  EXPECT_NEAR(layout.CalendarFrame().Top(), layout.TitleFrame().Bottom(), kTol);
  EXPECT_NEAR(layout.CalendarFrame().Bottom(), 0.0F, kTol);
  // Right edge reduced by the default 5pt calendar margin.
  EXPECT_NEAR(layout.CalendarFrame().Width(), 165.0F, kTol);  // 170 - 5
}

TEST(CalendarLayoutTest, CellAndRowAndDayMetrics) {
  const CalendarLayout layout = MakeLayout();

  // 13 columns across the calendar frame.
  EXPECT_NEAR(layout.CellWidth(), 165.0F / 13.0F, kTol);
  // (2 header rows + 3 span years) divide the height.
  EXPECT_NEAR(layout.RowHeight(), 215.0F / 5.0F, kTol);
  // Day width is the cells-frame width spread over a 366-day year.
  EXPECT_NEAR(layout.DayWidth(), (165.0F - (165.0F / 13.0F)) / 366.0F, kTol);
}

TEST(CalendarLayoutTest, SubFrameAlignsHorizontallyWithCellsFrame) {
  const CalendarLayout layout = MakeLayout();

  const RectF sub = layout.GetSubFrame(0, 1);
  EXPECT_NEAR(sub.Left(), layout.CellsFrame().Left(), kTol);
  EXPECT_NEAR(sub.Right(), layout.CellsFrame().Right(), kTol);
  // The sub-frame lies within the cells frame vertically.
  EXPECT_GE(sub.Bottom(), layout.CellsFrame().Bottom() - kTol);
  EXPECT_LE(sub.Top(), layout.CellsFrame().Top() + kTol);
}

TEST(CalendarLayoutTest, DefaultConstructedIsEmpty) {
  const CalendarLayout layout;
  EXPECT_NEAR(layout.PrintArea().Width(), 0.0F, kTol);
  EXPECT_NEAR(layout.CellWidth(), 0.0F, kTol);
}

}  // namespace
