#include <gtest/gtest.h>

#include "application/calendar/calendar_layout.hpp"
#include "domain/band_part.hpp"
#include "domain/calendar_config.hpp"
#include "domain/calendar_sizing.hpp"
#include "domain/calendar_view.hpp"
#include "infrastructure/graphics/rect.hpp"

namespace {

constexpr BandProportions kEqualParts = {1.0F, 1.0F, 1.0F, 1.0F,
                                         1.0F, 1.0F, 1.0F};

// A4-ish page with deliberately asymmetric margins (l/r/b/t all different) so a
// swapped axis would be caught, plus a 3-year span and equal band parts.
CalendarLayout MakeLayout() {
  const RectF page =
      RectF::FromDimension(RectF::Dimension{.width = 200.0F, .height = 300.0F});
  const RectF margin(10.0F, 20.0F, 30.0F, 40.0F);
  CalendarConfig config;
  config.SetYears({.first_year = 2001, .last_year = 2003});
  config.SetBandProportions(kEqualParts);
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

TEST(CalendarLayoutTest, PartAreaAlignsHorizontallyWithCellsFrame) {
  const CalendarLayout layout = MakeLayout();

  const RectF days = layout.GetPartArea(0, BandPart::kDays);
  EXPECT_NEAR(days.Left(), layout.CellsArea().Left(), kTol);
  EXPECT_NEAR(days.Right(), layout.CellsArea().Right(), kTol);
  // The part lies within the cells frame vertically.
  EXPECT_GE(days.Bottom(), layout.CellsArea().Bottom() - kTol);
  EXPECT_LE(days.Top(), layout.CellsArea().Top() + kTol);
}

// The single row takes every year's band; the labels and the legend keep
// theirs, and so does what a band's part sizes.
TEST(CalendarLayoutTest, OneRowForAllYearsKeepsTheBandsOfAYearPerRow) {
  const CalendarLayout per_year = MakeLayout();
  const RectF page =
      RectF::FromDimension(RectF::Dimension{.width = 200.0F, .height = 300.0F});
  CalendarConfig config;
  config.SetYears({.first_year = 2001, .last_year = 2003});
  config.SetBandProportions(kEqualParts);
  config.SetView(CalendarView::kAllYearsInOneRow);
  const CalendarLayout one_row(page, RectF(10.0F, 20.0F, 30.0F, 40.0F),
                               /*title_area_height=*/15.0F, config);

  EXPECT_NEAR(one_row.GetRowArea(0).Height(), one_row.CellsArea().Height(),
              kTol);
  EXPECT_NEAR(one_row.XLabelsArea().Height(), per_year.XLabelsArea().Height(),
              kTol);
  EXPECT_NEAR(one_row.LegendArea().Height(), per_year.LegendArea().Height(),
              kTol);
  EXPECT_NEAR(one_row.BandPartHeight(BandPart::kDays),
              per_year.GetPartArea(0, BandPart::kDays).Height(), kTol);
  // 2001 to 2003 hold 1095 days, and the row's width stands for all of them.
  EXPECT_NEAR(one_row.DayWidth(), one_row.CellsArea().Width() / 1095.0F, kTol);
}

TEST(CalendarLayoutTest, FittedLegendSharesItsAreaOutAmongTheEntries) {
  const CalendarLayout layout = MakeLayout();

  EXPECT_NEAR(layout.LegendEntryWidth(3), layout.LegendArea().Width() / 3.0F,
              kTol);
}

CalendarLayout MakeFixedLayout(const CalendarSizing& sizing) {
  const RectF page =
      RectF::FromDimension(RectF::Dimension{.width = 200.0F, .height = 300.0F});
  CalendarConfig config;
  config.SetYears({.first_year = 2001, .last_year = 2003});
  config.SetBandProportions(kEqualParts);
  config.SetSizing(sizing);
  return CalendarLayout(page, RectF(10.0F, 20.0F, 30.0F, 40.0F),
                        /*title_area_height=*/15.0F, config);
}

// A day of 1 mm makes a year row 366 mm wide on a print area of 170 mm: the
// calendar starts at the left edge and runs past the right one.
TEST(CalendarLayoutTest, FixedWidthTakesTheMillimetres) {
  CalendarSizing sizing;
  sizing.SetFixesWidth(true);
  sizing.SetFixedWidths(
      {.day = 1.0F, .row_labels = 12.0F, .legend_entry = 30.0F});
  const CalendarLayout layout = MakeFixedLayout(sizing);

  EXPECT_NEAR(layout.DayWidth(), 1.0F, kTol);
  EXPECT_NEAR(layout.YLabelsArea().Width(), 12.0F, kTol);
  EXPECT_NEAR(layout.CellsArea().Width(), 366.0F, kTol);
  EXPECT_NEAR(layout.CalendarArea().Left(), layout.PrintArea().Left(), kTol);
  EXPECT_NEAR(layout.CalendarArea().Right(), 378.0F, kTol);
  EXPECT_NEAR(layout.LegendEntryWidth(3), 30.0F, kTol);
  // The height still fits the page.
  EXPECT_NEAR(layout.GetRowArea(0).Height(), 215.0F / 5.0F, kTol);
}

TEST(CalendarLayoutTest, FixedHeightTakesTheMillimetresBelowTheTitle) {
  CalendarSizing sizing;
  sizing.SetFixesHeight(true);
  sizing.SetFixedHeights({.band = 4.0F, .column_labels = 5.0F, .legend = 7.0F});
  const CalendarLayout layout = MakeFixedLayout(sizing);

  EXPECT_NEAR(layout.GetRowArea(0).Height(), 4.0F, kTol);
  EXPECT_NEAR(layout.XLabelsArea().Height(), 5.0F, kTol);
  EXPECT_NEAR(layout.LegendArea().Height(), 7.0F, kTol);
  EXPECT_NEAR(layout.CalendarArea().Top(), layout.TitleArea().Bottom(), kTol);
  EXPECT_NEAR(layout.CalendarArea().Height(), (3.0F * 4.0F) + 5.0F + 7.0F,
              kTol);
  EXPECT_NEAR(layout.BandPartHeight(BandPart::kDays), 4.0F / 7.0F, kTol);
  // The width still fits the page.
  EXPECT_NEAR(layout.CalendarArea().Width(), 165.0F, kTol);
}

TEST(CalendarLayoutTest, DefaultConstructedIsEmpty) {
  const CalendarLayout layout;
  EXPECT_NEAR(layout.PrintArea().Width(), 0.0F, kTol);
  EXPECT_NEAR(layout.DayWidth(), 0.0F, kTol);
}

}  // namespace
