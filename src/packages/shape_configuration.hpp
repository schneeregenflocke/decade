#ifndef SHAPE_CONFIGURATION_HPP
#define SHAPE_CONFIGURATION_HPP

#include <algorithm>
#include <glm/vec4.hpp>
#include <string>
#include <string_view>
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

// Pure value object: the shape configurations, split into the fixed ones (page
// margin, labels, the per-year "Annual Sum", …) and the per-date-group ones.
// The two live in separate containers, so a group configuration is identified
// structurally (its position in the group list), not by parsing its name. No
// signal -> Rule of Zero, copyable.
class ShapeConfigSet {
 public:
  ShapeConfigSet() : fixed_configurations_(BuildDefaults()) {}

  // The configuration with the given name, searched across the fixed and the
  // group configurations (a default-constructed value when absent). The name
  // remains a stable per-configuration label (and a node's style id); it is
  // just no longer what decides whether a configuration is a group entry.
  [[nodiscard]] ShapeConfiguration GetShapeConfiguration(
      const std::string& name) const {
    const ShapeConfiguration* found = Find(name);
    return found != nullptr ? *found : ShapeConfiguration{};
  }

  // Replaces the configuration that shares `config`'s name, in whichever
  // container holds it. Returns false when no such configuration exists.
  bool UpdateConfiguration(const ShapeConfiguration& config) {
    ShapeConfiguration* found = FindMutable(config.Name());
    if (found == nullptr) {
      return false;
    }
    *found = config;
    return true;
  }

  // Display name of the per-date-group configuration at the given zero-based
  // index. Still used to label the group configurations (and as the matching
  // node style id); group membership no longer depends on it.
  [[nodiscard]] static std::string DynamicConfigurationName(
      size_t group_index) {
    return std::string(kDynamicNamePrefix) + std::to_string(group_index);
  }

  // Name of the "Annual Sum" (per-year total) configuration, one of the fixed
  // configurations. Shared by the renderer so the string lives in one place.
  [[nodiscard]] static std::string AnnualSumConfigurationName() {
    return "Years Totals";
  }

  // The configuration for the date group at the given zero-based index (a
  // default-constructed value when out of range).
  [[nodiscard]] ShapeConfiguration GetDynamicConfiguration(
      size_t group_index) const {
    if (group_index >= group_configurations_.size()) {
      return {};
    }
    return group_configurations_.at(group_index);
  }

  // Reconciles the group configurations with the current date groups: keeps the
  // existing entries (so user customisations survive), drops the entries past
  // `group_count` and synthesises fresh ones from the palette for newly added
  // groups. The "Annual Sum" entry is re-derived from the palette only when the
  // group count actually changed, so a rename of a group keeps its colour.
  void SyncToDateGroups(size_t group_count) {
    const size_t previous_group_count = group_configurations_.size();
    if (group_count < previous_group_count) {
      group_configurations_.resize(group_count);
    } else {
      for (size_t index = previous_group_count; index < group_count; ++index) {
        group_configurations_.push_back(MakeBarGroupConfiguration(index));
      }
    }
    if (group_count != previous_group_count) {
      RefreshAnnualSumConfiguration(group_count);
    }
  }

  // Raw access for non-intrusive serialization in the infrastructure layer.
  [[nodiscard]] const std::vector<ShapeConfiguration>& FixedConfigurations()
      const {
    return fixed_configurations_;
  }
  [[nodiscard]] std::vector<ShapeConfiguration>& MutableFixedConfigurations() {
    return fixed_configurations_;
  }
  [[nodiscard]] const std::vector<ShapeConfiguration>& GroupConfigurations()
      const {
    return group_configurations_;
  }
  [[nodiscard]] std::vector<ShapeConfiguration>& MutableGroupConfigurations() {
    return group_configurations_;
  }

 private:
  // Name prefix shared by every per-date-group configuration; used only to
  // label them, no longer to decide group membership.
  static constexpr std::string_view kDynamicNamePrefix = "Bar Group ";

  // Locates the configuration with the given name across both containers
  // (fixed first, then group), or nullptr when absent.
  [[nodiscard]] const ShapeConfiguration* Find(const std::string& name) const {
    for (const std::vector<ShapeConfiguration>* container :
         {&fixed_configurations_, &group_configurations_}) {
      const auto found = std::ranges::find_if(
          *container,
          [&](const ShapeConfiguration& config) { return config == name; });
      if (found != container->end()) {
        return &*found;
      }
    }
    return nullptr;
  }
  ShapeConfiguration* FindMutable(const std::string& name) {
    for (std::vector<ShapeConfiguration>* container :
         {&fixed_configurations_, &group_configurations_}) {
      const auto found = std::ranges::find_if(
          *container,
          [&](const ShapeConfiguration& config) { return config == name; });
      if (found != container->end()) {
        return &*found;
      }
    }
    return nullptr;
  }

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
    ShapeConfiguration* found = FindMutable(AnnualSumConfigurationName());
    if (found != nullptr) {
      *found = MakeCategoricalConfiguration(AnnualSumConfigurationName(),
                                            group_count);
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

  std::vector<ShapeConfiguration> fixed_configurations_;
  std::vector<ShapeConfiguration> group_configurations_;
};
#endif  // SHAPE_CONFIGURATION_HPP
