#ifndef CASTS_HPP
#define CASTS_HPP

#include <wx/colour.h>

#include <glm/vec4.hpp>

inline glm::vec4 to_glm_vec4(const wxColour& color) {
  constexpr float kColorScale = 1.0F / 255.0F;

  const auto red = static_cast<float>(color.Red()) * kColorScale;
  const auto green = static_cast<float>(color.Green()) * kColorScale;
  const auto blue = static_cast<float>(color.Blue()) * kColorScale;
  const auto alpha = static_cast<float>(color.Alpha()) * kColorScale;

  return {red, green, blue, alpha};
}

inline wxColour to_wx_color(const glm::vec4& color) {
  constexpr float kColorMax = 255.0F;
  const auto red = static_cast<unsigned char>(color[0] * kColorMax);
  const auto green = static_cast<unsigned char>(color[1] * kColorMax);
  const auto blue = static_cast<unsigned char>(color[2] * kColorMax);
  const auto alpha = static_cast<unsigned char>(color[3] * kColorMax);

  return {red, green, blue, alpha};
}
#endif  // CASTS_HPP
