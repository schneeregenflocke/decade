#ifndef VALUE_SERIALIZATION_HPP
#define VALUE_SERIALIZATION_HPP

// Non-intrusive Boost.Serialization for the domain value types.
//
// The domain values in `domain/` are deliberately Boost-free (SoC / Clean
// Architecture): persistence is an infrastructure concern and lives here, not
// in the domain. Every serializer below goes through the value's public API
// only — no `friend`, no member `serialize`. The on-disk format is owned by
// this header alone.

#include <algorithm>
#include <array>
#include <boost/serialization/array.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <vector>

#include "../../domain/band_part.hpp"
#include "../../domain/calendar_config.hpp"
#include "../../domain/calendar_sizing.hpp"
#include "../../domain/calendar_view.hpp"
#include "../../domain/date.hpp"
#include "../../domain/date_category.hpp"
#include "../../domain/date_entry.hpp"
#include "../../domain/date_period.hpp"
#include "../../domain/page_setup_config.hpp"
#include "../../domain/shape_configuration.hpp"
#include "../../domain/title_config.hpp"

namespace persistence::serialization_detail {

// Dates travel through the archive as ISO-8601 strings ("YYYY-MM-DD"); the
// invalid date is the empty string. This replaces the previous
// boost::gregorian greg_serialize representation — the on-disk format changed
// with the migration away from Boost.DateTime.
std::string DateToIsoString(const Date& date);

Date DateFromIsoString(const std::string& text);

// glm::vec4 is not directly archivable, so colours travel as a 4-float array.
// These two converters localise the vec4 <-> array marshalling shared by every
// colour field, keeping the per-field save/load explicit about the on-disk
// format.
std::array<float, 4> ColorToArray(const glm::vec4& color);

glm::vec4 ColorFromArray(const std::array<float, 4>& array);

// A number no view carries — a file from a later version — falls back to a
// year per row rather than into an enumerator nobody handles.
CalendarView CalendarViewFromNumber(int number);

}  // namespace persistence::serialization_detail

namespace boost::serialization {

// --- DateCategory ---
template <class Archive>
void save(Archive& ar, const DateCategory& category, const unsigned int /*v*/) {
  const int number = category.GetNumber();
  const std::string& name = category.GetName();
  ar& make_nvp("number", number);
  ar& make_nvp("name", name);
}
template <class Archive>
void load(Archive& ar, DateCategory& category, const unsigned int /*v*/) {
  int number = 0;
  std::string name;
  ar& make_nvp("number", number);
  ar& make_nvp("name", name);
  category.SetNumber(number);
  category.SetName(std::move(name));
}

// --- DateEntry ---
// Only the primary state is persisted: the half-open interval (interval_end
// is exclusive) and the category. The derived fields (inter-interval, number)
// are recomputed by DateEntryStore::ReceiveDateEntries when the loaded entries
// are pushed back into the store.
template <class Archive>
void save(Archive& ar, const DateEntry& entry, const unsigned int /*v*/) {
  const std::string interval_begin =
      persistence::serialization_detail::DateToIsoString(
          entry.GetDateInterval().Begin());
  const std::string interval_end =
      persistence::serialization_detail::DateToIsoString(
          entry.GetDateInterval().End());
  const int category = entry.GetCategory();
  ar& make_nvp("interval_begin", interval_begin);
  ar& make_nvp("interval_end", interval_end);
  ar& make_nvp("category", category);
}
template <class Archive>
void load(Archive& ar, DateEntry& entry, const unsigned int /*v*/) {
  std::string interval_begin;
  std::string interval_end;
  int category = 0;
  ar& make_nvp("interval_begin", interval_begin);
  ar& make_nvp("interval_end", interval_end);
  ar& make_nvp("category", category);
  entry.SetDateInterval(DatePeriod(
      persistence::serialization_detail::DateFromIsoString(interval_begin),
      persistence::serialization_detail::DateFromIsoString(interval_end)));
  entry.SetCategory(category);
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
// The tag stays "frame_height" although the value is called AreaHeight in the
// code (#67): the tag is the on-disk format, and renaming it would make every
// project file written so far unreadable.
template <class Archive>
void save(Archive& ar, const TitleConfig& config, const unsigned int /*v*/) {
  const float frame_height = config.AreaHeight();
  const float font_size_points = config.FontSizePoints();
  const std::string& title_text = config.TitleText();
  const std::array<float, 4> text_color =
      persistence::serialization_detail::ColorToArray(config.TextColor());
  ar& make_nvp("frame_height", frame_height);
  ar& make_nvp("font_size_points", font_size_points);
  ar& make_nvp("title_text", title_text);
  ar& make_nvp("text_color", text_color);
}
template <class Archive>
void load(Archive& ar, TitleConfig& config, const unsigned int /*v*/) {
  float frame_height = 0.0F;
  float font_size_points = 0.0F;
  std::string title_text;
  std::array<float, 4> text_color{};
  ar& make_nvp("frame_height", frame_height);
  ar& make_nvp("font_size_points", font_size_points);
  ar& make_nvp("title_text", title_text);
  ar& make_nvp("text_color", text_color);
  config.SetAreaHeight(frame_height);
  config.SetFontSizePoints(font_size_points);
  config.SetTitleText(std::move(title_text));
  config.SetTextColor(
      persistence::serialization_detail::ColorFromArray(text_color));
}

// --- ShapeConfiguration ---
template <class Archive>
void save(Archive& ar, const ShapeConfiguration& config,
          const unsigned int /*v*/) {
  const std::string& key = config.Key();
  const bool outline_visible = config.OutlineVisible();
  const bool fill_visible = config.FillVisible();
  const float line_width = config.LineWidthDisabled();
  const std::array<float, 4> outline_color =
      persistence::serialization_detail::ColorToArray(
          config.OutlineColorDisabled());
  const std::array<float, 4> fill_color =
      persistence::serialization_detail::ColorToArray(
          config.FillColorDisabled());
  ar& make_nvp("key", key);
  ar& make_nvp("outline_visible", outline_visible);
  ar& make_nvp("fill_visible", fill_visible);
  ar& make_nvp("line_width", line_width);
  ar& make_nvp("outline_color", outline_color);
  ar& make_nvp("fill_color", fill_color);
}
template <class Archive>
void load(Archive& ar, ShapeConfiguration& config, const unsigned int /*v*/) {
  std::string key;
  bool outline_visible = false;
  bool fill_visible = false;
  float line_width = 0.0F;
  std::array<float, 4> outline_color{};
  std::array<float, 4> fill_color{};
  ar& make_nvp("key", key);
  ar& make_nvp("outline_visible", outline_visible);
  ar& make_nvp("fill_visible", fill_visible);
  ar& make_nvp("line_width", line_width);
  ar& make_nvp("outline_color", outline_color);
  ar& make_nvp("fill_color", fill_color);
  config = ShapeConfiguration(
      std::move(key), outline_visible, fill_visible, line_width,
      ShapeConfiguration::OutlineColorValue{
          persistence::serialization_detail::ColorFromArray(outline_color)},
      ShapeConfiguration::FillColorValue{
          persistence::serialization_detail::ColorFromArray(fill_color)});
}

// --- ShapeConfigSet ---
template <class Archive>
void save(Archive& ar, const ShapeConfigSet& set, const unsigned int /*v*/) {
  const std::vector<ShapeConfiguration>& fixed = set.FixedConfigurations();
  const std::vector<ShapeConfiguration>& categories =
      set.CategoryConfigurations();
  ar& make_nvp("fixed_configurations", fixed);
  ar& make_nvp("category_configurations", categories);
}
template <class Archive>
void load(Archive& ar, ShapeConfigSet& set, const unsigned int /*v*/) {
  std::vector<ShapeConfiguration> fixed;
  std::vector<ShapeConfiguration> categories;
  ar& make_nvp("fixed_configurations", fixed);
  ar& make_nvp("category_configurations", categories);
  set.MutableFixedConfigurations() = std::move(fixed);
  set.MutableCategoryConfigurations() = std::move(categories);
}

// --- CalendarConfig (incl. CalendarSpan year range) ---
template <class Archive>
void save(Archive& ar, const CalendarConfig& config, const unsigned int /*v*/) {
  const int first_year = config.FirstYear();
  const int last_year = config.LastYear();
  const bool fit_years_to_entries = config.IsFitYearsToEntries();
  const BandProportions& band_proportions = config.GetBandProportions();
  const std::vector<float> stored_proportions(band_proportions.begin(),
                                              band_proportions.end());
  const bool shows_annual_coverage = config.ShowsAnnualCoverage();
  const int view = static_cast<int>(config.View());
  const CalendarSizing& sizing = config.Sizing();
  const bool fixes_width = sizing.FixesWidth();
  const CalendarSizing::Widths& widths = sizing.FixedWidths();
  const bool fixes_height = sizing.FixesHeight();
  const CalendarSizing::Heights& heights = sizing.FixedHeights();
  const std::vector<float> band_part_heights(heights.parts.begin(),
                                             heights.parts.end());
  ar& make_nvp("first_year", first_year);
  ar& make_nvp("last_year", last_year);
  ar& make_nvp("fit_years_to_entries", fit_years_to_entries);
  // The element keeps its earlier name, so the files written before the
  // proportions got named after the band stay readable.
  ar& make_nvp("spacing_proportions", stored_proportions);
  ar& make_nvp("shows_annual_coverage", shows_annual_coverage);
  ar& make_nvp("view", view);
  ar& make_nvp("fixes_width", fixes_width);
  ar& make_nvp("day_width", widths.day);
  ar& make_nvp("row_labels_width", widths.row_labels);
  ar& make_nvp("legend_entry_width", widths.legend_entry);
  ar& make_nvp("fixes_height", fixes_height);
  ar& make_nvp("band_part_heights", band_part_heights);
  ar& make_nvp("column_labels_height", heights.column_labels);
  ar& make_nvp("legend_height", heights.legend);
}
template <class Archive>
void load(Archive& ar, CalendarConfig& config, const unsigned int version) {
  int first_year = 0;
  int last_year = 0;
  bool fit_years_to_entries = true;
  std::vector<float> stored_proportions;
  bool shows_annual_coverage = true;
  int view = static_cast<int>(CalendarView::kYearPerRow);
  CalendarSizing sizing;
  bool fixes_width = sizing.FixesWidth();
  CalendarSizing::Widths widths = sizing.FixedWidths();
  bool fixes_height = sizing.FixesHeight();
  CalendarSizing::Heights heights = sizing.FixedHeights();
  std::vector<float> band_part_heights;
  ar& make_nvp("first_year", first_year);
  ar& make_nvp("last_year", last_year);
  ar& make_nvp("fit_years_to_entries", fit_years_to_entries);
  ar& make_nvp("spacing_proportions", stored_proportions);
  if (version >= 1) {
    ar& make_nvp("shows_annual_coverage", shows_annual_coverage);
  }
  if (version >= 2) {
    ar& make_nvp("view", view);
  }
  if (version >= 3) {
    ar& make_nvp("fixes_width", fixes_width);
    ar& make_nvp("day_width", widths.day);
    ar& make_nvp("row_labels_width", widths.row_labels);
    ar& make_nvp("legend_entry_width", widths.legend_entry);
    ar& make_nvp("fixes_height", fixes_height);
    ar& make_nvp("band_part_heights", band_part_heights);
    ar& make_nvp("column_labels_height", heights.column_labels);
    ar& make_nvp("legend_height", heights.legend);
  }
  if (band_part_heights.size() == kBandPartCount) {
    std::ranges::copy(band_part_heights, heights.parts.begin());
  }
  sizing.SetFixesWidth(fixes_width);
  sizing.SetFixedWidths(widths);
  sizing.SetFixesHeight(fixes_height);
  sizing.SetFixedHeights(heights);
  config.SetYears(
      CalendarSpan::YearSpan{.first_year = first_year, .last_year = last_year});
  config.SetFitYearsToEntries(fit_years_to_entries);
  // An earlier layout split a band into fewer parts than the sections draw
  // into; its proportions give way to the defaults.
  if (stored_proportions.size() == kBandPartCount) {
    BandProportions band_proportions{};
    std::ranges::copy(stored_proportions, band_proportions.begin());
    config.SetBandProportions(band_proportions);
  }
  config.SetShowsAnnualCoverage(shows_annual_coverage);
  config.SetView(
      persistence::serialization_detail::CalendarViewFromNumber(view));
  config.SetSizing(sizing);
}

}  // namespace boost::serialization

BOOST_SERIALIZATION_SPLIT_FREE(DateCategory)
BOOST_SERIALIZATION_SPLIT_FREE(DateEntry)
BOOST_SERIALIZATION_SPLIT_FREE(PageSetupConfig)
BOOST_SERIALIZATION_SPLIT_FREE(TitleConfig)
BOOST_SERIALIZATION_SPLIT_FREE(ShapeConfiguration)
BOOST_SERIALIZATION_SPLIT_FREE(ShapeConfigSet)
BOOST_SERIALIZATION_SPLIT_FREE(CalendarConfig)

// Version 1 adds shows_annual_coverage; a version 0 file shows it. Version 2
// adds the view; an earlier file lays out a year per row. Version 3 adds the
// sizing; an earlier file fits both axes to the page.
BOOST_CLASS_VERSION(CalendarConfig, 3)

#endif  // VALUE_SERIALIZATION_HPP
