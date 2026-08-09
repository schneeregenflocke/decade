#ifndef TITLE_CONFIG_HPP
#define TITLE_CONFIG_HPP

#include <glm/vec4.hpp>
#include <string>
#include <utility>

#include "typography.hpp"

// Pure domain value. No serialization, no signal -> Rule of Zero, copyable.
class TitleConfig {
 public:
  TitleConfig() = default;

  [[nodiscard]] float AreaHeight() const;
  void SetAreaHeight(float value);

  [[nodiscard]] float FontSizePoints() const;
  void SetFontSizePoints(float value);

  [[nodiscard]] float FontSizeMillimetres() const;

  [[nodiscard]] const std::string& TitleText() const;
  void SetTitleText(std::string value);

  [[nodiscard]] const glm::vec4& TextColor() const;
  void SetTextColor(const glm::vec4& value);

 private:
  static constexpr float kDefaultAreaHeight = 10.0F;
  static constexpr float kDefaultFontSizePoints = 28.0F;
  static constexpr float kDefaultTextColor = 0.0F;
  static constexpr float kDefaultTextAlpha = 1.0F;

  float area_height_{kDefaultAreaHeight};
  float font_size_points_{kDefaultFontSizePoints};
  std::string title_text_{"title config constructor text"};
  glm::vec4 text_color_{kDefaultTextColor, kDefaultTextColor, kDefaultTextColor,
                        kDefaultTextAlpha};
};
#endif  // TITLE_CONFIG_HPP
