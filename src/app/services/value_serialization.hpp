#ifndef VALUE_SERIALIZATION_HPP
#define VALUE_SERIALIZATION_HPP

// Non-intrusive Boost.Serialization for the domain value types.
//
// The domain values in `packages/` are deliberately Boost-free (SoC / Clean
// Architecture): persistence is an infrastructure concern and lives here, not
// in the domain. Every serializer below goes through the value's public API
// only — no `friend`, no member `serialize`. The on-disk format is owned by
// this header alone.

#include <array>
#include <boost/serialization/array.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <cstddef>
#include <string>
#include <vector>
// split_free.hpp must precede greg_serialize.hpp: the latter expands
// BOOST_DATE_TIME_SPLIT_FREE, which references boost::serialization::split_free
// without including its declaration itself.
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <glm/vec4.hpp>

#include "../../packages/calendar_config.hpp"
#include "../../packages/date_store.hpp"
#include "../../packages/group_store.hpp"
#include "../../packages/page_config.hpp"
#include "../../packages/shape_config.hpp"
#include "../../packages/title_config.hpp"

namespace boost::serialization {

// --- DateGroup ---
template <class Archive>
void save(Archive& ar, const DateGroup& group, const unsigned int /*v*/) {
  const int number = group.GetNumber();
  const std::string name = group.GetName();
  const bool exclude = group.IsExcluded();
  ar& make_nvp("number", number);
  ar& make_nvp("name", name);
  ar& make_nvp("exclude", exclude);
}
template <class Archive>
void load(Archive& ar, DateGroup& group, const unsigned int /*v*/) {
  int number = 0;
  std::string name;
  bool exclude = false;
  ar& make_nvp("number", number);
  ar& make_nvp("name", name);
  ar& make_nvp("exclude", exclude);
  group.SetNumber(number);
  group.SetName(std::move(name));
  group.SetExcluded(exclude);
}

// --- DateIntervalBundle ---
template <class Archive>
void save(Archive& ar, const DateIntervalBundle& bundle,
          const unsigned int /*v*/) {
  const boost::gregorian::date_period date_interval = bundle.GetDateInterval();
  const boost::gregorian::date_period date_inter_interval =
      bundle.GetDateInterInterval();
  const int number = bundle.GetNumber();
  const int group = bundle.GetGroup();
  const int group_number = bundle.GetGroupNumber();
  const bool exclude = bundle.IsExcluded();
  const std::string comment = bundle.GetComment();
  ar& make_nvp("date_interval", date_interval);
  ar& make_nvp("date_inter_interval", date_inter_interval);
  ar& make_nvp("number", number);
  ar& make_nvp("group", group);
  ar& make_nvp("group_number", group_number);
  ar& make_nvp("exclude", exclude);
  ar& make_nvp("comment", comment);
}
template <class Archive>
void load(Archive& ar, DateIntervalBundle& bundle, const unsigned int /*v*/) {
  boost::gregorian::date_period date_interval(
      boost::gregorian::date(boost::date_time::not_a_date_time),
      boost::gregorian::date(boost::date_time::not_a_date_time));
  boost::gregorian::date_period date_inter_interval(date_interval);
  int number = 0;
  int group = 0;
  int group_number = 0;
  bool exclude = false;
  std::string comment;
  ar& make_nvp("date_interval", date_interval);
  ar& make_nvp("date_inter_interval", date_inter_interval);
  ar& make_nvp("number", number);
  ar& make_nvp("group", group);
  ar& make_nvp("group_number", group_number);
  ar& make_nvp("exclude", exclude);
  ar& make_nvp("comment", comment);
  bundle.SetDateInterval(date_interval);
  bundle.SetDateInterInterval(date_inter_interval);
  bundle.SetNumber(number);
  bundle.SetGroup(group);
  bundle.SetGroupNumber(group_number);
  bundle.SetExcluded(exclude);
  bundle.SetComment(std::move(comment));
}

// --- PageSetupConfig (aggregate, public members) ---
template <class Archive>
void serialize(Archive& ar, PageSetupConfig& config, const unsigned int /*v*/) {
  ar& make_nvp("size", config.size);
  ar& make_nvp("margins", config.margins);
  ar& make_nvp("orientation", config.orientation);
}

// --- TitleConfig ---
template <class Archive>
void save(Archive& ar, const TitleConfig& config, const unsigned int /*v*/) {
  const float frame_height = config.FrameHeight();
  const float font_size_ratio = config.FontSizeRatio();
  const std::string title_text = config.TitleText();
  const std::array<float, 4> text_color = config.TextColor();
  ar& make_nvp("frame_height", frame_height);
  ar& make_nvp("font_size_ratio", font_size_ratio);
  ar& make_nvp("title_text", title_text);
  ar& make_nvp("text_color", text_color);
}
template <class Archive>
void load(Archive& ar, TitleConfig& config, const unsigned int /*v*/) {
  float frame_height = 0.0F;
  float font_size_ratio = 0.0F;
  std::string title_text;
  std::array<float, 4> text_color{};
  ar& make_nvp("frame_height", frame_height);
  ar& make_nvp("font_size_ratio", font_size_ratio);
  ar& make_nvp("title_text", title_text);
  ar& make_nvp("text_color", text_color);
  config.SetFrameHeight(frame_height);
  config.SetFontSizeRatio(font_size_ratio);
  config.SetTitleText(std::move(title_text));
  config.SetTextColor(text_color);
}

// --- ShapeConfiguration ---
template <class Archive>
void save(Archive& ar, const ShapeConfiguration& config,
          const unsigned int /*v*/) {
  const std::string name = config.Name();
  const bool outline_visible = config.OutlineVisible();
  const bool fill_visible = config.FillVisible();
  const float line_width = config.LineWidthDisabled();
  const glm::vec4 outline = config.OutlineColorDisabled();
  const glm::vec4 fill = config.FillColorDisabled();
  const std::array<float, 4> outline_color{outline[0], outline[1], outline[2],
                                           outline[3]};
  const std::array<float, 4> fill_color{fill[0], fill[1], fill[2], fill[3]};
  ar& make_nvp("name", name);
  ar& make_nvp("outline_visible", outline_visible);
  ar& make_nvp("fill_visible", fill_visible);
  ar& make_nvp("line_width", line_width);
  ar& make_nvp("outline_color", outline_color);
  ar& make_nvp("fill_color", fill_color);
}
template <class Archive>
void load(Archive& ar, ShapeConfiguration& config, const unsigned int /*v*/) {
  std::string name;
  bool outline_visible = false;
  bool fill_visible = false;
  float line_width = 0.0F;
  std::array<float, 4> outline_color{};
  std::array<float, 4> fill_color{};
  ar& make_nvp("name", name);
  ar& make_nvp("outline_visible", outline_visible);
  ar& make_nvp("fill_visible", fill_visible);
  ar& make_nvp("line_width", line_width);
  ar& make_nvp("outline_color", outline_color);
  ar& make_nvp("fill_color", fill_color);
  config = ShapeConfiguration(
      std::move(name), outline_visible, fill_visible, line_width,
      ShapeConfiguration::OutlineColorValue{
          glm::vec4{outline_color[0], outline_color[1], outline_color[2],
                    outline_color[3]}},
      ShapeConfiguration::FillColorValue{glm::vec4{
          fill_color[0], fill_color[1], fill_color[2], fill_color[3]}});
}

// --- ShapeConfigSet ---
template <class Archive>
void save(Archive& ar, const ShapeConfigSet& set, const unsigned int /*v*/) {
  const std::vector<ShapeConfiguration>& configurations = set.Configurations();
  const std::size_t number_persistent = set.NumberPersistent();
  ar& make_nvp("configurations", configurations);
  ar& make_nvp("number_persistent", number_persistent);
}
template <class Archive>
void load(Archive& ar, ShapeConfigSet& set, const unsigned int /*v*/) {
  std::vector<ShapeConfiguration> configurations;
  std::size_t number_persistent = 0;
  ar& make_nvp("configurations", configurations);
  ar& make_nvp("number_persistent", number_persistent);
  set.MutableConfigurations() = std::move(configurations);
  set.SetNumberPersistent(number_persistent);
}

// --- CalendarConfig (incl. CalendarSpan year range) ---
template <class Archive>
void save(Archive& ar, const CalendarConfig& config, const unsigned int /*v*/) {
  const std::array<int, 2> limits = config.GetSpanLimitsYears();
  const int first_year = limits[0];
  const int last_year = limits[1];
  const bool auto_calendar_span = config.IsAutoCalendarSpan();
  const std::vector<float> spacing_proportions = config.GetSpacingProportions();
  ar& make_nvp("first_year", first_year);
  ar& make_nvp("last_year", last_year);
  ar& make_nvp("auto_calendar_span", auto_calendar_span);
  ar& make_nvp("spacing_proportions", spacing_proportions);
}
template <class Archive>
void load(Archive& ar, CalendarConfig& config, const unsigned int /*v*/) {
  int first_year = 0;
  int last_year = 0;
  bool auto_calendar_span = true;
  std::vector<float> spacing_proportions;
  ar& make_nvp("first_year", first_year);
  ar& make_nvp("last_year", last_year);
  ar& make_nvp("auto_calendar_span", auto_calendar_span);
  ar& make_nvp("spacing_proportions", spacing_proportions);
  config.SetSpan(
      CalendarSpan::YearSpan{.first_year = first_year, .last_year = last_year});
  config.SetAutoCalendarSpan(auto_calendar_span);
  config.SetSpacingProportions(spacing_proportions);
}

}  // namespace boost::serialization

BOOST_SERIALIZATION_SPLIT_FREE(DateGroup)
BOOST_SERIALIZATION_SPLIT_FREE(DateIntervalBundle)
BOOST_SERIALIZATION_SPLIT_FREE(TitleConfig)
BOOST_SERIALIZATION_SPLIT_FREE(ShapeConfiguration)
BOOST_SERIALIZATION_SPLIT_FREE(ShapeConfigSet)
BOOST_SERIALIZATION_SPLIT_FREE(CalendarConfig)

#endif  // VALUE_SERIALIZATION_HPP
