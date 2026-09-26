#ifndef GRAPHICS_ENGINE_HPP
#define GRAPHICS_ENGINE_HPP

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

#include "mvp_matrices.hpp"
#include "scene.hpp"
#include "shaders.hpp"

class GraphicsEngine {
 public:
  // What shows where the scene draws nothing: grey around the page on the
  // canvas, paper in an export, where a calendar laid out in millimetres runs
  // past the page.
  enum class Backdrop : std::uint8_t { kCanvas, kPaper };

  void Render(Backdrop backdrop);

  void SetMVP(const MVP& new_mvp);

  // Borrows the Scene (non-owning): its owner outlives the engine's use of it,
  // so there is a single source of truth for the graph's lifetime. An optional
  // reference and not a pointer — the engine can be asked to render before a
  // scene has been set, which is the one absence to model, and a raw pointer
  // member would say nothing about ownership either way.
  void SetScene(Scene& scene);

  std::optional<std::reference_wrapper<Shader>> SearchShader(
      const std::string& search_name);

 private:
  static constexpr float kCanvasGray = 0.2F;

  MVP mvp_;
  Shaders shaders_;
  std::optional<std::reference_wrapper<Scene>> scene_;
};
#endif  // GRAPHICS_ENGINE_HPP
