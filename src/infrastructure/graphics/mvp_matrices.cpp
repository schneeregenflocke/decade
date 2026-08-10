#include "mvp_matrices.hpp"

#include <glm/ext/matrix_float4x4.hpp>

MVP::MVP() : projection_(1.0F), view_(1.0F) {}

void MVP::SetProjection(const glm::mat4& new_projection) {
  projection_ = new_projection;
}

void MVP::SetView(const glm::mat4& new_view) { view_ = new_view; }

glm::mat4 MVP::GetProjection() const { return projection_; }

glm::mat4 MVP::GetView() const { return view_; }
