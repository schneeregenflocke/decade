#include <gtest/gtest.h>

#include <vector>

#include "app/binding/calendar_layout.hpp"
#include "graphics/rect.hpp"

namespace {

// A4-ish page with deliberately asymmetric margins (l/r/b/t all different) so a
// swapped axis would be caught, plus a 3-year span and a 7-entry proportion set
// (gap/sub/gap/sub/gap/sub/gap -> 3 sub-frames per row).
CalendarLayout MakeLayout() {
  const rectf page = rectf::from_dimension(
      rectf::Dimension{.width = 200.0F, .height = 300.0F});
  const rectf margin(10.0F, 20.0F, 30.0F, 40.0F);
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
  EXPECT_NEAR(layout.PrintArea().l(), 0.0F, kTol);
  EXPECT_NEAR(layout.PrintArea().b(), 0.0F, kTol);
  EXPECT_NEAR(layout.PrintArea().width(), 170.0F, kTol);   // 200 - 10 - 20
  EXPECT_NEAR(layout.PrintArea().height(), 230.0F, kTol);  // 300 - 30 - 40
}

TEST(CalendarLayoutTest, TitleFrameSitsAtTopWithGivenHeight) {
  const CalendarLayout layout = MakeLayout();

  EXPECT_NEAR(layout.TitleFrame().t(), layout.PrintArea().t(), kTol);
  EXPECT_NEAR(layout.TitleFrame().height(), 15.0F, kTol);
  EXPECT_NEAR(layout.TitleFrame().l(), layout.PrintArea().l(), kTol);
  EXPECT_NEAR(layout.TitleFrame().r(), layout.PrintArea().r(), kTol);
}

TEST(CalendarLayoutTest, CalendarFrameIsBelowTitleAndRightMargined) {
  const CalendarLayout layout = MakeLayout();

  EXPECT_NEAR(layout.CalendarFrame().t(), layout.TitleFrame().b(), kTol);
  EXPECT_NEAR(layout.CalendarFrame().b(), 0.0F, kTol);
  // Right edge reduced by the default 5pt calendar margin.
  EXPECT_NEAR(layout.CalendarFrame().width(), 165.0F, kTol);  // 170 - 5
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

  const rectf sub = layout.GetSubFrame(0, 1);
  EXPECT_NEAR(sub.l(), layout.CellsFrame().l(), kTol);
  EXPECT_NEAR(sub.r(), layout.CellsFrame().r(), kTol);
  // The sub-frame lies within the cells frame vertically.
  EXPECT_GE(sub.b(), layout.CellsFrame().b() - kTol);
  EXPECT_LE(sub.t(), layout.CellsFrame().t() + kTol);
}

TEST(CalendarLayoutTest, DefaultConstructedIsEmpty) {
  const CalendarLayout layout;
  EXPECT_NEAR(layout.PrintArea().width(), 0.0F, kTol);
  EXPECT_NEAR(layout.CellWidth(), 0.0F, kTol);
}

}  // namespace
