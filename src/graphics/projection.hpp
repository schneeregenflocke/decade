#ifndef HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_PROJECTION_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_PROJECTION_HPP

#include "rect.hpp"

#include <epoxy/gl.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>

// #include <iostream>

class Projection {
public:
  static float AspectRatio()
  {
    std::array<GLint, 4> viewport{};
    glGetIntegerv(GL_VIEWPORT, viewport.data());

    const auto width = static_cast<float>(viewport[2]);
    const auto height = static_cast<float>(viewport[3]);

    return width / height;
  }

  static glm::mat4 OrthoMatrix(const rectf &view_size)
  {
    const auto page_height_ratio = view_size.width() / view_size.height();
    const auto viewport_height_ratio = AspectRatio();

    glm::mat4 ortho_matrix;
    if (page_height_ratio >= viewport_height_ratio) {
      ortho_matrix = OrthoMatrixWidth(view_size.width());
    } else {
      ortho_matrix = OrthoMatrixHeight(view_size.height());
    }

    return ortho_matrix;
  }

  static glm::mat4 PerspectiveMatrix(const float fovy, const float z_near, const float z_far)
  {
    return glm::perspective(fovy, AspectRatio(), z_near, z_far);
  }

  static glm::mat4 OrthoMatrixWidth(float width)
  {
    constexpr float kHalf = 0.5F;
    const float x_half_size = width * kHalf;
    const float y_half_size = width / AspectRatio() * kHalf;
    return glm::ortho(-x_half_size, x_half_size, -y_half_size, y_half_size);
  }

  static glm::mat4 OrthoMatrixHeight(float height)
  {
    constexpr float kHalf = 0.5F;
    const float x_half_size = height * AspectRatio() * kHalf;
    const float y_half_size = height * kHalf;
    return glm::ortho(-x_half_size, x_half_size, -y_half_size, y_half_size);
  }
};
#endif // HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_PROJECTION_HPP
