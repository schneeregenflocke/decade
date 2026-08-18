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

  // The per-node model matrix is applied by each Shape during the scene-graph
  // traversal below.
  shaders_.SetCameraUniforms(mvp_);

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
