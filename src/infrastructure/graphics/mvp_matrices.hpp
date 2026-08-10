#ifndef MVP_MATRICES_HPP
#define MVP_MATRICES_HPP

#include <glm/mat4x4.hpp>

class MVP {
 public:
  MVP();

  void SetProjection(const glm::mat4& new_projection);

  void SetView(const glm::mat4& new_view);

  [[nodiscard]] glm::mat4 GetProjection() const;

  [[nodiscard]] glm::mat4 GetView() const;

 private:
  glm::mat4 projection_;
  glm::mat4 view_;
};
#endif  // MVP_MATRICES_HPP
