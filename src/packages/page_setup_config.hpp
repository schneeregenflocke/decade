#ifndef PAGE_SETUP_CONFIG_HPP
#define PAGE_SETUP_CONFIG_HPP

#include <array>

// Pure domain value. No serialization, no signal -> aggregate, copyable.
struct PageSetupConfig {
  std::array<float, 2> size{};
  std::array<float, 4> margins{};
  int orientation{};
};
#endif  // PAGE_SETUP_CONFIG_HPP
