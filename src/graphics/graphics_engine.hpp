#ifndef GRAPHICS_ENGINE_HPP
#define GRAPHICS_ENGINE_HPP

#include <memory>
#include <optional>
#include <string>
#include <tinycolormap.hpp>

#include "mvp_matrices.hpp"
#include "scene_graph.hpp"
#include "shaders.hpp"

class GraphicsEngine {
 public:
  // Background fill of the GL framebuffer: a dark grey.
  static constexpr tinycolormap::Color kClearColor{0.2};

  void Render() {
    glClearColor(static_cast<GLfloat>(kClearColor.r()),
                 static_cast<GLfloat>(kClearColor.g()),
                 static_cast<GLfloat>(kClearColor.b()), 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (size_t index = 0; index < shaders_.GetNumberShaders(); ++index) {
      auto& shader = *shaders_.GetShader(index);

      shader.UseProgram();
      shader.SetUniform("projection", mvp_.GetProjection());
      shader.SetUniform("view", mvp_.GetView());
      shader.SetUniform("model", glm::mat4(1.F));
    }

    scene_graph_->draw();
  }

  void SetMVP(const MVP& new_mvp) { mvp_ = new_mvp; }

  void set_scene_graph(const std::shared_ptr<SceneNode>& new_scene_graph) {
    scene_graph_ = new_scene_graph;
  }

  Shaders& GetShaders() { return shaders_; }

  std::optional<Shader*> search_shader(const std::string& search_name) {
    return shaders_.search_shader(search_name);
  }

 private:
  MVP mvp_;
  Shaders shaders_;
  std::shared_ptr<SceneNode> scene_graph_;
};
#endif  // GRAPHICS_ENGINE_HPP
