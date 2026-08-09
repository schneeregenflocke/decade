#include "font_config.hpp"

#include <string>
#include <utility>

#include "typography.hpp"

const std::string& FontConfig::FilePath() const { return file_path_; }

void FontConfig::SetFilePath(std::string value) {
  file_path_ = std::move(value);
}

float FontConfig::SizePoints() const { return size_points_; }

void FontConfig::SetSizePoints(float value) { size_points_ = value; }

float FontConfig::SizeMillimetres() const {
  return domain::MillimetresFromPoints(size_points_);
}
