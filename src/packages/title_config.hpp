#ifndef TITLE_CONFIG_HPP
#define TITLE_CONFIG_HPP

#include <array>
#include <sigslot/signal.hpp>
#include <string>
#include <utility>

#include "detail/reentry_guard.hpp"

// Pure domain value. No serialization, no signal -> Rule of Zero, copyable.
class TitleConfig {
 public:
  TitleConfig() = default;

  [[nodiscard]] float FrameHeight() const { return frame_height; }
  void SetFrameHeight(float value) { frame_height = value; }

  [[nodiscard]] float FontSizeRatio() const { return font_size_ratio; }
  void SetFontSizeRatio(float value) { font_size_ratio = value; }

  [[nodiscard]] const std::string& TitleText() const { return title_text; }
  void SetTitleText(std::string value) { title_text = std::move(value); }

  [[nodiscard]] const std::array<float, 4>& TextColor() const {
    return text_color;
  }
  void SetTextColor(const std::array<float, 4>& value) { text_color = value; }

 private:
  static constexpr float kDefaultFrameHeight = 10.0F;
  static constexpr float kDefaultFontSizeRatio = 1.0F;
  static constexpr float kDefaultTextColor = 0.0F;
  static constexpr float kDefaultTextAlpha = 1.0F;

  float frame_height{kDefaultFrameHeight};
  float font_size_ratio{kDefaultFontSizeRatio};
  std::string title_text{"title config constructor text"};
  std::array<float, 4> text_color{kDefaultTextColor, kDefaultTextColor,
                                  kDefaultTextColor, kDefaultTextAlpha};
};

// Owns a TitleConfig value plus the change signal. Non-copyable. No
// serialization code (handled non-intrusively in the infrastructure layer).
class TitleConfigStore {
 public:
  TitleConfigStore() = default;
  ~TitleConfigStore() = default;
  TitleConfigStore(const TitleConfigStore&) = delete;
  TitleConfigStore& operator=(const TitleConfigStore&) = delete;
  TitleConfigStore(TitleConfigStore&&) = delete;
  TitleConfigStore& operator=(TitleConfigStore&&) = delete;

  void SendTitleConfig() { signal_title_config(title_config); }

  void ReceiveTitleConfig(const TitleConfig& incoming_title_config) {
    if (emitting_) {
      return;
    }
    const packages::detail::ScopedReentryFlag guard(emitting_);
    title_config = incoming_title_config;
    signal_title_config(title_config);
  }

  [[nodiscard]] const TitleConfig& GetTitleConfig() const {
    return title_config;
  }

  sigslot::signal<const TitleConfig&>& SignalTitleConfig() {
    return signal_title_config;
  }

 private:
  TitleConfig title_config;
  sigslot::signal<const TitleConfig&> signal_title_config;
  bool emitting_{false};
};
#endif  // TITLE_CONFIG_HPP
