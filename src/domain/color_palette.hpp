#ifndef COLOR_PALETTE_HPP
#define COLOR_PALETTE_HPP

#include <cstddef>
#include <glm/vec3.hpp>

namespace palette {

// Maps a zero-based category index to a distinct, reproducible RGB color.
// Indices beyond the palette wrap around.
[[nodiscard]] glm::vec3 CategoricalColor(std::size_t index);

}  // namespace palette
#endif  // COLOR_PALETTE_HPP
