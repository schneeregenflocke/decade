#ifndef MOUSE_INTERACTION_HPP
#define MOUSE_INTERACTION_HPP

#include <cmath>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <iostream>

#include "../common/debug_log.hpp"
#include "../infrastructure/graphics/mvp_matrices.hpp"
#include "../infrastructure/graphics/pan_zoom_camera.hpp"

// Translates mouse events into camera commands: dragging becomes a pan delta in
// world space, the mouse wheel a zoom factor around the pointer. The pan and
// zoom state itself lives in PanZoomCamera.
//
// The viewport travels in as a parameter rather than being read back with
// glGetIntegerv(GL_VIEWPORT). This runs in input handlers, where no GL context
// is current — a QOpenGLWidget makes its context current for the three
// rendering callbacks alone — so the query would answer out of nowhere. The
// canvas knows its own framebuffer size anyway, which leaves this class free of
// GL and of any widget toolkit.
class MouseInteraction {
 public:
  void Apply(MVP& mvp, PanZoomCamera& camera, glm::ivec2 mouse_position,
             glm::ivec2 viewport_size, bool dragging, int wheel_rotation);

  // Unprojects a pixel position to page/world space (the space in which the
  // scene graph places geometry), inverting both projection and view so the
  // result lines up with the bars' page-space rectangles for hit-testing.
  static glm::vec2 ScreenToPage(const glm::ivec2& mouse_pos_px,
                                const glm::ivec2& viewport_size,
                                const MVP& mvp);

 private:
  static float WheelZoomFactor(int wheel_rotation);

  void LogPointerInput(glm::ivec2 mouse_position, bool dragging,
                       int wheel_rotation,
                       const glm::vec3& current_mouse_pos) const;

  static void LogViewState(const PanZoomCamera& camera,
                           const glm::mat4& view_matrix);

  static glm::vec3 MouseClipSpace(const glm::ivec2& mouse_pos_px,
                                  const glm::ivec2& viewport_size);

  static glm::vec3 MouseWorldSpacePos(const glm::ivec2& mouse_pos_px,
                                      const glm::ivec2& viewport_size,
                                      const MVP& mvp);

  static glm::vec3 MouseViewSpacePos(const glm::vec3& mouse_world_space_pos,
                                     const glm::mat4& view_matrix);

  static constexpr float kMouseWheelStep = 1200.F;

  glm::vec3 persistent_mouse_pos_{0.F};
};

#endif  // MOUSE_INTERACTION_HPP
