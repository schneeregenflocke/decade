#ifndef MVP_MATRICES_HPP
#define MVP_MATRICES_HPP

#include <glm/glm.hpp>

class MVP {
 public:
  MVP() : projection_(1.0F), view_(1.0F), model_(1.0F) {}

  void SetProjection(const glm::mat4& new_projection) {
    projection_ = new_projection;
  }

  void SetView(const glm::mat4& new_view) { view_ = new_view; }

  [[nodiscard]] glm::mat4 GetProjection() const { return projection_; }

  [[nodiscard]] glm::mat4 GetView() const { return view_; }

 private:
  glm::mat4 projection_;
  glm::mat4 view_;
  glm::mat4 model_;
};
#endif  // MVP_MATRICES_HPP
