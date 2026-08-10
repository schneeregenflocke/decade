#include "graphics_engine.hpp"

#include <epoxy/gl.h>

#include <cstddef>
#include <functional>
#include <optional>
#include <string>

#include "mvp_matrices.hpp"
#include "scene.hpp"
#include "shaders.hpp"

void GraphicsEngine::Render() {
  glClearColor(static_cast<GLfloat>(kClearColor.r()),
               static_cast<GLfloat>(kClearColor.g()),
               static_cast<GLfloat>(kClearColor.b()), 1.0F);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Camera uniforms (projection/view) are frame-global and set here for every
  // shader; the per-node model matrix is applied by each Shape during the
  // scene-graph traversal below.
  for (size_t index = 0; index < shaders_.GetNumberShaders(); ++index) {
    auto& shader = shaders_.GetShader(index);

    shader.UseProgram();
    shader.SetUniform("projection", mvp_.GetProjection());
    shader.SetUniform("view", mvp_.GetView());
  }

  if (scene_.has_value()) {
    scene_->get().Draw();
  }
}

void GraphicsEngine::SetMVP(const MVP& new_mvp) { mvp_ = new_mvp; }

void GraphicsEngine::SetScene(Scene& scene) { scene_ = std::ref(scene); }

std::optional<std::reference_wrapper<Shader>> GraphicsEngine::SearchShader(
    const std::string& search_name) {
  return shaders_.SearchShader(search_name);
}
