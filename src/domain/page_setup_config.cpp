#include "page_setup_config.hpp"

#include <array>

const std::array<float, 2>& PageSetupConfig::Size() const { return size_; }

const std::array<float, 4>& PageSetupConfig::Margins() const {
  return margins_;
}

int PageSetupConfig::Orientation() const { return orientation_; }

void PageSetupConfig::SetSize(const std::array<float, 2>& size) {
  size_ = size;
}

void PageSetupConfig::SetMargins(const std::array<float, 4>& margins) {
  margins_ = margins;
}

void PageSetupConfig::SetOrientation(int orientation) {
  orientation_ = orientation;
}
