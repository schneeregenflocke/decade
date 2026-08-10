#include "casts.hpp"

#include <QtGui/QColor>
#include <glm/ext/vector_float4.hpp>

glm::vec4 ToGlmVec4(const QColor& color) {
  constexpr float kColorScale = 1.0F / 255.0F;

  const auto red = static_cast<float>(color.red()) * kColorScale;
  const auto green = static_cast<float>(color.green()) * kColorScale;
  const auto blue = static_cast<float>(color.blue()) * kColorScale;
  const auto alpha = static_cast<float>(color.alpha()) * kColorScale;

  return {red, green, blue, alpha};
}

QColor ToQColor(const glm::vec4& color) {
  constexpr float kColorMax = 255.0F;
  const auto red = static_cast<int>(color[0] * kColorMax);
  const auto green = static_cast<int>(color[1] * kColorMax);
  const auto blue = static_cast<int>(color[2] * kColorMax);
  const auto alpha = static_cast<int>(color[3] * kColorMax);

  return {red, green, blue, alpha};
}
