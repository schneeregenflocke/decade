#include "band_part.hpp"

#include <array>
#include <numeric>

float SumOfParts(const std::array<float, kBandPartCount>& parts) {
  return std::accumulate(parts.begin(), parts.end(), 0.0F);
}
