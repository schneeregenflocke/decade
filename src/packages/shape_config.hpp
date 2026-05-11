#ifndef HOME_TITAN99_CODE_DECADE_SRC_PACKAGES_SHAPE_CONFIG_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_PACKAGES_SHAPE_CONFIG_HPP

// #include "../casts.hpp"

#include <array>
#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <glm/vec4.hpp>
#include <sigslot/signal.hpp>
// #include <exception>
#include <random>
#include <string>
#include <vector>

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

  void RandomColor() {
    const float alpha_channel = 0.5F;

    std::random_device random_device;
    std::uniform_real_distribution<float> dist(0.0F, 1.0F);

    fill_color_ = std::array<float, 4>{dist(random_device), dist(random_device),
                                       dist(random_device), alpha_channel};
    outline_color_ = fill_color_;
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

  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& archive, const unsigned int version) {
    (void)version;
    archive& BOOST_SERIALIZATION_NVP(name_);
    archive& BOOST_SERIALIZATION_NVP(outline_visible_);
    archive& BOOST_SERIALIZATION_NVP(fill_visible_);
    archive& BOOST_SERIALIZATION_NVP(line_width_);
    archive& BOOST_SERIALIZATION_NVP(outline_color_);
    archive& BOOST_SERIALIZATION_NVP(fill_color_);
  }
};

class ShapeConfigurationStorage {
 public:
  ShapeConfigurationStorage()
      : shape_configurations(BuildDefaults()),
        number_persistent_configurations(shape_configurations.size()) {}

  void ReceiveShapeConfigurationStorage(
      const ShapeConfigurationStorage& shape_configuration_storage) {
    CopyFrom(shape_configuration_storage);
    SendShapeConfigurationStorage();
  }

  void SendShapeConfigurationStorage() {
    signal_shape_configuration_storage(*this);
  }

  void CopyFrom(const ShapeConfigurationStorage& other) {
    shape_configurations = other.shape_configurations;
    number_persistent_configurations = other.number_persistent_configurations;
  }

  ShapeConfigurationStorage(const ShapeConfigurationStorage&) = delete;
  ShapeConfigurationStorage(ShapeConfigurationStorage&&) = delete;
  ShapeConfigurationStorage& operator=(const ShapeConfigurationStorage&) =
      delete;
  ShapeConfigurationStorage& operator=(ShapeConfigurationStorage&&) = delete;
  ~ShapeConfigurationStorage() = default;

  [[nodiscard]] size_t size() const { return shape_configurations.size(); }

  void resize(size_t size) { shape_configurations.resize(size); }

  ShapeConfiguration& operator[](size_t index) {
    return shape_configurations.at(index);
  }
  const ShapeConfiguration& operator[](size_t index) const {
    return shape_configurations.at(index);
  }

  [[nodiscard]] ShapeConfiguration GetShapeConfiguration(
      const std::string& name) const {
    auto found = std::find(shape_configurations.begin(),
                           shape_configurations.end(), name);

    if (found == shape_configurations.end()) {
      // throw std::runtime_error("Can't find shape configuration!");
      return {};
    }
    return *found;
  }

  [[nodiscard]] size_t GetNumberPersistentConfigurations() const {
    return number_persistent_configurations;
  }

  sigslot::signal<const ShapeConfigurationStorage&>&
  SignalShapeConfigurationStorage() {
    return signal_shape_configuration_storage;
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
    const glm::vec4 green_accent{kQuarter, kThreeQuarters, kQuarter, kOne};

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
        ShapeConfiguration{"Years Totals", false, true, kLineWidthThin,
                           ShapeConfiguration::OutlineColorValue{green_accent},
                           ShapeConfiguration::FillColorValue{green_accent}},
    };
  }

  std::vector<ShapeConfiguration> shape_configurations;
  size_t number_persistent_configurations{0};
  sigslot::signal<const ShapeConfigurationStorage&>
      signal_shape_configuration_storage;

  friend class boost::serialization::access;
  template <class Archive>
  void save(Archive& archive, const unsigned int version) const {
    (void)version;
    archive& BOOST_SERIALIZATION_NVP(shape_configurations);
    archive& BOOST_SERIALIZATION_NVP(number_persistent_configurations);
  }
  template <class Archive>
  void load(Archive& archive, const unsigned int version) {
    (void)version;
    archive& BOOST_SERIALIZATION_NVP(shape_configurations);
    archive& BOOST_SERIALIZATION_NVP(number_persistent_configurations);
    SendShapeConfigurationStorage();
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()
};
#endif  // HOME_TITAN99_CODE_DECADE_SRC_PACKAGES_SHAPE_CONFIG_HPP
