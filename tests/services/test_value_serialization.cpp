#include <gtest/gtest.h>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "app/services/value_serialization.hpp"
#include "packages/calendar_config.hpp"
#include "packages/date.hpp"
#include "packages/date_entry.hpp"
#include "packages/date_group.hpp"
#include "packages/date_period.hpp"

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
  EXPECT_EQ(app::serialization_detail::DateToIsoString(date), "1998-09-23");
  EXPECT_EQ(app::serialization_detail::DateFromIsoString("1998-09-23"), date);
}

TEST(ValueSerializationTest, InvalidDateIsEmptyString) {
  EXPECT_EQ(app::serialization_detail::DateToIsoString(Date()), "");
  EXPECT_FALSE(app::serialization_detail::DateFromIsoString("").IsValid());
  EXPECT_FALSE(
      app::serialization_detail::DateFromIsoString("garbage").IsValid());
  EXPECT_FALSE(
      app::serialization_detail::DateFromIsoString("1998/09/23").IsValid());
}

TEST(ValueSerializationTest, DateEntriesRoundTrip) {
  std::vector<DateEntry> entries(1);
  entries[0].SetDateInterval(
      DatePeriod(Date::FromYmd(1998, 9, 23), Date::FromYmd(1998, 11, 25)));
  entries[0].SetGroup(2);

  const auto loaded = XmlRoundTrip(entries);

  ASSERT_EQ(loaded.size(), 1U);
  EXPECT_EQ(loaded[0].GetDateInterval(), entries[0].GetDateInterval());
  EXPECT_EQ(loaded[0].GetGroup(), 2);
}

TEST(ValueSerializationTest, DateGroupsRoundTrip) {
  std::vector<DateGroup> groups;
  groups.emplace_back("Gruppe äöü");
  groups.back().SetNumber(3);

  const auto loaded = XmlRoundTrip(groups);

  ASSERT_EQ(loaded.size(), 1U);
  EXPECT_EQ(loaded[0].GetName(), "Gruppe äöü");
  EXPECT_EQ(loaded[0].GetNumber(), 3);
}

TEST(ValueSerializationTest, CalendarConfigRoundTrip) {
  CalendarConfig config;
  config.SetSpan({.first_year = 1998, .last_year = 2003});
  config.SetAutoCalendarSpan(false);

  const auto loaded = XmlRoundTrip(config);

  EXPECT_EQ(loaded.GetSpanLimitsYears(), (std::array<int, 2>{1998, 2003}));
  EXPECT_FALSE(loaded.IsAutoCalendarSpan());
}
