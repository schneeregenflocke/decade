#include "shape_configuration.hpp"

#include <cstddef>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "color_palette.hpp"

ShapeConfiguration::ShapeConfiguration(std::string name, bool outline_visible,
                                       bool fill_visible, float line_width,
                                       OutlineColorValue outline_color,
                                       FillColorValue fill_color)
    : name_(std::move(name)),
      outline_visible_(outline_visible),
      fill_visible_(fill_visible),
      line_width_(line_width),
      outline_color_(outline_color.value),
      fill_color_(fill_color.value) {}

const std::string& ShapeConfiguration::Name() const { return name_; }

void ShapeConfiguration::FillVisible(bool value) { fill_visible_ = value; }

void ShapeConfiguration::OutlineVisible(bool value) {
  outline_visible_ = value;
}

bool ShapeConfiguration::FillVisible() const { return fill_visible_; }

bool ShapeConfiguration::OutlineVisible() const { return outline_visible_; }

void ShapeConfiguration::LineWidth(float value) { line_width_ = value; }

void ShapeConfiguration::OutlineColor(const glm::vec4& value) {
  outline_color_ = value;
}

void ShapeConfiguration::FillColor(const glm::vec4& value) {
  fill_color_ = value;
}

float ShapeConfiguration::LineWidth() const {
  return outline_visible_ ? line_width_ : 0.0F;
}

glm::vec4 ShapeConfiguration::OutlineColor() const {
  return outline_visible_ ? outline_color_ : glm::vec4{0.0F, 0.0F, 0.0F, 0.0F};
}

glm::vec4 ShapeConfiguration::FillColor() const {
  return fill_visible_ ? fill_color_ : glm::vec4{0.0F, 0.0F, 0.0F, 0.0F};
}

float ShapeConfiguration::LineWidthDisabled() const { return line_width_; }

glm::vec4 ShapeConfiguration::OutlineColorDisabled() const {
  return outline_color_;
}

glm::vec4 ShapeConfiguration::FillColorDisabled() const { return fill_color_; }

bool ShapeConfiguration::operator==(std::string_view compare) const {
  return Name() == compare;
}

ShapeConfigSet::ShapeConfigSet() : fixed_configurations_(BuildDefaults()) {}

ShapeConfiguration ShapeConfigSet::GetShapeConfiguration(
    std::string_view name) const {
  const ShapeConfiguration* found = Find(name);
  return found != nullptr ? *found : ShapeConfiguration{};
}

bool ShapeConfigSet::UpdateConfiguration(const ShapeConfiguration& config) {
  ShapeConfiguration* found = Find(config.Name());
  if (found == nullptr) {
    return false;
  }
  *found = config;
  return true;
}

std::string ShapeConfigSet::DynamicConfigurationName(size_t group_index) {
  return std::string(kDynamicNamePrefix) + std::to_string(group_index);
}

ShapeConfiguration ShapeConfigSet::GetDynamicConfiguration(
    size_t group_index) const {
  if (group_index >= group_configurations_.size()) {
    return {};
  }
  return group_configurations_.at(group_index);
}

void ShapeConfigSet::SyncToDateGroups(size_t group_count) {
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

const std::vector<ShapeConfiguration>& ShapeConfigSet::FixedConfigurations()
    const {
  return fixed_configurations_;
}

std::vector<ShapeConfiguration>& ShapeConfigSet::MutableFixedConfigurations() {
  return fixed_configurations_;
}

const std::vector<ShapeConfiguration>& ShapeConfigSet::GroupConfigurations()
    const {
  return group_configurations_;
}

std::vector<ShapeConfiguration>& ShapeConfigSet::MutableGroupConfigurations() {
  return group_configurations_;
}

ShapeConfiguration ShapeConfigSet::MakeCategoricalConfiguration(
    std::string name, size_t palette_index) {
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

ShapeConfiguration ShapeConfigSet::MakeBarGroupConfiguration(
    size_t group_index) {
  return MakeCategoricalConfiguration(DynamicConfigurationName(group_index),
                                      group_index);
}

void ShapeConfigSet::RefreshAnnualSumConfiguration(size_t group_count) {
  ShapeConfiguration* found = Find(kYearsTotals);
  if (found != nullptr) {
    *found =
        MakeCategoricalConfiguration(std::string(kYearsTotals), group_count);
  }
}

std::vector<ShapeConfiguration> ShapeConfigSet::BuildDefaults() {
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
  const glm::vec4 dark_quarter_transparent{kQuarter, kQuarter, kQuarter, kZero};
  // Initial "Annual Sum" placeholder: a pastel fill with a stronger outline,
  // matching the bar-group recipe. The panel re-derives the actual color from
  // the categorical palette (index = group count) on the first update.
  const glm::vec4 green_outline{kQuarter, kThreeQuarters, kQuarter,
                                kThreeQuarters};
  const glm::vec4 green_fill{kQuarter, kThreeQuarters, kQuarter, kMid};

  return {
      ShapeConfiguration{std::string(kPageMargin), true, false, kLineWidthThin,
                         ShapeConfiguration::OutlineColorValue{black_opaque},
                         ShapeConfiguration::FillColorValue{white_transparent}},
      ShapeConfiguration{std::string(kTitleFrame), true, false, kLineWidthThick,
                         ShapeConfiguration::OutlineColorValue{dark_gray},
                         ShapeConfiguration::FillColorValue{white_transparent}},
      ShapeConfiguration{
          std::string(kCalendarLabels), true, false, kLineWidthVeryThin,
          ShapeConfiguration::OutlineColorValue{light_gray_transparent},
          ShapeConfiguration::FillColorValue{black_opaque}},
      ShapeConfiguration{std::string(kDayShapes), true, true, kLineWidthThin,
                         ShapeConfiguration::OutlineColorValue{light_gray},
                         ShapeConfiguration::FillColorValue{transparent}},
      ShapeConfiguration{std::string(kSundayShapes), true, true, kLineWidthThin,
                         ShapeConfiguration::OutlineColorValue{light_gray},
                         ShapeConfiguration::FillColorValue{light_gray}},
      ShapeConfiguration{
          std::string(kMonthsShapes), true, false, kLineWidthThin,
          ShapeConfiguration::OutlineColorValue{mid_gray},
          ShapeConfiguration::FillColorValue{mid_gray_transparent}},
      ShapeConfiguration{
          "Years Shapes", false, false, kLineWidthThin,
          ShapeConfiguration::OutlineColorValue{dark_quarter},
          ShapeConfiguration::FillColorValue{dark_quarter_transparent}},
      ShapeConfiguration{std::string(kYearsTotals), true, true, kLineWidthThick,
                         ShapeConfiguration::OutlineColorValue{green_outline},
                         ShapeConfiguration::FillColorValue{green_fill}},
  };
}
