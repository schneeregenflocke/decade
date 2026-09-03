#include "color_palette.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float3.hpp>

namespace palette {
namespace {

// Qt's own palette, Qt::GlobalColor, minus the achromatic entries: black and
// the greys collide with the page grid once the bar fill drops to a third of
// its opacity, and white leaves nothing to see. The values are copied rather
// than taken from Qt::red and its neighbours because those live in QtGui,
// which the domain layer must not reach into.
//
// Hues alternate between the full and the half-intensity variant so that
// neighbouring group indices differ in lightness as well as in hue.
constexpr std::array<std::uint32_t, 12> kCategoricalColors{
    0xFF0000,  // red
    0x008080,  // darkCyan
    0xFFFF00,  // yellow
    0x000080,  // darkBlue
    0x00FF00,  // green
    0x800080,  // darkMagenta
    0x00FFFF,  // cyan
    0x800000,  // darkRed
    0xFF00FF,  // magenta
    0x008000,  // darkGreen
    0x0000FF,  // blue
    0x808000,  // darkYellow
};

constexpr float kByteMaximum = 255.0F;
constexpr std::uint32_t kChannelMask = 0xFFU;
constexpr unsigned int kRedShift = 16U;
constexpr unsigned int kGreenShift = 8U;
constexpr unsigned int kBlueShift = 0U;

glm::vec3 FromHex(std::uint32_t hex) {
  const auto channel = [hex](unsigned int shift) {
    return static_cast<float>((hex >> shift) & kChannelMask) / kByteMaximum;
  };
  return {channel(kRedShift), channel(kGreenShift), channel(kBlueShift)};
}

}  // namespace

glm::vec3 CategoricalColor(std::size_t index) {
  return FromHex(kCategoricalColors.at(index % kCategoricalColors.size()));
}

}  // namespace palette
