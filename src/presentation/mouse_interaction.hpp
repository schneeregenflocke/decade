#ifndef MOUSE_INTERACTION_HPP
#define MOUSE_INTERACTION_HPP

#include <epoxy/gl.h>
#include <wx/gdicmn.h>

#include <array>
#include <cmath>
#include <glm/glm.hpp>
#include <iostream>

#include "../common/debug_log.hpp"
#include "../infrastructure/graphics/mvp_matrices.hpp"
#include "../infrastructure/graphics/pan_zoom_camera.hpp"

// Übersetzt Mausereignisse in Kamera-Befehle: Ziehen wird zum Pan-Delta im
// Weltraum, das Mausrad zum Zoom-Faktor um den Zeiger. Der Pan-/Zoom-Zustand
// selbst lebt in PanZoomCamera.
class MouseInteraction {
 public:
  void Apply(MVP& mvp, PanZoomCamera& camera, wxPoint mouse_position,
             bool dragging, int wheel_rotation) {
    glm::vec3 const current_mouse_pos = MouseWorldSpacePos(mouse_position, mvp);
    LogPointerInput(mouse_position, dragging, wheel_rotation,
                    current_mouse_pos);

    // Reihenfolge tragend: der Zoom korrigiert über die View-Matrix, die den
    // aktuellen Pan-Zustand enthält — Pan muss also zuerst laufen.
    if (dragging) {
      camera.Pan(current_mouse_pos - persistent_mouse_pos_);
    }
    if (wheel_rotation != 0) {
      camera.ZoomAround(current_mouse_pos, WheelZoomFactor(wheel_rotation));
    }

    mvp.SetView(camera.ViewMatrix());
    LogViewState(camera, mvp.GetView());

    persistent_mouse_pos_ = current_mouse_pos;
  }

  // Unprojects a pixel position to page/world space (the space in which the
  // scene graph places geometry), inverting both projection and view so the
  // result lines up with the bars' page-space rectangles for hit-testing.
  static glm::vec2 ScreenToPage(const wxPoint& mouse_pos_px, const MVP& mvp) {
    const glm::vec3 view_space = MouseWorldSpacePos(mouse_pos_px, mvp);
    const glm::vec3 page = MouseViewSpacePos(view_space, mvp.GetView());
    return {page.x, page.y};
  }

 private:
  static float WheelZoomFactor(int wheel_rotation) {
    return std::exp(static_cast<float>(wheel_rotation) / kMouseWheelStep);
  }

  void LogPointerInput(wxPoint mouse_position, bool dragging,
                       int wheel_rotation,
                       const glm::vec3& current_mouse_pos) const {
    if (!decade_debug::LogEnabled()) {
      return;
    }
    std::cout << "Mouse: px=(" << mouse_position.x << "," << mouse_position.y
              << ") drag=" << dragging << " wheel=" << wheel_rotation << '\n';
    decade_debug::LogVec3("Mouse current (view-space)", current_mouse_pos);
    decade_debug::LogVec3("Mouse persistent (prev)", persistent_mouse_pos_);
  }

  static void LogViewState(const PanZoomCamera& camera,
                           const glm::mat4& view_matrix) {
    if (!decade_debug::LogEnabled()) {
      return;
    }
    std::cout << "scale=" << camera.ScaleFactor() << '\n';
    decade_debug::LogMat4("view_matrix", view_matrix);
  }

  static glm::vec3 MouseClipSpace(const wxPoint& mouse_pos_px) {
    const glm::vec2 window_mouse_pos(static_cast<float>(mouse_pos_px.x),
                                     static_cast<float>(mouse_pos_px.y));

    std::array<GLint, 4> viewport_px{};
    glGetIntegerv(GL_VIEWPORT, viewport_px.data());
    glm::vec4 const viewport(0.F, 0.F, static_cast<float>(viewport_px[2]),
                             static_cast<float>(viewport_px[3]));

    auto viewport_ortho =
        glm::ortho(viewport.x, viewport.z, viewport.w, viewport.y);
    auto mouse_pos_clip_space =
        viewport_ortho * glm::vec4(window_mouse_pos, 0.F, 1.F);
    return mouse_pos_clip_space;
  }

  static glm::vec3 MouseWorldSpacePos(const wxPoint& mouse_pos_px,
                                      const MVP& mvp) {
    const glm::mat4 projection_matrix = mvp.GetProjection();
    const auto inverse_projection_matrix = glm::inverse(projection_matrix);

    const auto mouse_pos = inverse_projection_matrix *
                           glm::vec4(MouseClipSpace(mouse_pos_px), 1.F);
    return {mouse_pos.x, mouse_pos.y, 0.F};
  }

  static glm::vec3 MouseViewSpacePos(const glm::vec3& mouse_world_space_pos,
                                     const glm::mat4& view_matrix) {
    const auto inverse_view_matrix = glm::inverse(view_matrix);

    const auto mouse_pos =
        inverse_view_matrix * glm::vec4(mouse_world_space_pos, 1.F);
    return {mouse_pos.x, mouse_pos.y, 0.F};
  }

  static constexpr float kMouseWheelStep = 1200.F;

  glm::vec3 persistent_mouse_pos_{0.F};
};

#endif  // MOUSE_INTERACTION_HPP
