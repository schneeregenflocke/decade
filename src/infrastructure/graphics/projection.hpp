#ifndef PROJECTION_HPP
#define PROJECTION_HPP

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

  static glm::mat4 OrthoMatrix(const RectF& view_size, float aspect_ratio) {
    const auto page_height_ratio = view_size.Width() / view_size.Height();

    if (page_height_ratio >= aspect_ratio) {
      return OrthoMatrixWidth(view_size.Width(), aspect_ratio);
    }
    return OrthoMatrixHeight(view_size.Height(), aspect_ratio);
  }

  static glm::mat4 OrthoMatrixWidth(float width, float aspect_ratio) {
    constexpr float kHalf = 0.5F;
    const float x_half_size = width * kHalf;
    const float y_half_size = width / aspect_ratio * kHalf;
    return glm::ortho(-x_half_size, x_half_size, -y_half_size, y_half_size);
  }

  static glm::mat4 OrthoMatrixHeight(float height, float aspect_ratio) {
    constexpr float kHalf = 0.5F;
    const float x_half_size = height * aspect_ratio * kHalf;
    const float y_half_size = height * kHalf;
    return glm::ortho(-x_half_size, x_half_size, -y_half_size, y_half_size);
  }
};
#endif  // PROJECTION_HPP
