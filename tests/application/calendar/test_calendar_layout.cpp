#include <gtest/gtest.h>

#include <vector>

#include "application/calendar/calendar_layout.hpp"
#include "domain/calendar_config.hpp"
#include "domain/calendar_view.hpp"
#include "infrastructure/graphics/rect.hpp"

namespace {

// A4-ish page with deliberately asymmetric margins (l/r/b/t all different) so a
// swapped axis would be caught, plus a 3-year span and a 7-entry proportion set
// (gap/sub/gap/sub/gap/sub/gap -> 3 sub-frames per row).
CalendarLayout MakeLayout() {
  const RectF page =
      RectF::FromDimension(RectF::Dimension{.width = 200.0F, .height = 300.0F});
  const RectF margin(10.0F, 20.0F, 30.0F, 40.0F);
  CalendarConfig config;
  config.SetYears({.first_year = 2001, .last_year = 2003});
  config.SetSpacingProportions(std::vector<float>(7, 1.0F));
  return CalendarLayout(page, margin, /*title_area_height=*/15.0F, config);
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

  EXPECT_NEAR(layout.TitleArea().Top(), layout.PrintArea().Top(), kTol);
  EXPECT_NEAR(layout.TitleArea().Height(), 15.0F, kTol);
  EXPECT_NEAR(layout.TitleArea().Left(), layout.PrintArea().Left(), kTol);
  EXPECT_NEAR(layout.TitleArea().Right(), layout.PrintArea().Right(), kTol);
}

TEST(CalendarLayoutTest, CalendarFrameIsBelowTitleAndRightMargined) {
  const CalendarLayout layout = MakeLayout();

  EXPECT_NEAR(layout.CalendarArea().Top(), layout.TitleArea().Bottom(), kTol);
  EXPECT_NEAR(layout.CalendarArea().Bottom(), 0.0F, kTol);
  // Right edge reduced by the default 5pt calendar margin.
  EXPECT_NEAR(layout.CalendarArea().Width(), 165.0F, kTol);  // 170 - 5
}

TEST(CalendarLayoutTest, CellAndRowAndDayMetrics) {
  const CalendarLayout layout = MakeLayout();

  // The row labels take one of 13 columns across the calendar frame.
  EXPECT_NEAR(layout.YLabelsArea().Width(), 165.0F / 13.0F, kTol);
  // (2 header bands + 3 span years) divide the height.
  EXPECT_NEAR(layout.GetRowArea(0).Height(), 215.0F / 5.0F, kTol);
  EXPECT_NEAR(layout.XLabelsArea().Height(), 215.0F / 5.0F, kTol);
  // Day width is the cells-frame width spread over a 366-day year.
  EXPECT_NEAR(layout.DayWidth(), (165.0F - (165.0F / 13.0F)) / 366.0F, kTol);
}

TEST(CalendarLayoutTest, SubFrameAlignsHorizontallyWithCellsFrame) {
  const CalendarLayout layout = MakeLayout();

  const RectF sub = layout.GetSubArea(0, 1);
  EXPECT_NEAR(sub.Left(), layout.CellsArea().Left(), kTol);
  EXPECT_NEAR(sub.Right(), layout.CellsArea().Right(), kTol);
  // The sub-frame lies within the cells frame vertically.
  EXPECT_GE(sub.Bottom(), layout.CellsArea().Bottom() - kTol);
  EXPECT_LE(sub.Top(), layout.CellsArea().Top() + kTol);
}

// The single row takes every year's band; the labels and the legend keep
// theirs, and so does what a band's subrow sizes.
TEST(CalendarLayoutTest, OneRowForAllYearsKeepsTheBandsOfAYearPerRow) {
  const CalendarLayout per_year = MakeLayout();
  const RectF page =
      RectF::FromDimension(RectF::Dimension{.width = 200.0F, .height = 300.0F});
  CalendarConfig config;
  config.SetYears({.first_year = 2001, .last_year = 2003});
  config.SetSpacingProportions(std::vector<float>(7, 1.0F));
  config.SetView(CalendarView::kAllYearsInOneRow);
  const CalendarLayout one_row(page, RectF(10.0F, 20.0F, 30.0F, 40.0F),
                               /*title_area_height=*/15.0F, config);

  EXPECT_NEAR(one_row.GetRowArea(0).Height(), one_row.CellsArea().Height(),
              kTol);
  EXPECT_NEAR(one_row.XLabelsArea().Height(), per_year.XLabelsArea().Height(),
              kTol);
  EXPECT_NEAR(one_row.LegendArea().Height(), per_year.LegendArea().Height(),
              kTol);
  EXPECT_NEAR(one_row.BandSubHeight(1), per_year.GetSubArea(0, 1).Height(),
              kTol);
  // 2001 to 2003 hold 1095 days, and the row's width stands for all of them.
  EXPECT_NEAR(one_row.DayWidth(), one_row.CellsArea().Width() / 1095.0F, kTol);
}

TEST(CalendarLayoutTest, DefaultConstructedIsEmpty) {
  const CalendarLayout layout;
  EXPECT_NEAR(layout.PrintArea().Width(), 0.0F, kTol);
  EXPECT_NEAR(layout.DayWidth(), 0.0F, kTol);
}

}  // namespace
