#ifndef SHAPE_CONFIGURATION_HPP
#define SHAPE_CONFIGURATION_HPP

#include <algorithm>
#include <array>
#include <glm/vec4.hpp>
#include <string>
#include <utility>
#include <vector>

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
        outline_color_({outline_color.value[0], outline_color.value[1],
                        outline_color.value[2], outline_color.value[3]}),
        fill_color_({fill_color.value[0], fill_color.value[1],
                     fill_color.value[2], fill_color.value[3]}) {}

  [[nodiscard]] const std::string& Name() const { return name_; }

  void FillVisible(bool value) { fill_visible_ = value; }

  void OutlineVisible(bool value) { outline_visible_ = value; }

  [[nodiscard]] bool FillVisible() const { return fill_visible_; }

  [[nodiscard]] bool OutlineVisible() const { return outline_visible_; }

  void LineWidth(float value) { line_width_ = value; }

  void OutlineColor(const glm::vec4& value) {
    outline_color_ = {value[0], value[1], value[2], value[3]};
  }

  void FillColor(const glm::vec4& value) {
    fill_color_ = {value[0], value[1], value[2], value[3]};
  }

  [[nodiscard]] float LineWidth() const {
    float return_value = 0.0F;

    if (outline_visible_) {
      return_value = line_width_;
    }

    return return_value;
  }

  [[nodiscard]] glm::vec4 OutlineColor() const {
    glm::vec4 value = glm::vec4{0.0F, 0.0F, 0.0F, 0.0F};

    if (outline_visible_) {
      value = {outline_color_[0], outline_color_[1], outline_color_[2],
               outline_color_[3]};
    }

    return value;
  }

  [[nodiscard]] glm::vec4 FillColor() const {
    glm::vec4 value = glm::vec4{0.0F, 0.0F, 0.0F, 0.0F};

    if (fill_visible_) {
      value = {fill_color_[0], fill_color_[1], fill_color_[2], fill_color_[3]};
    }

    return value;
  }

  [[nodiscard]] float LineWidthDisabled() const { return line_width_; }

  [[nodiscard]] glm::vec4 OutlineColorDisabled() const {
    return {outline_color_[0], outline_color_[1], outline_color_[2],
            outline_color_[3]};
  }

  [[nodiscard]] glm::vec4 FillColorDisabled() const {
    return {fill_color_[0], fill_color_[1], fill_color_[2], fill_color_[3]};
  }

  bool operator==(const std::string& compare) const {
    return Name() == compare;
  }

 private:
  std::string name_;
  bool outline_visible_{true};
  bool fill_visible_{false};
  float line_width_{1.0F};
  std::array<float, 4> outline_color_{0.0F, 0.0F, 0.0F, 1.0F};
  std::array<float, 4> fill_color_{0.0F, 0.0F, 0.0F, 1.0F};
};

// Pure value object: the ordered set of shape configurations (persistent ones
// followed by per-date-group dynamic ones) plus the lookups over them. No
// signal -> Rule of Zero, copyable.
class ShapeConfigSet {
 public:
  ShapeConfigSet()
      : shape_configurations_(BuildDefaults()),
        number_persistent_configurations_(shape_configurations_.size()) {}

  [[nodiscard]] size_t size() const { return shape_configurations_.size(); }

  void resize(size_t size) { shape_configurations_.resize(size); }

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
    return "Bar Group " + std::to_string(group_index);
  }

  // Name of the persistent "Annual Sum" (per-year total) configuration. Shared
  // by the panel and the renderer so the string lives in exactly one place.
  [[nodiscard]] static std::string AnnualSumConfigurationName() {
    return "Years Totals";
  }

  // Dynamic configurations are stored immediately after the persistent ones, so
  // a group's configuration can be addressed by its index directly.
  [[nodiscard]] ShapeConfiguration GetDynamicConfiguration(
      size_t group_index) const {
    const size_t config_index = number_persistent_configurations_ + group_index;
    if (config_index >= shape_configurations_.size()) {
      return {};
    }
    return shape_configurations_.at(config_index);
  }

  [[nodiscard]] size_t GetNumberPersistentConfigurations() const {
    return number_persistent_configurations_;
  }

  // Raw access for non-intrusive serialization in the infrastructure layer.
  [[nodiscard]] const std::vector<ShapeConfiguration>& Configurations() const {
    return shape_configurations_;
  }
  [[nodiscard]] std::vector<ShapeConfiguration>& MutableConfigurations() {
    return shape_configurations_;
  }
  [[nodiscard]] size_t NumberPersistent() const {
    return number_persistent_configurations_;
  }
  void SetNumberPersistent(size_t value) {
    number_persistent_configurations_ = value;
  }

 private:
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
  size_t number_persistent_configurations_{0};
};
#endif  // SHAPE_CONFIGURATION_HPP
