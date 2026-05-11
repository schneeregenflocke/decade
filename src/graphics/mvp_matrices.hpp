#ifndef HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_MVP_MATRICES_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_MVP_MATRICES_HPP

#include <glm/glm.hpp>

class MVP {
public:
  MVP() : projection(1.0F), view(1.0F), model(1.0F) {}

  void SetProjection(const glm::mat4 &new_projection) { projection = new_projection; }

  void SetView(const glm::mat4 &new_view) { view = new_view; }

  [[nodiscard]] glm::mat4 GetProjection() const { return projection; }

  [[nodiscard]] glm::mat4 GetView() const { return view; }

private:
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 model;
};
#endif // HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_MVP_MATRICES_HPP
