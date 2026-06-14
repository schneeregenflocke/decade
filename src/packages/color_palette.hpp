#ifndef COLOR_PALETTE_HPP
#define COLOR_PALETTE_HPP

#include <cmath>
#include <cstddef>
#include <glm/vec3.hpp>
#include <tinycolormap.hpp>

// UI-agnostic mapping from a category index to a high-quality color, backed by
// the tinycolormap library. Used to assign distinct colors to the dynamic
// "Bar Group" shapes instead of the previous random-number generator.
namespace palette {

// Perceptually-uniform colormap used to derive categorical colors. Viridis is
// colorblind-friendly and stays legible when printed in grayscale.
inline constexpr tinycolormap::ColormapType kCategoricalColormap =
    tinycolormap::ColormapType::Viridis;

// Maps a zero-based category index to a distinct, reproducible RGB color.
//
// Rather than partitioning the colormap evenly (which would shift every color
// whenever the number of categories changes), successive indices are spread
// across the colormap using the golden-ratio conjugate. This keeps each index's
// color stable as new categories are added while maximising the perceptual
// distance between consecutive entries.
[[nodiscard]] inline glm::vec3 CategoricalColor(std::size_t index) {
  constexpr double kGoldenRatioConjugate = 0.618033988749895;
  constexpr double kInitialOffset = 0.5;

  const double position = std::fmod(
      kInitialOffset + (static_cast<double>(index) * kGoldenRatioConjugate),
      1.0);
  const tinycolormap::Color color =
      tinycolormap::GetColor(position, kCategoricalColormap);

  return {static_cast<float>(color.r()), static_cast<float>(color.g()),
          static_cast<float>(color.b())};
}

}  // namespace palette
#endif  // COLOR_PALETTE_HPP
