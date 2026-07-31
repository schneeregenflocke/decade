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

  [[nodiscard]] float FrameHeight() const { return frame_height_; }
  void SetFrameHeight(float value) { frame_height_ = value; }

  [[nodiscard]] float FontSizePoints() const { return font_size_points_; }
  void SetFontSizePoints(float value) { font_size_points_ = value; }

  [[nodiscard]] float FontSizeMillimetres() const {
    return domain::MillimetresFromPoints(font_size_points_);
  }

  [[nodiscard]] const std::string& TitleText() const { return title_text_; }
  void SetTitleText(std::string value) { title_text_ = std::move(value); }

  [[nodiscard]] const glm::vec4& TextColor() const { return text_color_; }
  void SetTextColor(const glm::vec4& value) { text_color_ = value; }

 private:
  static constexpr float kDefaultFrameHeight = 10.0F;
  static constexpr float kDefaultFontSizePoints = 28.0F;
  static constexpr float kDefaultTextColor = 0.0F;
  static constexpr float kDefaultTextAlpha = 1.0F;

  float frame_height_{kDefaultFrameHeight};
  float font_size_points_{kDefaultFontSizePoints};
  std::string title_text_{"title config constructor text"};
  glm::vec4 text_color_{kDefaultTextColor, kDefaultTextColor, kDefaultTextColor,
                        kDefaultTextAlpha};
};
#endif  // TITLE_CONFIG_HPP
