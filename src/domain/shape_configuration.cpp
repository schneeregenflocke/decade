#include "shape_configuration.hpp"

#include <cstddef>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "color_palette.hpp"

ShapeConfiguration::ShapeConfiguration(std::string key, bool outline_visible,
                                       bool fill_visible, float line_width,
                                       OutlineColorValue outline_color,
                                       FillColorValue fill_color)
    : key_(std::move(key)),
      outline_visible_(outline_visible),
      fill_visible_(fill_visible),
      line_width_(line_width),
      outline_color_(outline_color.value),
      fill_color_(fill_color.value) {}

const std::string& ShapeConfiguration::Key() const { return key_; }

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
  return Key() == compare;
}

ShapeConfigSet::ShapeConfigSet() : fixed_configurations_(BuildDefaults()) {}

ShapeConfiguration ShapeConfigSet::GetShapeConfiguration(
    std::string_view key) const {
  const ShapeConfiguration* found = Find(key);
  return found != nullptr ? *found : ShapeConfiguration{};
}

bool ShapeConfigSet::UpdateConfiguration(const ShapeConfiguration& config) {
  ShapeConfiguration* found = Find(config.Key());
  if (found == nullptr) {
    return false;
  }
  *found = config;
  return true;
}

std::string ShapeConfigSet::DynamicConfigurationKey(size_t group_index) {
  return std::string(kGroupKeyPrefix) + std::to_string(group_index);
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

ShapeConfiguration ShapeConfigSet::MakeBarStyledConfiguration(
    std::string key, const glm::vec3& color) {
  constexpr float kDynamicLineWidth = 0.5F;
  constexpr float kOutlineAlpha = 0.75F;
  constexpr float kFillAlpha = 0.35F;

  return ShapeConfiguration{
      std::move(key),
      true,
      true,
      kDynamicLineWidth,
      ShapeConfiguration::OutlineColorValue{glm::vec4(color, kOutlineAlpha)},
      ShapeConfiguration::FillColorValue{glm::vec4(color, kFillAlpha)}};
}

ShapeConfiguration ShapeConfigSet::MakeBarGroupConfiguration(
    size_t group_index) {
  return MakeBarStyledConfiguration(DynamicConfigurationKey(group_index),
                                    palette::CategoricalColor(group_index));
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
  // The annual coverage is no category, so it takes no colour out of the
  // categorical palette: a neutral grey tells the aggregate apart from the
  // groups it sums up, and it collides with none of them however many there
  // are — Viridis is saturated end to end.
  const glm::vec3 annual_coverage_gray{kMid, kMid, kMid};

  return {
      ShapeConfiguration{std::string(kPageMarginKey), true, false,
                         kLineWidthThin,
                         ShapeConfiguration::OutlineColorValue{black_opaque},
                         ShapeConfiguration::FillColorValue{white_transparent}},
      ShapeConfiguration{std::string(kTitleFrameKey), true, false,
                         kLineWidthThick,
                         ShapeConfiguration::OutlineColorValue{dark_gray},
                         ShapeConfiguration::FillColorValue{white_transparent}},
      ShapeConfiguration{
          std::string(kCalendarLabelsKey), true, false, kLineWidthVeryThin,
          ShapeConfiguration::OutlineColorValue{light_gray_transparent},
          ShapeConfiguration::FillColorValue{black_opaque}},
      ShapeConfiguration{std::string(kDayShapesKey), true, true, kLineWidthThin,
                         ShapeConfiguration::OutlineColorValue{light_gray},
                         ShapeConfiguration::FillColorValue{transparent}},
      ShapeConfiguration{std::string(kSundayShapesKey), true, true,
                         kLineWidthThin,
                         ShapeConfiguration::OutlineColorValue{light_gray},
                         ShapeConfiguration::FillColorValue{light_gray}},
      ShapeConfiguration{
          std::string(kMonthsShapesKey), true, false, kLineWidthThin,
          ShapeConfiguration::OutlineColorValue{mid_gray},
          ShapeConfiguration::FillColorValue{mid_gray_transparent}},
      ShapeConfiguration{
          std::string(kYearsShapesKey), false, false, kLineWidthThin,
          ShapeConfiguration::OutlineColorValue{dark_quarter},
          ShapeConfiguration::FillColorValue{dark_quarter_transparent}},
      MakeBarStyledConfiguration(std::string(kAnnualCoverageKey),
                                 annual_coverage_gray),
  };
}
