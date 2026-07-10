#ifndef PAGE_SETUP_CONFIG_HPP
#define PAGE_SETUP_CONFIG_HPP

#include <array>

// Pure domain value: the page geometry (size, margins, orientation). Like the
// other value objects it encapsulates its state — data members are private,
// read through const accessors and changed only through named setters. No
// serialization, no signal -> Rule of Zero, copyable.
class PageSetupConfig {
 public:
  PageSetupConfig() = default;

  // `size` is {width, height}; `margins` is {left, bottom, right, top}.
  [[nodiscard]] const std::array<float, 2>& Size() const { return size_; }
  [[nodiscard]] const std::array<float, 4>& Margins() const { return margins_; }
  [[nodiscard]] int Orientation() const { return orientation_; }

  void SetSize(const std::array<float, 2>& size) { size_ = size; }
  void SetMargins(const std::array<float, 4>& margins) { margins_ = margins; }
  void SetOrientation(int orientation) { orientation_ = orientation; }

 private:
  std::array<float, 2> size_{};
  std::array<float, 4> margins_{};
  int orientation_{};
};
#endif  // PAGE_SETUP_CONFIG_HPP
