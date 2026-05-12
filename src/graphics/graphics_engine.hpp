#ifndef HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_GRAPHICS_ENGINE_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_GRAPHICS_ENGINE_HPP

#include <memory>
#include <optional>
#include <string>

#include "mvp_matrices.hpp"
#include "scene_graph.hpp"
#include "shaders.hpp"

class GraphicsEngine {
 public:
  void Render() {
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (size_t index = 0; index < shaders.GetNumberShaders(); ++index) {
      auto& shader = *shaders.GetShader(index);

      shader.UseProgram();
      shader.SetUniform("projection", mvp.GetProjection());
      shader.SetUniform("view", mvp.GetView());
      shader.SetUniform("model", glm::mat4(1.F));
    }

    scene_graph->draw();
  }

  void SetMVP(const MVP& new_mvp) { mvp = new_mvp; }

  void set_scene_graph(const std::shared_ptr<SceneNode>& new_scene_graph) {
    scene_graph = new_scene_graph;
  }

  Shaders& GetShaders() { return shaders; }

  std::optional<Shader*> search_shader(const std::string& search_name) {
    return shaders.search_shader(search_name);
  }

 private:
  MVP mvp;
  Shaders shaders;
  std::shared_ptr<SceneNode> scene_graph;
};
#endif  // HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_GRAPHICS_ENGINE_HPP
