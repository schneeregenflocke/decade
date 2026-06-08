#ifndef TITLE_CONFIG_HPP
#define TITLE_CONFIG_HPP

#include <array>
#include <string>
#include <utility>

// Pure domain value. No serialization, no signal -> Rule of Zero, copyable.
class TitleConfig {
 public:
  TitleConfig() = default;

  [[nodiscard]] float FrameHeight() const { return frame_height_; }
  void SetFrameHeight(float value) { frame_height_ = value; }

  [[nodiscard]] float FontSizeRatio() const { return font_size_ratio_; }
  void SetFontSizeRatio(float value) { font_size_ratio_ = value; }

  [[nodiscard]] const std::string& TitleText() const { return title_text_; }
  void SetTitleText(std::string value) { title_text_ = std::move(value); }

  [[nodiscard]] const std::array<float, 4>& TextColor() const {
    return text_color_;
  }
  void SetTextColor(const std::array<float, 4>& value) { text_color_ = value; }

 private:
  static constexpr float kDefaultFrameHeight = 10.0F;
  static constexpr float kDefaultFontSizeRatio = 1.0F;
  static constexpr float kDefaultTextColor = 0.0F;
  static constexpr float kDefaultTextAlpha = 1.0F;

  float frame_height_{kDefaultFrameHeight};
  float font_size_ratio_{kDefaultFontSizeRatio};
  std::string title_text_{"title config constructor text"};
  std::array<float, 4> text_color_{kDefaultTextColor, kDefaultTextColor,
                                   kDefaultTextColor, kDefaultTextAlpha};
};
#endif  // TITLE_CONFIG_HPP
