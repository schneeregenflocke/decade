#ifndef SHAPE_CONFIGURATION_HPP
#define SHAPE_CONFIGURATION_HPP

#include <algorithm>
#include <glm/vec4.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "color_palette.hpp"

// Pure domain value: the visual configuration of one shape kind. No
// serialization, no signal -> Rule of Zero, copyable.
class ShapeConfiguration {
 public:
  struct OutlineColorValue {
    glm::vec4 value;
  };

  struct FillColorValue {
    glm::vec4 value;
  };

  ShapeConfiguration() = default;

  ShapeConfiguration(std::string name, bool outline_visible, bool fill_visible,
                     float line_width, OutlineColorValue outline_color,
                     FillColorValue fill_color)
      : name_(std::move(name)),
        outline_visible_(outline_visible),
        fill_visible_(fill_visible),
        line_width_(line_width),
        outline_color_(outline_color.value),
        fill_color_(fill_color.value) {}

  [[nodiscard]] const std::string& Name() const { return name_; }

  void FillVisible(bool value) { fill_visible_ = value; }

  void OutlineVisible(bool value) { outline_visible_ = value; }

  [[nodiscard]] bool FillVisible() const { return fill_visible_; }

  [[nodiscard]] bool OutlineVisible() const { return outline_visible_; }

  void LineWidth(float value) { line_width_ = value; }

  void OutlineColor(const glm::vec4& value) { outline_color_ = value; }

  void FillColor(const glm::vec4& value) { fill_color_ = value; }

  [[nodiscard]] float LineWidth() const {
    return outline_visible_ ? line_width_ : 0.0F;
  }

  [[nodiscard]] glm::vec4 OutlineColor() const {
    return outline_visible_ ? outline_color_
                            : glm::vec4{0.0F, 0.0F, 0.0F, 0.0F};
  }

  [[nodiscard]] glm::vec4 FillColor() const {
    return fill_visible_ ? fill_color_ : glm::vec4{0.0F, 0.0F, 0.0F, 0.0F};
  }

  [[nodiscard]] float LineWidthDisabled() const { return line_width_; }

  [[nodiscard]] glm::vec4 OutlineColorDisabled() const {
    return outline_color_;
  }

  [[nodiscard]] glm::vec4 FillColorDisabled() const { return fill_color_; }

  bool operator==(const std::string& compare) const {
    return Name() == compare;
  }

 private:
  std::string name_;
  bool outline_visible_{true};
  bool fill_visible_{false};
  float line_width_{1.0F};
  glm::vec4 outline_color_{0.0F, 0.0F, 0.0F, 1.0F};
  glm::vec4 fill_color_{0.0F, 0.0F, 0.0F, 1.0F};
};

// Pure value object: the ordered set of shape configurations (the fixed ones
// followed by per-date-group dynamic ones) plus the lookups over them. Dynamic
// configurations are identified by their name (the "Bar Group N" pattern), so
// the set needs no boundary index. No signal -> Rule of Zero, copyable.
class ShapeConfigSet {
 public:
  ShapeConfigSet() : shape_configurations_(BuildDefaults()) {}

  [[nodiscard]] size_t size() const { return shape_configurations_.size(); }

  ShapeConfiguration& operator[](size_t index) {
    return shape_configurations_.at(index);
  }
  const ShapeConfiguration& operator[](size_t index) const {
    return shape_configurations_.at(index);
  }

  [[nodiscard]] ShapeConfiguration GetShapeConfiguration(
      const std::string& name) const {
    auto found = std::ranges::find_if(
        shape_configurations_,
        [&](const ShapeConfiguration& config) { return config == name; });

    if (found == shape_configurations_.end()) {
      return {};
    }
    return *found;
  }

  // Display name of the dynamic (per date group) configuration at the given
  // zero-based group index. Kept here so producers and consumers share one
  // definition instead of reconstructing the string in several places.
  [[nodiscard]] static std::string DynamicConfigurationName(
      size_t group_index) {
    return std::string(kDynamicNamePrefix) + std::to_string(group_index);
  }

  // Whether `name` denotes a dynamic per-date-group configuration. This is the
  // single criterion that separates the fixed configurations from the
  // group-driven ones, replacing the former persistent-count boundary index.
  [[nodiscard]] static bool IsDynamicConfigurationName(
      const std::string& name) {
    return name.starts_with(kDynamicNamePrefix);
  }

  // Name of the persistent "Annual Sum" (per-year total) configuration. Shared
  // by the panel and the renderer so the string lives in exactly one place.
  [[nodiscard]] static std::string AnnualSumConfigurationName() {
    return "Years Totals";
  }

  // The configuration for the dynamic bar group at the given zero-based index,
  // resolved by name (a default-constructed value when absent).
  [[nodiscard]] ShapeConfiguration GetDynamicConfiguration(
      size_t group_index) const {
    return GetShapeConfiguration(DynamicConfigurationName(group_index));
  }

  // Reconciles the dynamic per-group configurations with the current date
  // groups: keeps the fixed configurations and any existing group entry (so
  // user customisations survive), drops stale group entries and synthesises
  // fresh ones from the palette for newly added groups. The "Annual Sum" entry
  // is re-derived from the palette only when the group count actually changed,
  // so a rename of a group keeps its customised colour.
  void SyncToDateGroups(size_t group_count) {
    std::unordered_map<std::string, ShapeConfiguration> existing_dynamic;
    std::vector<ShapeConfiguration> fixed;
    fixed.reserve(shape_configurations_.size());
    for (const ShapeConfiguration& config : shape_configurations_) {
      if (IsDynamicConfigurationName(config.Name())) {
        existing_dynamic.emplace(config.Name(), config);
      } else {
        fixed.push_back(config);
      }
    }
    const size_t previous_group_count = existing_dynamic.size();

    shape_configurations_ = std::move(fixed);
    for (size_t index = 0; index < group_count; ++index) {
      const std::string name = DynamicConfigurationName(index);
      const auto found = existing_dynamic.find(name);
      shape_configurations_.push_back(found != existing_dynamic.end()
                                          ? found->second
                                          : MakeBarGroupConfiguration(index));
    }

    if (group_count != previous_group_count) {
      RefreshAnnualSumConfiguration(group_count);
    }
  }

  // Raw access for non-intrusive serialization in the infrastructure layer.
  [[nodiscard]] const std::vector<ShapeConfiguration>& Configurations() const {
    return shape_configurations_;
  }
  [[nodiscard]] std::vector<ShapeConfiguration>& MutableConfigurations() {
    return shape_configurations_;
  }

 private:
  // Name prefix shared by every dynamic per-date-group configuration; the sole
  // marker distinguishing group entries from the fixed ones.
  static constexpr std::string_view kDynamicNamePrefix = "Bar Group ";

  // Builds a categorical shape configuration: the colour comes from the shared
  // palette at the given index, with a stronger outline than fill so the box
  // has a visible border while the fill stays pastel. The single source of the
  // alpha recipe for every palette-driven entry (bar groups and the annual
  // sum).
  static ShapeConfiguration MakeCategoricalConfiguration(std::string name,
                                                         size_t palette_index) {
    constexpr float kDynamicLineWidth = 0.5F;
    constexpr float kOutlineAlpha = 0.75F;
    constexpr float kFillAlpha = 0.35F;

    const glm::vec3 color = palette::CategoricalColor(palette_index);

    return ShapeConfiguration{
        std::move(name),
        true,
        true,
        kDynamicLineWidth,
        ShapeConfiguration::OutlineColorValue{glm::vec4(color, kOutlineAlpha)},
        ShapeConfiguration::FillColorValue{glm::vec4(color, kFillAlpha)}};
  }

  // Default configuration for the dynamic bar group at the given zero-based
  // index, reproducible across sessions because the palette is index-stable.
  static ShapeConfiguration MakeBarGroupConfiguration(size_t group_index) {
    return MakeCategoricalConfiguration(DynamicConfigurationName(group_index),
                                        group_index);
  }

  // Re-derives the "Annual Sum" configuration from the palette at the index
  // right past the last group, so it is coloured and styled exactly like a bar
  // group and stays consistent if the palette is ever changed.
  void RefreshAnnualSumConfiguration(size_t group_count) {
    for (ShapeConfiguration& config : shape_configurations_) {
      if (config == AnnualSumConfigurationName()) {
        config = MakeCategoricalConfiguration(AnnualSumConfigurationName(),
                                              group_count);
        break;
      }
    }
  }

  static std::vector<ShapeConfiguration> BuildDefaults() {
    constexpr float kZero = 0.0F;
    constexpr float kOne = 1.0F;
    constexpr float kQuarter = 0.25F;
    constexpr float kHalf = 0.5F;
    constexpr float kThreeQuarters = 0.75F;
    constexpr float kDark = 0.1F;
    constexpr float kMid = 0.4F;
    constexpr float kLight = 0.85F;

    constexpr float kLineWidthVeryThin = 0.1F;
    constexpr float kLineWidthThin = 0.2F;
    constexpr float kLineWidthThick = 0.5F;

    const glm::vec4 black_opaque{kZero, kZero, kZero, kOne};
    const glm::vec4 white_transparent{kOne, kOne, kOne, kZero};
    const glm::vec4 dark_gray{kDark, kDark, kDark, kOne};
    const glm::vec4 light_gray_transparent{kThreeQuarters, kThreeQuarters,
                                           kThreeQuarters, kQuarter};
    const glm::vec4 light_gray{kLight, kLight, kLight, kOne};
    const glm::vec4 transparent{kZero, kZero, kZero, kZero};
    const glm::vec4 mid_gray{kMid, kMid, kMid, kOne};
    const glm::vec4 mid_gray_transparent{kHalf, kHalf, kHalf, kZero};
    const glm::vec4 dark_quarter{kQuarter, kQuarter, kQuarter, kOne};
    const glm::vec4 dark_quarter_transparent{kQuarter, kQuarter, kQuarter,
                                             kZero};
    // Initial "Annual Sum" placeholder: a pastel fill with a stronger outline,
    // matching the bar-group recipe. The panel re-derives the actual color from
    // the categorical palette (index = group count) on the first update.
    const glm::vec4 green_outline{kQuarter, kThreeQuarters, kQuarter,
                                  kThreeQuarters};
    const glm::vec4 green_fill{kQuarter, kThreeQuarters, kQuarter, kMid};

    return {
        ShapeConfiguration{
            "Page Margin", true, false, kLineWidthThin,
            ShapeConfiguration::OutlineColorValue{black_opaque},
            ShapeConfiguration::FillColorValue{white_transparent}},
        ShapeConfiguration{
            "Title Frame", true, false, kLineWidthThick,
            ShapeConfiguration::OutlineColorValue{dark_gray},
            ShapeConfiguration::FillColorValue{white_transparent}},
        ShapeConfiguration{
            "Calendar Labels", true, false, kLineWidthVeryThin,
            ShapeConfiguration::OutlineColorValue{light_gray_transparent},
            ShapeConfiguration::FillColorValue{black_opaque}},
        ShapeConfiguration{"Day Shapes", true, true, kLineWidthThin,
                           ShapeConfiguration::OutlineColorValue{light_gray},
                           ShapeConfiguration::FillColorValue{transparent}},
        ShapeConfiguration{"Sunday Shapes", true, true, kLineWidthThin,
                           ShapeConfiguration::OutlineColorValue{light_gray},
                           ShapeConfiguration::FillColorValue{light_gray}},
        ShapeConfiguration{
            "Months Shapes", true, false, kLineWidthThin,
            ShapeConfiguration::OutlineColorValue{mid_gray},
            ShapeConfiguration::FillColorValue{mid_gray_transparent}},
        ShapeConfiguration{
            "Years Shapes", false, false, kLineWidthThin,
            ShapeConfiguration::OutlineColorValue{dark_quarter},
            ShapeConfiguration::FillColorValue{dark_quarter_transparent}},
        ShapeConfiguration{AnnualSumConfigurationName(), true, true,
                           kLineWidthThick,
                           ShapeConfiguration::OutlineColorValue{green_outline},
                           ShapeConfiguration::FillColorValue{green_fill}},
    };
  }

  std::vector<ShapeConfiguration> shape_configurations_;
};
#endif  // SHAPE_CONFIGURATION_HPP
