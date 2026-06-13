#include <gtest/gtest.h>

#include "packages/date.hpp"
#include "packages/date_period.hpp"

namespace {

DatePeriod MakePeriod(int year, int month_begin, int day_begin, int month_end,
                      int day_end) {
  return {Date::FromYmd(year, month_begin, day_begin),
          Date::FromYmd(year, month_end, day_end)};
}

}  // namespace

TEST(DatePeriodTest, DefaultConstructedIsNullAndInvalid) {
  const DatePeriod period;
  EXPECT_FALSE(period.HasValidDates());
  EXPECT_TRUE(period.IsNull());
  EXPECT_EQ(period.LengthDays(), 0);
}

TEST(DatePeriodTest, LengthIsHalfOpen) {
  const DatePeriod period = MakePeriod(2030, 6, 10, 6, 20);
  EXPECT_EQ(period.LengthDays(), 10);
}

TEST(DatePeriodTest, LastIsDayBeforeEnd) {
  const DatePeriod period = MakePeriod(2030, 6, 10, 6, 20);
  EXPECT_EQ(period.Last(), Date::FromYmd(2030, 6, 19));
}

TEST(DatePeriodTest, LastCrossesYearBoundaryBackwards) {
  const DatePeriod period(Date::FromYmd(2030, 1, 1), Date::FromYmd(2030, 1, 1));
  EXPECT_EQ(period.Last(), Date::FromYmd(2029, 12, 31));
}

TEST(DatePeriodTest, SingleDatePeriodIsNull) {
  const DatePeriod period = MakePeriod(2030, 6, 10, 6, 10);
  EXPECT_TRUE(period.HasValidDates());
  EXPECT_TRUE(period.IsNull());
  EXPECT_EQ(period.LengthDays(), 0);
}

TEST(DatePeriodTest, ReversedPeriodIsNull) {
  const DatePeriod period = MakePeriod(2030, 6, 20, 6, 10);
  EXPECT_TRUE(period.IsNull());
}

TEST(PeriodFromInclusiveDatesTest, BuildsHalfOpenFromInclusiveTo) {
  const DatePeriod period = PeriodFromInclusiveDates(
      Date::FromYmd(2030, 6, 10), Date::FromYmd(2030, 6, 20));
  EXPECT_EQ(period.Begin(), Date::FromYmd(2030, 6, 10));
  EXPECT_EQ(period.End(), Date::FromYmd(2030, 6, 21));
  EXPECT_EQ(period.Last(), Date::FromYmd(2030, 6, 20));
  EXPECT_EQ(period.LengthDays(), 11);  // both endpoints count
}

TEST(PeriodFromInclusiveDatesTest, EqualDatesYieldSingleDayPeriod) {
  const DatePeriod period = PeriodFromInclusiveDates(Date::FromYmd(2022, 1, 1),
                                                     Date::FromYmd(2022, 1, 1));
  EXPECT_EQ(period.LengthDays(), 1);
  EXPECT_FALSE(period.IsNull());
  EXPECT_EQ(period.Last(), Date::FromYmd(2022, 1, 1));
}

TEST(PeriodFromInclusiveDatesTest, MissingToDateYieldsSingleDayPeriod) {
  const DatePeriod period =
      PeriodFromInclusiveDates(Date::FromYmd(2030, 6, 10), Date());
  EXPECT_EQ(period.Begin(), Date::FromYmd(2030, 6, 10));
  EXPECT_EQ(period.LengthDays(), 1);
}

TEST(PeriodFromInclusiveDatesTest, UnusableInputYieldsNullPeriod) {
  // to before from
  EXPECT_TRUE(PeriodFromInclusiveDates(Date::FromYmd(2030, 6, 20),
                                       Date::FromYmd(2030, 6, 10))
                  .IsNull());
  // from missing
  EXPECT_TRUE(
      PeriodFromInclusiveDates(Date(), Date::FromYmd(2030, 6, 10)).IsNull());
  EXPECT_TRUE(PeriodFromInclusiveDates(Date(), Date()).IsNull());
}
