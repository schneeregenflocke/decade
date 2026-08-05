#ifndef FONT_CONFIG_HPP
#define FONT_CONFIG_HPP

#include <string>
#include <utility>

#include "typography.hpp"

// The application-wide chosen font: the file and the size in points. A pure
// domain value, no serialisation, no signal -> rule of zero, copyable.
class FontConfig {
 public:
  FontConfig() = default;

  [[nodiscard]] const std::string& FilePath() const { return file_path_; }
  void SetFilePath(std::string value) { file_path_ = std::move(value); }

  [[nodiscard]] float SizePoints() const { return size_points_; }
  void SetSizePoints(float value) { size_points_ = value; }

  [[nodiscard]] float SizeMillimetres() const {
    return domain::MillimetresFromPoints(size_points_);
  }

 private:
  static constexpr float kDefaultSizePoints = 10.0F;

  std::string file_path_;
  float size_points_{kDefaultSizePoints};
};
#endif  // FONT_CONFIG_HPP
