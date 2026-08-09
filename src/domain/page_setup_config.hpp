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
  [[nodiscard]] const std::array<float, 2>& Size() const;
  [[nodiscard]] const std::array<float, 4>& Margins() const;
  [[nodiscard]] int Orientation() const;

  void SetSize(const std::array<float, 2>& size);
  void SetMargins(const std::array<float, 4>& margins);
  void SetOrientation(int orientation);

 private:
  std::array<float, 2> size_{};
  std::array<float, 4> margins_{};
  int orientation_{};
};
#endif  // PAGE_SETUP_CONFIG_HPP
