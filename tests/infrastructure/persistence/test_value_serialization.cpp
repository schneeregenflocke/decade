#include <gtest/gtest.h>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>
#include <glm/vec4.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "domain/calendar_config.hpp"
#include "domain/date.hpp"
#include "domain/date_category.hpp"
#include "domain/date_entry.hpp"
#include "domain/date_period.hpp"
#include "domain/shape_configuration.hpp"
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

  const auto loaded = XmlRoundTrip(config);

  EXPECT_EQ(loaded.FirstYear(), 1998);
  EXPECT_EQ(loaded.LastYear(), 2003);
  EXPECT_FALSE(loaded.IsFitYearsToEntries());
}

// A project file written before the rename to category carries "Bar Group N"
// as the key of every category configuration; loading derives the key anew
// from the index and keeps the colour.
TEST(ValueSerializationTest, CategoryConfigurationKeysFollowTheirIndex) {
  const glm::vec4 teal(0.0F, 0.5F, 0.5F, 1.0F);
  ShapeConfigSet written;
  written.MutableCategoryConfigurations().emplace_back(
      "Bar Group 0", true, true, 0.3F,
      ShapeConfiguration::OutlineColorValue{teal},
      ShapeConfiguration::FillColorValue{teal});

  const auto loaded = XmlRoundTrip(written);

  ASSERT_EQ(loaded.CategoryConfigurations().size(), 1U);
  EXPECT_EQ(loaded.CategoryConfigurations()[0].Key(), "Bar Category 0");
  EXPECT_EQ(loaded.CategoryConfigurations()[0].FillColorDisabled(), teal);
}
