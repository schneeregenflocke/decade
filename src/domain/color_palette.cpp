#include "color_palette.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <limits>
#include <vector>

namespace palette {
namespace {

// Qt's standard swatches, rebuilt from the three loops QColorDialog fills them
// with (qplatformdialoghelper.cpp): a 4x4x3 sampling of the RGB cube. Rebuilt
// rather than read from Qt, because QColorDialog lives in QtWidgets and the
// domain layer must not reach into it.
constexpr int kRedSteps = 4;
constexpr int kGreenSteps = 4;
constexpr int kBlueSteps = 3;
constexpr float kByteMaximum = 255.0F;

// Qt::GlobalColor, the chromatic entries alone. They carry the fully saturated
// primaries the cube sampling misses.
constexpr std::array<std::uint32_t, 12> kQtGlobalColors{
    0xFF0000, 0x800000, 0x00FF00, 0x008000, 0x0000FF, 0x000080,
    0x00FFFF, 0x008080, 0xFF00FF, 0x800080, 0xFFFF00, 0x808000,
};

constexpr std::uint32_t kChannelMask = 0xFFU;
constexpr unsigned int kRedShift = 16U;
constexpr unsigned int kGreenShift = 8U;
constexpr unsigned int kBlueShift = 0U;

// One lightness for every group, so no bar shouts louder than its neighbours
// merely by being brighter. Below the middle, because the bars are drawn over a
// light page at a third of their opacity.
constexpr float kGroupLightness = 0.55F;

ok_color::RGB LinearFromDisplay(const ok_color::RGB& display) {
  return {.r = ok_color::srgb_transfer_function_inv(display.r),
          .g = ok_color::srgb_transfer_function_inv(display.g),
          .b = ok_color::srgb_transfer_function_inv(display.b)};
}

ok_color::RGB DisplayFromLinear(const ok_color::RGB& linear) {
  return {.r = ok_color::srgb_transfer_function(linear.r),
          .g = ok_color::srgb_transfer_function(linear.g),
          .b = ok_color::srgb_transfer_function(linear.b)};
}

bool IsGrey(const ok_color::RGB& color) {
  return color.r == color.g && color.g == color.b;
}

float SquaredDistance(const ok_color::Lab& first, const ok_color::Lab& second) {
  const float lightness = first.L - second.L;
  const float green_red = first.a - second.a;
  const float blue_yellow = first.b - second.b;

  return (lightness * lightness) + (green_red * green_red) +
         (blue_yellow * blue_yellow);
}

float Chroma(const ok_color::Lab& color) {
  return (color.a * color.a) + (color.b * color.b);
}

// What the palette carries per entry: the Oklab reading distances are measured
// on, and the sRGB triple the renderer draws. Both come out of one clip, rather
// than converting back and forth, which would drift by a few 1e-6 per turn.
struct GroupColor {
  ok_color::Lab measured;
  glm::vec3 shown;
};

// Holding hue and chroma while forcing a common lightness pushes some colors
// out of what sRGB can show — a saturated yellow has no dark form, a saturated
// blue no bright one. Clipping along the line towards the cusp keeps the hue
// and gives up chroma alone, where a channel-wise clamp would shift the hue.
GroupColor AtGroupLightness(const ok_color::Lab& color) {
  const ok_color::Lab raised{.L = kGroupLightness, .a = color.a, .b = color.b};
  const ok_color::RGB clipped = ok_color::gamut_clip_adaptive_L0_L_cusp(
      ok_color::oklab_to_linear_srgb(raised));
  const ok_color::RGB display = DisplayFromLinear(clipped);

  // The clip lands on the gamut boundary, and a float lands a millionth to
  // either side of it. Saturating that is rounding, not a color decision.
  const auto saturate = [](float channel) {
    return std::clamp(channel, 0.0F, 1.0F);
  };

  return {
      .measured = ok_color::linear_srgb_to_oklab(clipped),
      .shown = {saturate(display.r), saturate(display.g), saturate(display.b)}};
}

std::vector<GroupColor> QtColorsAtGroupLightness() {
  std::vector<GroupColor> colors;

  const auto add = [&colors](const ok_color::RGB& display) {
    if (IsGrey(display)) {
      return;
    }
    const GroupColor color = AtGroupLightness(
        ok_color::linear_srgb_to_oklab(LinearFromDisplay(display)));
    const bool known = std::ranges::any_of(colors, [&color](const auto& seen) {
      constexpr float kSameColor = 1.0e-6F;
      return SquaredDistance(seen.measured, color.measured) < kSameColor;
    });
    if (!known) {
      colors.push_back(color);
    }
  };

  for (int green = 0; green < kGreenSteps; ++green) {
    for (int red = 0; red < kRedSteps; ++red) {
      for (int blue = 0; blue < kBlueSteps; ++blue) {
        add({.r = static_cast<float>(red) / static_cast<float>(kRedSteps - 1),
             .g = static_cast<float>(green) /
                  static_cast<float>(kGreenSteps - 1),
             .b = static_cast<float>(blue) /
                  static_cast<float>(kBlueSteps - 1)});
      }
    }
  }

  for (const std::uint32_t hex : kQtGlobalColors) {
    const auto channel = [hex](unsigned int shift) {
      return static_cast<float>((hex >> shift) & kChannelMask) / kByteMaximum;
    };
    add({.r = channel(kRedShift),
         .g = channel(kGreenShift),
         .b = channel(kBlueShift)});
  }

  return colors;
}

// Order the palette so that each group sits as far from every earlier one as
// Oklab can measure: the neighbours a reader compares are the pair that has to
// stay apart, and an arbitrary order would put two blues side by side.
std::vector<glm::vec3> OrderedByPerceptualDistance() {
  std::vector<GroupColor> remaining = QtColorsAtGroupLightness();
  std::vector<ok_color::Lab> chosen;
  std::vector<glm::vec3> ordered;

  auto next = std::ranges::max_element(
      remaining, {}, [](const auto& color) { return Chroma(color.measured); });

  while (next != remaining.end()) {
    chosen.push_back(next->measured);
    ordered.push_back(next->shown);
    remaining.erase(next);

    next = std::ranges::max_element(
        remaining, {}, [&chosen](const auto& candidate) {
          float nearest = std::numeric_limits<float>::max();
          for (const auto& taken : chosen) {
            nearest =
                std::min(nearest, SquaredDistance(candidate.measured, taken));
          }
          return nearest;
        });
  }

  return ordered;
}

}  // namespace

glm::vec3 CategoricalColor(std::size_t index) {
  static const std::vector<glm::vec3> palette = OrderedByPerceptualDistance();

  return palette.at(index % palette.size());
}

}  // namespace palette
