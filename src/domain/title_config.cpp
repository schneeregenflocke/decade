#include "title_config.hpp"

#include <glm/ext/vector_float4.hpp>
#include <string>
#include <utility>

#include "typography.hpp"

float TitleConfig::AreaHeight() const { return area_height_; }

void TitleConfig::SetAreaHeight(float value) { area_height_ = value; }

float TitleConfig::FontSizePoints() const { return font_size_points_; }

void TitleConfig::SetFontSizePoints(float value) { font_size_points_ = value; }

float TitleConfig::FontSizeMillimetres() const {
  return domain::MillimetresFromPoints(font_size_points_);
}

const std::string& TitleConfig::TitleText() const { return title_text_; }

void TitleConfig::SetTitleText(std::string value) {
  title_text_ = std::move(value);
}

const glm::vec4& TitleConfig::TextColor() const { return text_color_; }

void TitleConfig::SetTextColor(const glm::vec4& value) { text_color_ = value; }
