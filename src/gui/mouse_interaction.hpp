#ifndef MOUSE_INTERACTION_HPP
#define MOUSE_INTERACTION_HPP

#include <epoxy/gl.h>
#include <wx/gdicmn.h>

#include <array>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "../graphics/debug_log.hpp"
#include "../graphics/mvp_matrices.hpp"

// Übersetzt Mausbewegungen (Ziehen und Mausrad) in Pan- und Zoom-Operationen
// auf der View-Matrix der MVP. Der Zoom bleibt dabei auf den Mauszeiger
// zentriert, indem die durch die Skalierung entstandene Verschiebung
// korrigiert wird.
class MouseInteraction {
 public:
  MouseInteraction()
      : persistent_mouse_pos_(0.F),
        translate_pre_scaled_(0.F),
        translate_post_scaled_(0.F) {}

  void Apply(MVP& mvp, wxPoint mouse_position, bool dragging,
             int wheel_rotation) {
    glm::vec3 const current_mouse_pos = MouseWorldSpacePos(mouse_position, mvp);
    LogPointerInput(mouse_position, dragging, wheel_rotation,
                    current_mouse_pos);

    // Reihenfolge tragend: der Zoom liest den Pan-Zustand über
    // CalculateViewMatrix, Pan muss also zuerst laufen.
    if (dragging) {
      Pan(current_mouse_pos);
    }
    if (wheel_rotation != 0) {
      ZoomAroundMouse(current_mouse_pos, wheel_rotation);
    }

    const auto view_matrix = CalculateViewMatrix(persistent_scale_factor_);
    mvp.SetView(view_matrix);
    LogViewState(view_matrix);

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
  // Verschiebt die Seite um die Mausbewegung seit dem letzten Ereignis.
  void Pan(const glm::vec3& current_mouse_pos) {
    translate_pre_scaled_ += current_mouse_pos - persistent_mouse_pos_;
  }

  // Skaliert um den Mauszeiger: die Verschiebung, die der Punkt unter dem
  // Zeiger durch die Skalierung erfährt, wird zurückkorrigiert.
  void ZoomAroundMouse(const glm::vec3& current_mouse_pos, int wheel_rotation) {
    const auto scale = static_cast<float>(wheel_rotation) / kMouseWheelStep;

    const auto pre_scale_view_matrix =
        CalculateViewMatrix(persistent_scale_factor_);
    const auto pre_scale_mouse_pos =
        MouseViewSpacePos(current_mouse_pos, pre_scale_view_matrix);

    persistent_scale_factor_ *= std::exp(scale);

    const auto post_scale_view_matrix =
        CalculateViewMatrix(persistent_scale_factor_);
    const auto post_scale_mouse_pos =
        MouseViewSpacePos(current_mouse_pos, post_scale_view_matrix);

    const glm::vec3 view_space_correction =
        post_scale_mouse_pos - pre_scale_mouse_pos;
    translate_post_scaled_ += view_space_correction;
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

  void LogViewState(const glm::mat4& view_matrix) const {
    if (!decade_debug::LogEnabled()) {
      return;
    }
    decade_debug::LogVec3("translate_pre_scaled", translate_pre_scaled_);
    decade_debug::LogVec3("translate_post_scaled", translate_post_scaled_);
    std::cout << "scale=" << persistent_scale_factor_ << '\n';
    decade_debug::LogMat4("view_matrix", view_matrix);
  }

  glm::mat4 CalculateViewMatrix(float scale_factor) {
    auto pre_scaled = glm::translate(glm::mat4(1.F), translate_pre_scaled_);
    auto post_scaled =
        glm::scale(pre_scaled, glm::vec3(scale_factor, scale_factor, 1.F));
    auto view_matrix = glm::translate(post_scaled, translate_post_scaled_);
    return view_matrix;
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

  float persistent_scale_factor_{1.F};
  glm::vec3 persistent_mouse_pos_;
  glm::vec3 translate_pre_scaled_;
  glm::vec3 translate_post_scaled_;
};

#endif  // MOUSE_INTERACTION_HPP
