#include <gtest/gtest.h>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "domain/calendar_config.hpp"
#include "domain/calendar_view.hpp"
#include "domain/date.hpp"
#include "domain/date_category.hpp"
#include "domain/date_entry.hpp"
#include "domain/date_period.hpp"
#include "infrastructure/persistence/value_serialization.hpp"

namespace {

template <typename Value>
Value XmlRoundTrip(const Value& value) {
  std::ostringstream out;
  {
    boost::archive::xml_oarchive oarchive(out);
    oarchive << boost::serialization::make_nvp("value", value);
  }
  Value loaded{};
  std::istringstream in(out.str());
  {
    boost::archive::xml_iarchive iarchive(in);
    iarchive >> boost::serialization::make_nvp("value", loaded);
  }
  return loaded;
}

}  // namespace

TEST(ValueSerializationTest, DateIsoStringRoundTrip) {
  const Date date = Date::FromYmd(1998, 9, 23);
  EXPECT_EQ(persistence::serialization_detail::DateToIsoString(date),
            "1998-09-23");
  EXPECT_EQ(persistence::serialization_detail::DateFromIsoString("1998-09-23"),
            date);
}

TEST(ValueSerializationTest, InvalidDateIsEmptyString) {
  EXPECT_EQ(persistence::serialization_detail::DateToIsoString(Date()), "");
  EXPECT_FALSE(
      persistence::serialization_detail::DateFromIsoString("").IsValid());
  EXPECT_FALSE(persistence::serialization_detail::DateFromIsoString("garbage")
                   .IsValid());
  EXPECT_FALSE(
      persistence::serialization_detail::DateFromIsoString("1998/09/23")
          .IsValid());
}

TEST(ValueSerializationTest, DateEntriesRoundTrip) {
  std::vector<DateEntry> entries(1);
  entries[0].SetDateInterval(
      DatePeriod(Date::FromYmd(1998, 9, 23), Date::FromYmd(1998, 11, 25)));
  entries[0].SetCategory(2);

  const auto loaded = XmlRoundTrip(entries);

  ASSERT_EQ(loaded.size(), 1U);
  EXPECT_EQ(loaded[0].GetDateInterval(), entries[0].GetDateInterval());
  EXPECT_EQ(loaded[0].GetCategory(), 2);
}

TEST(ValueSerializationTest, DateCategoriesRoundTrip) {
  std::vector<DateCategory> categories;
  categories.emplace_back("Gruppe äöü");
  categories.back().SetNumber(3);

  const auto loaded = XmlRoundTrip(categories);

  ASSERT_EQ(loaded.size(), 1U);
  EXPECT_EQ(loaded[0].GetName(), "Gruppe äöü");
  EXPECT_EQ(loaded[0].GetNumber(), 3);
}

TEST(ValueSerializationTest, CalendarConfigRoundTrip) {
  CalendarConfig config;
  config.SetYears({.first_year = 1998, .last_year = 2003});
  config.SetFitYearsToEntries(false);
  config.SetShowsAnnualCoverage(false);
  config.SetView(CalendarView::kAllYearsInOneRow);

  const auto loaded = XmlRoundTrip(config);

  EXPECT_EQ(loaded.FirstYear(), 1998);
  EXPECT_EQ(loaded.LastYear(), 2003);
  EXPECT_FALSE(loaded.IsFitYearsToEntries());
  EXPECT_FALSE(loaded.ShowsAnnualCoverage());
  EXPECT_EQ(loaded.View(), CalendarView::kAllYearsInOneRow);
}

// A project saved before the views existed carries class version 1 and no
// view element; it has to load with a year per row.
TEST(ValueSerializationTest, CalendarConfigVersionOneLaysOutAYearPerRow) {
  std::istringstream in(
      R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<!DOCTYPE boost_serialization>
<boost_serialization signature="serialization::archive" version="20">
<value class_id="0" tracking_level="0" version="1">
	<first_year>1998</first_year>
	<last_year>2003</last_year>
	<fit_years_to_entries>0</fit_years_to_entries>
	<spacing_proportions>
		<count>3</count>
		<item_version>0</item_version>
		<item>1</item>
		<item>2</item>
		<item>1</item>
	</spacing_proportions>
	<shows_annual_coverage>1</shows_annual_coverage>
</value>
</boost_serialization>
)");
  CalendarConfig loaded;
  loaded.SetView(CalendarView::kAllYearsInOneRow);
  {
    boost::archive::xml_iarchive iarchive(in);
    iarchive >> boost::serialization::make_nvp("value", loaded);
  }

  EXPECT_EQ(loaded.LastYear(), 2003);
  EXPECT_EQ(loaded.View(), CalendarView::kYearPerRow);
}

TEST(ValueSerializationTest, UnknownViewNumberFallsBackToAYearPerRow) {
  EXPECT_EQ(persistence::serialization_detail::CalendarViewFromNumber(1),
            CalendarView::kAllYearsInOneRow);
  EXPECT_EQ(persistence::serialization_detail::CalendarViewFromNumber(-1),
            CalendarView::kYearPerRow);
  EXPECT_EQ(persistence::serialization_detail::CalendarViewFromNumber(99),
            CalendarView::kYearPerRow);
}

// A project saved before the switch existed carries class version 0 and no
// shows_annual_coverage element; it has to load with the coverage shown.
TEST(ValueSerializationTest, CalendarConfigVersionZeroShowsAnnualCoverage) {
  std::istringstream in(
      R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<!DOCTYPE boost_serialization>
<boost_serialization signature="serialization::archive" version="20">
<value class_id="0" tracking_level="0" version="0">
	<first_year>1998</first_year>
	<last_year>2003</last_year>
	<fit_years_to_entries>0</fit_years_to_entries>
	<spacing_proportions>
		<count>3</count>
		<item_version>0</item_version>
		<item>1</item>
		<item>2</item>
		<item>1</item>
	</spacing_proportions>
</value>
</boost_serialization>
)");
  CalendarConfig loaded;
  loaded.SetShowsAnnualCoverage(false);
  {
    boost::archive::xml_iarchive iarchive(in);
    iarchive >> boost::serialization::make_nvp("value", loaded);
  }

  EXPECT_EQ(loaded.LastYear(), 2003);
  EXPECT_TRUE(loaded.ShowsAnnualCoverage());
}

// An earlier layout split a band into one part of content alone; its three
// proportions would leave the layout without the parts it draws into.
TEST(ValueSerializationTest, ProportionsOfAnEarlierLayoutKeepTheDefaults) {
  std::istringstream in(
      R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<!DOCTYPE boost_serialization>
<boost_serialization signature="serialization::archive" version="20">
<value class_id="0" tracking_level="0" version="2">
	<first_year>1998</first_year>
	<last_year>2003</last_year>
	<fit_years_to_entries>0</fit_years_to_entries>
	<spacing_proportions>
		<count>3</count>
		<item_version>0</item_version>
		<item>1</item>
		<item>2</item>
		<item>1</item>
	</spacing_proportions>
	<shows_annual_coverage>1</shows_annual_coverage>
	<view>0</view>
</value>
</boost_serialization>
)");
  CalendarConfig loaded;
  {
    boost::archive::xml_iarchive iarchive(in);
    iarchive >> boost::serialization::make_nvp("value", loaded);
  }

  EXPECT_EQ(loaded.GetBandProportions(), CalendarConfig().GetBandProportions());
}
