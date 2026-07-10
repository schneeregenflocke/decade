#ifndef VALUE_SERIALIZATION_HPP
#define VALUE_SERIALIZATION_HPP

// Non-intrusive Boost.Serialization for the domain value types.
//
// The domain values in `domain/` are deliberately Boost-free (SoC / Clean
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
#include <charconv>
#include <cstddef>
#include <format>
#include <glm/vec4.hpp>
#include <string>
#include <system_error>
#include <vector>

#include "../../domain/calendar_config.hpp"
#include "../../domain/date.hpp"
#include "../../domain/date_entry.hpp"
#include "../../domain/date_group.hpp"
#include "../../domain/date_period.hpp"
#include "../../domain/page_setup_config.hpp"
#include "../../domain/shape_configuration.hpp"
#include "../../domain/title_config.hpp"

namespace app::serialization_detail {

// Dates travel through the archive as ISO-8601 strings ("YYYY-MM-DD"); the
// invalid date is the empty string. This replaces the previous
// boost::gregorian greg_serialize representation — the on-disk format changed
// with the migration away from Boost.DateTime.
inline std::string DateToIsoString(const Date& date) {
  if (!date.IsValid()) {
    return {};
  }
  return std::format("{:04}-{:02}-{:02}", date.Year(), date.Month(),
                     date.Day());
}

inline Date DateFromIsoString(const std::string& text) {
  // Field positions inside "YYYY-MM-DD".
  constexpr std::size_t kIsoLength = 10;
  constexpr std::size_t kYearEnd = 4;
  constexpr std::size_t kMonthBegin = 5;
  constexpr std::size_t kMonthEnd = 7;
  constexpr std::size_t kDayBegin = 8;
  if (text.size() != kIsoLength || text[kYearEnd] != '-' ||
      text[kMonthEnd] != '-') {
    return {};
  }
  const auto parse_int = [](const char* first, const char* last, int& out) {
    return std::from_chars(first, last, out).ec == std::errc{};
  };
  int year = 0;
  int month = 0;
  int day = 0;
  const char* data = text.data();
  if (!parse_int(data, data + kYearEnd, year) ||
      !parse_int(data + kMonthBegin, data + kMonthEnd, month) ||
      !parse_int(data + kDayBegin, data + kIsoLength, day)) {
    return {};
  }
  return Date::FromYmd(year, month, day);
}

// glm::vec4 is not directly archivable, so colours travel as a 4-float array.
// These two converters localise the vec4 <-> array marshalling shared by every
// colour field, keeping the per-field save/load explicit about the on-disk
// format.
inline std::array<float, 4> ColorToArray(const glm::vec4& color) {
  return {color[0], color[1], color[2], color[3]};
}
inline glm::vec4 ColorFromArray(const std::array<float, 4>& array) {
  return {array[0], array[1], array[2], array[3]};
}

}  // namespace app::serialization_detail

namespace boost::serialization {

// --- DateGroup ---
template <class Archive>
void save(Archive& ar, const DateGroup& group, const unsigned int /*v*/) {
  const int number = group.GetNumber();
  const std::string& name = group.GetName();
  ar& make_nvp("number", number);
  ar& make_nvp("name", name);
}
template <class Archive>
void load(Archive& ar, DateGroup& group, const unsigned int /*v*/) {
  int number = 0;
  std::string name;
  ar& make_nvp("number", number);
  ar& make_nvp("name", name);
  group.SetNumber(number);
  group.SetName(std::move(name));
}

// --- DateEntry ---
// Only the primary state is persisted: the half-open interval (interval_end
// is exclusive) and the group. The derived fields (inter-interval, number,
// group number) are recomputed by DateEntryStore::ReceiveDateEntries when the
// loaded entries are pushed back into the store.
template <class Archive>
void save(Archive& ar, const DateEntry& entry, const unsigned int /*v*/) {
  const std::string interval_begin = app::serialization_detail::DateToIsoString(
      entry.GetDateInterval().Begin());
  const std::string interval_end =
      app::serialization_detail::DateToIsoString(entry.GetDateInterval().End());
  const int group = entry.GetGroup();
  ar& make_nvp("interval_begin", interval_begin);
  ar& make_nvp("interval_end", interval_end);
  ar& make_nvp("group", group);
}
template <class Archive>
void load(Archive& ar, DateEntry& entry, const unsigned int /*v*/) {
  std::string interval_begin;
  std::string interval_end;
  int group = 0;
  ar& make_nvp("interval_begin", interval_begin);
  ar& make_nvp("interval_end", interval_end);
  ar& make_nvp("group", group);
  entry.SetDateInterval(
      DatePeriod(app::serialization_detail::DateFromIsoString(interval_begin),
                 app::serialization_detail::DateFromIsoString(interval_end)));
  entry.SetGroup(group);
}

// --- PageSetupConfig ---
template <class Archive>
void save(Archive& ar, const PageSetupConfig& config,
          const unsigned int /*v*/) {
  const std::array<float, 2> size = config.Size();
  const std::array<float, 4> margins = config.Margins();
  const int orientation = config.Orientation();
  ar& make_nvp("size", size);
  ar& make_nvp("margins", margins);
  ar& make_nvp("orientation", orientation);
}

template <class Archive>
void load(Archive& ar, PageSetupConfig& config, const unsigned int /*v*/) {
  std::array<float, 2> size{};
  std::array<float, 4> margins{};
  int orientation = 0;
  ar& make_nvp("size", size);
  ar& make_nvp("margins", margins);
  ar& make_nvp("orientation", orientation);
  config.SetSize(size);
  config.SetMargins(margins);
  config.SetOrientation(orientation);
}

// --- TitleConfig ---
template <class Archive>
void save(Archive& ar, const TitleConfig& config, const unsigned int /*v*/) {
  const float frame_height = config.FrameHeight();
  const float font_size_ratio = config.FontSizeRatio();
  const std::string& title_text = config.TitleText();
  const std::array<float, 4> text_color =
      app::serialization_detail::ColorToArray(config.TextColor());
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
  config.SetTextColor(app::serialization_detail::ColorFromArray(text_color));
}

// --- ShapeConfiguration ---
template <class Archive>
void save(Archive& ar, const ShapeConfiguration& config,
          const unsigned int /*v*/) {
  const std::string& name = config.Name();
  const bool outline_visible = config.OutlineVisible();
  const bool fill_visible = config.FillVisible();
  const float line_width = config.LineWidthDisabled();
  const std::array<float, 4> outline_color =
      app::serialization_detail::ColorToArray(config.OutlineColorDisabled());
  const std::array<float, 4> fill_color =
      app::serialization_detail::ColorToArray(config.FillColorDisabled());
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
          app::serialization_detail::ColorFromArray(outline_color)},
      ShapeConfiguration::FillColorValue{
          app::serialization_detail::ColorFromArray(fill_color)});
}

// --- ShapeConfigSet ---
template <class Archive>
void save(Archive& ar, const ShapeConfigSet& set, const unsigned int /*v*/) {
  const std::vector<ShapeConfiguration>& fixed = set.FixedConfigurations();
  const std::vector<ShapeConfiguration>& groups = set.GroupConfigurations();
  ar& make_nvp("fixed_configurations", fixed);
  ar& make_nvp("group_configurations", groups);
}
template <class Archive>
void load(Archive& ar, ShapeConfigSet& set, const unsigned int /*v*/) {
  std::vector<ShapeConfiguration> fixed;
  std::vector<ShapeConfiguration> groups;
  ar& make_nvp("fixed_configurations", fixed);
  ar& make_nvp("group_configurations", groups);
  set.MutableFixedConfigurations() = std::move(fixed);
  set.MutableGroupConfigurations() = std::move(groups);
}

// --- CalendarConfig (incl. CalendarSpan year range) ---
template <class Archive>
void save(Archive& ar, const CalendarConfig& config, const unsigned int /*v*/) {
  const std::array<int, 2> limits = config.GetSpanLimitsYears();
  const int first_year = limits[0];
  const int last_year = limits[1];
  const bool auto_calendar_span = config.IsAutoCalendarSpan();
  const std::vector<float>& spacing_proportions =
      config.GetSpacingProportions();
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
BOOST_SERIALIZATION_SPLIT_FREE(DateEntry)
BOOST_SERIALIZATION_SPLIT_FREE(PageSetupConfig)
BOOST_SERIALIZATION_SPLIT_FREE(TitleConfig)
BOOST_SERIALIZATION_SPLIT_FREE(ShapeConfiguration)
BOOST_SERIALIZATION_SPLIT_FREE(ShapeConfigSet)
BOOST_SERIALIZATION_SPLIT_FREE(CalendarConfig)

#endif  // VALUE_SERIALIZATION_HPP
