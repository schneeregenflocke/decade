#ifndef BAND_PART_HPP
#define BAND_PART_HPP

#include <array>
#include <cstddef>
#include <cstdint>

// The parts a year's band stacks from the bottom up: three rows of content,
// each with a gap below, and a gap above the top row.
enum class BandPart : std::uint8_t {
  kBelowCoverage,
  kCoverage,
  kBelowDays,
  kDays,
  kBelowLabels,
  kEntryLabels,
  kAboveLabels,
};

inline constexpr std::size_t kBandPartCount = 7;

// Relative heights, indexed by BandPart.
using BandProportions = std::array<float, kBandPartCount>;

// Heights in millimetres, indexed by BandPart.
using BandHeights = std::array<float, kBandPartCount>;

[[nodiscard]] float SumOfParts(const std::array<float, kBandPartCount>& parts);

#endif  // BAND_PART_HPP
