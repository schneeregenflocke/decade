#include "projection.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>

#include "rect.hpp"

glm::mat4 Projection::OrthoMatrix(const RectF& view_size, float aspect_ratio) {
  const auto page_height_ratio = view_size.Width() / view_size.Height();

  if (page_height_ratio >= aspect_ratio) {
    return OrthoMatrixWidth(view_size.Width(), aspect_ratio);
  }
  return OrthoMatrixHeight(view_size.Height(), aspect_ratio);
}

glm::mat4 Projection::OrthoMatrixWidth(float width, float aspect_ratio) {
  constexpr float kHalf = 0.5F;
  const float x_half_size = width * kHalf;
  const float y_half_size = width / aspect_ratio * kHalf;
  return glm::ortho(-x_half_size, x_half_size, -y_half_size, y_half_size);
}

glm::mat4 Projection::OrthoMatrixHeight(float height, float aspect_ratio) {
  constexpr float kHalf = 0.5F;
  const float x_half_size = height * aspect_ratio * kHalf;
  const float y_half_size = height * kHalf;
  return glm::ortho(-x_half_size, x_half_size, -y_half_size, y_half_size);
}
