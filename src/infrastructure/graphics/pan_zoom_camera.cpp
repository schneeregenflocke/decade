#include "pan_zoom_camera.hpp"

#include <algorithm>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/matrix.hpp>

void PanZoomCamera::Pan(const glm::vec3& world_delta) {
  translate_pre_scaled_ += world_delta;
}

void PanZoomCamera::ZoomAround(const glm::vec3& world_pos, float factor) {
  const float target_scale =
      std::clamp(scale_factor_ * factor, limits_.min_scale, limits_.max_scale);

  const glm::vec3 pre_scale_page_pos = PagePos(world_pos);
  scale_factor_ = target_scale;
  const glm::vec3 post_scale_page_pos = PagePos(world_pos);

  translate_post_scaled_ += post_scale_page_pos - pre_scale_page_pos;
}

void PanZoomCamera::SetScaleLimits(const ScaleLimits& limits) {
  limits_ = limits;
}

float PanZoomCamera::ScaleFactor() const { return scale_factor_; }

glm::mat4 PanZoomCamera::ViewMatrix() const {
  const auto pre_scaled = glm::translate(glm::mat4(1.F), translate_pre_scaled_);
  const auto scaled =
      glm::scale(pre_scaled, glm::vec3(scale_factor_, scale_factor_, 1.F));
  return glm::translate(scaled, translate_post_scaled_);
}

glm::vec3 PanZoomCamera::PagePos(const glm::vec3& world_pos) const {
  const auto page_pos = glm::inverse(ViewMatrix()) * glm::vec4(world_pos, 1.F);
  return {page_pos.x, page_pos.y, 0.F};
}

PanZoomCamera::ScaleLimits ComputeZoomLimits(const glm::mat4& ortho_projection,
                                             const glm::vec2& page_size_mm,
                                             float export_dpi) {
  // glm::ortho puts 2/(right-left) into [0][0] and 2/(top-bottom) into [1][1] —
  // from those, compute back the world extent visible at scale 1.
  const float visible_width = 2.F / ortho_projection[0][0];
  const float visible_height = 2.F / ortho_projection[1][1];

  constexpr float kMinVisibleExportPixels = 2.F;
  constexpr float kMmPerInch = 25.4F;
  const float min_visible_mm =
      kMinVisibleExportPixels * kMmPerInch / export_dpi;
  const float max_scale =
      std::min(visible_width, visible_height) / min_visible_mm;

  constexpr float kMaxVisiblePages = 2.25F;
  const float min_scale =
      std::min(visible_width / (kMaxVisiblePages * page_size_mm.x),
               visible_height / (kMaxVisiblePages * page_size_mm.y));

  // Degenerate geometry (a tiny window) must not deliver an inverted interval —
  // std::clamp demands min <= max.
  return {.min_scale = min_scale, .max_scale = std::max(max_scale, min_scale)};
}
