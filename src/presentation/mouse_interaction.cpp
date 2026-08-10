#include "mouse_interaction.hpp"

#include <cmath>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/matrix.hpp>
#include <iostream>

#include "../common/debug_log.hpp"
#include "../infrastructure/graphics/mvp_matrices.hpp"
#include "../infrastructure/graphics/pan_zoom_camera.hpp"

void MouseInteraction::Apply(MVP& mvp, PanZoomCamera& camera,
                             glm::ivec2 mouse_position,
                             glm::ivec2 viewport_size, bool dragging,
                             int wheel_rotation) {
  glm::vec3 const current_mouse_pos =
      MouseWorldSpacePos(mouse_position, viewport_size, mvp);
  LogPointerInput(mouse_position, dragging, wheel_rotation, current_mouse_pos);

  // The order is load-bearing: the zoom corrects through the view matrix,
  // which holds the current pan state — so the pan must run first.
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

glm::vec2 MouseInteraction::ScreenToPage(const glm::ivec2& mouse_pos_px,
                                         const glm::ivec2& viewport_size,
                                         const MVP& mvp) {
  const glm::vec3 view_space =
      MouseWorldSpacePos(mouse_pos_px, viewport_size, mvp);
  const glm::vec3 page = MouseViewSpacePos(view_space, mvp.GetView());
  return {page.x, page.y};
}

float MouseInteraction::WheelZoomFactor(int wheel_rotation) {
  return std::exp(static_cast<float>(wheel_rotation) / kMouseWheelStep);
}

void MouseInteraction::LogPointerInput(
    glm::ivec2 mouse_position, bool dragging, int wheel_rotation,
    const glm::vec3& current_mouse_pos) const {
  if (!decade_debug::LogEnabled()) {
    return;
  }
  std::cout << "Mouse: px=(" << mouse_position.x << "," << mouse_position.y
            << ") drag=" << dragging << " wheel=" << wheel_rotation << '\n';
  decade_debug::LogVec3("Mouse current (view-space)", current_mouse_pos);
  decade_debug::LogVec3("Mouse persistent (prev)", persistent_mouse_pos_);
}

void MouseInteraction::LogViewState(const PanZoomCamera& camera,
                                    const glm::mat4& view_matrix) {
  if (!decade_debug::LogEnabled()) {
    return;
  }
  std::cout << "scale=" << camera.ScaleFactor() << '\n';
  decade_debug::LogMat4("view_matrix", view_matrix);
}

glm::vec3 MouseInteraction::MouseClipSpace(const glm::ivec2& mouse_pos_px,
                                           const glm::ivec2& viewport_size) {
  const glm::vec2 window_mouse_pos(static_cast<float>(mouse_pos_px.x),
                                   static_cast<float>(mouse_pos_px.y));

  // Top-left origin: the pointer counts downwards, clip space upwards, which
  // is why bottom and top stand swapped in the ortho call.
  const auto viewport_ortho =
      glm::ortho(0.0F, static_cast<float>(viewport_size.x),
                 static_cast<float>(viewport_size.y), 0.0F);
  auto mouse_pos_clip_space =
      viewport_ortho * glm::vec4(window_mouse_pos, 0.F, 1.F);
  return mouse_pos_clip_space;
}

glm::vec3 MouseInteraction::MouseWorldSpacePos(const glm::ivec2& mouse_pos_px,
                                               const glm::ivec2& viewport_size,
                                               const MVP& mvp) {
  const glm::mat4 projection_matrix = mvp.GetProjection();
  const auto inverse_projection_matrix = glm::inverse(projection_matrix);

  const auto mouse_pos =
      inverse_projection_matrix *
      glm::vec4(MouseClipSpace(mouse_pos_px, viewport_size), 1.F);
  return {mouse_pos.x, mouse_pos.y, 0.F};
}

glm::vec3 MouseInteraction::MouseViewSpacePos(
    const glm::vec3& mouse_world_space_pos, const glm::mat4& view_matrix) {
  const auto inverse_view_matrix = glm::inverse(view_matrix);

  const auto mouse_pos =
      inverse_view_matrix * glm::vec4(mouse_world_space_pos, 1.F);
  return {mouse_pos.x, mouse_pos.y, 0.F};
}
