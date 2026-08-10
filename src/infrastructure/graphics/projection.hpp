#ifndef PROJECTION_HPP
#define PROJECTION_HPP

#include <glm/mat4x4.hpp>

#include "rect.hpp"

// The orthographic projection that fits the page into the window.
//
// The aspect ratio travels in as a parameter. It used to be read back with
// glGetIntegerv(GL_VIEWPORT), which tied the whole class to a current GL
// context and, worse, to the viewport GL happened to hold at that moment: the
// projection gets recomputed when the page or the window changes, and neither
// of those moments is a draw. The caller knows its framebuffer size anyway,
// which leaves this class pure — no GL, no state, testable.
class Projection {
 public:
  // A degenerate viewport (height or width 0 on a window pulled extremely flat)
  // would otherwise carry inf or NaN through OrthoMatrix and ComputeZoomLimits
  // into the whole rendering — and stay there until the next valid resize. 1:1
  // keeps the matrices finite.
  static constexpr float kDegenerateAspectRatio = 1.0F;

  [[nodiscard]] static constexpr float AspectRatioOf(int width, int height) {
    if (width <= 0 || height <= 0) {
      return kDegenerateAspectRatio;
    }
    return static_cast<float>(width) / static_cast<float>(height);
  }

  static glm::mat4 OrthoMatrix(const RectF& view_size, float aspect_ratio);

  static glm::mat4 OrthoMatrixWidth(float width, float aspect_ratio);

  static glm::mat4 OrthoMatrixHeight(float height, float aspect_ratio);
};
#endif  // PROJECTION_HPP
