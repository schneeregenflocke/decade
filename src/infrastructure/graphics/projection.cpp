#include "projection.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>

#include "rect.hpp"

glm::mat4 Projection::OrthoMatrix(const RectF& view_size, float aspect_ratio) {
  constexpr float kHalf = 0.5F;
  const auto page_height_ratio = view_size.Width() / view_size.Height();
  const bool fits_width = page_height_ratio >= aspect_ratio;
  const float x_half_size =
      kHalf *
      (fits_width ? view_size.Width() : view_size.Height() * aspect_ratio);
  const float y_half_size =
      kHalf *
      (fits_width ? view_size.Width() / aspect_ratio : view_size.Height());
  const glm::vec3 center = view_size.Center();
  return glm::ortho(center.x - x_half_size, center.x + x_half_size,
                    center.y - y_half_size, center.y + y_half_size);
}
