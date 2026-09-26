#include <gtest/gtest.h>

#include <cstddef>

#include "domain/calendar_config.hpp"
#include "domain/date.hpp"
#include "domain/date_period.hpp"
#include "domain/timeline_projection.hpp"

namespace {

DatePeriod HalfOpen(int y0, int m0, int d0, int y1, int m1, int d1) {
  return {Date::FromYmd(y0, m0, d0), Date::FromYmd(y1, m1, d1)};
}

}  // namespace

TEST(SplitAtYearBoundariesTest, WithinSingleYearReturnsUnchanged) {
  const auto period = HalfOpen(2020, 3, 1, 2020, 6, 1);
  const auto segments = SplitAtYearBoundaries(period);

  ASSERT_EQ(segments.size(), 1U);
  EXPECT_EQ(segments.front(), period);
}

TEST(SplitAtYearBoundariesTest, SingleDayReturnsUnchanged) {
  const auto period = HalfOpen(2020, 3, 1, 2020, 3, 2);
  const auto segments = SplitAtYearBoundaries(period);

  ASSERT_EQ(segments.size(), 1U);
  EXPECT_EQ(segments.front(), period);
}

TEST(SplitAtYearBoundariesTest, CrossesOneBoundary) {
  const auto period = HalfOpen(2020, 6, 1, 2021, 3, 1);
  const auto segments = SplitAtYearBoundaries(period);

  ASSERT_EQ(segments.size(), 2U);
  EXPECT_EQ(segments[0], HalfOpen(2020, 6, 1, 2021, 1, 1));
  EXPECT_EQ(segments[1], HalfOpen(2021, 1, 1, 2021, 3, 1));
}

TEST(SplitAtYearBoundariesTest, CrossesTwoBoundaries) {
  const auto period = HalfOpen(2019, 6, 1, 2021, 3, 1);
  const auto segments = SplitAtYearBoundaries(period);

  ASSERT_EQ(segments.size(), 3U);
  EXPECT_EQ(segments[0], HalfOpen(2019, 6, 1, 2020, 1, 1));
  EXPECT_EQ(segments[1], HalfOpen(2020, 1, 1, 2021, 1, 1));
  EXPECT_EQ(segments[2], HalfOpen(2021, 1, 1, 2021, 3, 1));
}

TEST(SplitAtYearBoundariesTest, SegmentsAreContiguousAndCoverThePeriod) {
  const auto period = HalfOpen(2018, 9, 15, 2022, 2, 10);
  const auto segments = SplitAtYearBoundaries(period);

  ASSERT_FALSE(segments.empty());
  EXPECT_EQ(segments.front().Begin(), period.Begin());
  EXPECT_EQ(segments.back().End(), period.End());
  for (std::size_t i = 0; i + 1 < segments.size(); ++i) {
    EXPECT_EQ(segments[i].End(), segments[i + 1].Begin());
  }
}

TEST(TimelineProjectionTest, RowCountMatchesSpanYears) {
  CalendarSpan span;
  span.SetYears({.first_year = 2020, .last_year = 2025});
  const TimelineProjection projection(span);

  EXPECT_EQ(projection.RowCount(), span.YearCount());
  EXPECT_EQ(projection.RowCount(), 6U);
}

TEST(TimelineProjectionTest, RowPeriodIsOneYearAscendingFromFirstYear) {
  CalendarSpan span;
  span.SetYears({.first_year = 2030, .last_year = 2032});
  const TimelineProjection projection(span);

  EXPECT_EQ(projection.RowPeriod(0), HalfOpen(2030, 1, 1, 2031, 1, 1));
  EXPECT_EQ(projection.RowPeriod(2), HalfOpen(2032, 1, 1, 2033, 1, 1));
}

TEST(TimelineProjectionTest, RowOfInvertsRowPeriod) {
  CalendarSpan span;
  span.SetYears({.first_year = 2030, .last_year = 2034});
  const TimelineProjection projection(span);

  for (std::size_t row = 0; row < projection.RowCount(); ++row) {
    EXPECT_EQ(projection.RowOf(projection.RowPeriod(row).Begin()), row);
    EXPECT_EQ(projection.RowOf(projection.RowPeriod(row).Last()), row);
  }
}

TEST(TimelineProjectionTest, SplitAtRowBoundariesCutsAtEachRowEnd) {
  CalendarSpan span;
  span.SetYears({.first_year = 2030, .last_year = 2034});
  const TimelineProjection projection(span);

  const auto pieces =
      projection.SplitAtRowBoundaries(HalfOpen(2030, 12, 20, 2032, 1, 10));

  ASSERT_EQ(pieces.size(), 3U);
  EXPECT_EQ(pieces[0], HalfOpen(2030, 12, 20, 2031, 1, 1));
  EXPECT_EQ(pieces[1], HalfOpen(2031, 1, 1, 2032, 1, 1));
  EXPECT_EQ(pieces[2], HalfOpen(2032, 1, 1, 2032, 1, 10));
}

TEST(TimelineProjectionTest, SplitAtRowBoundariesClipsToTheSpan) {
  CalendarSpan span;
  span.SetYears({.first_year = 2030, .last_year = 2030});
  const TimelineProjection projection(span);

  const auto pieces =
      projection.SplitAtRowBoundaries(HalfOpen(2029, 12, 20, 2031, 1, 10));

  ASSERT_EQ(pieces.size(), 1U);
  EXPECT_EQ(pieces[0], projection.RowPeriod(0));
}

TEST(TimelineProjectionTest, SplitAtRowBoundariesDropsAPeriodOutsideTheSpan) {
  CalendarSpan span;
  span.SetYears({.first_year = 2030, .last_year = 2030});
  const TimelineProjection projection(span);

  EXPECT_TRUE(projection.SplitAtRowBoundaries(HalfOpen(2031, 1, 1, 2031, 2, 1))
                  .empty());
}
