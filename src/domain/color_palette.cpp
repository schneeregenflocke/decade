#include "color_palette.hpp"

#include <cmath>
#include <cstddef>
#include <glm/ext/vector_float3.hpp>
#include <tinycolormap.hpp>

namespace palette {

glm::vec3 CategoricalColor(std::size_t index) {
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
