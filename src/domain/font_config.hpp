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

  [[nodiscard]] const std::string& FilePath() const;
  void SetFilePath(std::string value);

  [[nodiscard]] float SizePoints() const;
  void SetSizePoints(float value);

  [[nodiscard]] float SizeMillimetres() const;

 private:
  static constexpr float kDefaultSizePoints = 10.0F;

  std::string file_path_;
  float size_points_{kDefaultSizePoints};
};
#endif  // FONT_CONFIG_HPP
