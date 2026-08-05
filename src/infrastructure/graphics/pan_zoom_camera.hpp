#ifndef PAN_ZOOM_CAMERA_HPP
#define PAN_ZOOM_CAMERA_HPP

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>

// The 2D camera of the page view: it holds the pan and zoom state and derives
// the view matrix from it. Free of GL and UI, hence unit-testable; the mouse
// side (pixels → world space) sits in presentation/mouse_interaction.hpp.
class PanZoomCamera {
 public:
  // The permitted range of the scale factor; see ComputeZoomLimits.
  struct ScaleLimits {
    float min_scale{0.F};
    float max_scale{std::numeric_limits<float>::max()};
  };

  // Shifts the view by a delta in world space.
  void Pan(const glm::vec3& world_delta) { translate_pre_scaled_ += world_delta; }

  // Scales by factor (> 1 enlarges), bounded by the ScaleLimits. The world point
  // world_pos (typically the mouse pointer) stays at the same image position,
  // by correcting back the shift the scaling produced.
  void ZoomAround(const glm::vec3& world_pos, float factor) {
    const float target_scale = std::clamp(scale_factor_ * factor,
                                          limits_.min_scale, limits_.max_scale);

    const glm::vec3 pre_scale_page_pos = PagePos(world_pos);
    scale_factor_ = target_scale;
    const glm::vec3 post_scale_page_pos = PagePos(world_pos);

    translate_post_scaled_ += post_scale_page_pos - pre_scale_page_pos;
  }

  // Takes hold at the next ZoomAround alone; a state already outside does not
  // get snapped back.
  void SetScaleLimits(const ScaleLimits& limits) { limits_ = limits; }

  [[nodiscard]] float ScaleFactor() const { return scale_factor_; }

  [[nodiscard]] glm::mat4 ViewMatrix() const {
    const auto pre_scaled =
        glm::translate(glm::mat4(1.F), translate_pre_scaled_);
    const auto scaled =
        glm::scale(pre_scaled, glm::vec3(scale_factor_, scale_factor_, 1.F));
    return glm::translate(scaled, translate_post_scaled_);
  }

  // Converts a world point back into page space (the inverse view matrix).
  [[nodiscard]] glm::vec3 PagePos(const glm::vec3& world_pos) const {
    const auto page_pos = glm::inverse(ViewMatrix()) * glm::vec4(world_pos, 1.F);
    return {page_pos.x, page_pos.y, 0.F};
  }

 private:
  float scale_factor_{1.F};
  glm::vec3 translate_pre_scaled_{0.F};
  glm::vec3 translate_post_scaled_{0.F};
  ScaleLimits limits_;
};

// Derives the zoom bounds from the ortho projection, the page size (mm) and the
// export resolution:
// - max_scale: zooming in ends once at least 2 pixels of the export image
//   (export_dpi) stay visible in each direction.
// - min_scale: zooming out ends once 2 pages plus 25 % of the page measure are
//   visible in width and height each.
inline PanZoomCamera::ScaleLimits ComputeZoomLimits(
    const glm::mat4& ortho_projection, const glm::vec2& page_size_mm,
    float export_dpi) {
  // glm::ortho puts 2/(right-left) into [0][0] and 2/(top-bottom) into [1][1] —
  // from those, compute back the world extent visible at scale 1.
  const float visible_width = 2.F / ortho_projection[0][0];
  const float visible_height = 2.F / ortho_projection[1][1];

  constexpr float kMinVisibleExportPixels = 2.F;
  constexpr float kMmPerInch = 25.4F;
  const float min_visible_mm = kMinVisibleExportPixels * kMmPerInch / export_dpi;
  const float max_scale = std::min(visible_width, visible_height) / min_visible_mm;

  constexpr float kMaxVisiblePages = 2.25F;
  const float min_scale =
      std::min(visible_width / (kMaxVisiblePages * page_size_mm.x),
               visible_height / (kMaxVisiblePages * page_size_mm.y));

  // Degenerate geometry (a tiny window) must not deliver an inverted interval —
  // std::clamp demands min <= max.
  return {.min_scale = min_scale, .max_scale = std::max(max_scale, min_scale)};
}

#endif  // PAN_ZOOM_CAMERA_HPP
