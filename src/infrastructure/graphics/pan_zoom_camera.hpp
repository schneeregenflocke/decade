#ifndef PAN_ZOOM_CAMERA_HPP
#define PAN_ZOOM_CAMERA_HPP

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
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
  void Pan(const glm::vec3& world_delta);

  // Scales by factor (> 1 enlarges), bounded by the ScaleLimits. The world
  // point world_pos (typically the mouse pointer) stays at the same image
  // position, by correcting back the shift the scaling produced.
  void ZoomAround(const glm::vec3& world_pos, float factor);

  // Takes hold at the next ZoomAround alone; a state already outside does not
  // get snapped back.
  void SetScaleLimits(const ScaleLimits& limits);

  [[nodiscard]] float ScaleFactor() const;

  [[nodiscard]] glm::mat4 ViewMatrix() const;

  // Converts a world point back into page space (the inverse view matrix).
  [[nodiscard]] glm::vec3 PagePos(const glm::vec3& world_pos) const;

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
PanZoomCamera::ScaleLimits ComputeZoomLimits(const glm::mat4& ortho_projection,
                                             const glm::vec2& page_size_mm,
                                             float export_dpi);

#endif  // PAN_ZOOM_CAMERA_HPP
