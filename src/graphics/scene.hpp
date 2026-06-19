#ifndef SCENE_HPP
#define SCENE_HPP

#include <glm/mat4x4.hpp>
#include <memory>

#include "scene_graph.hpp"

// Infrastructure: the single owner (SSOT) of the render scene graph's root.
//
// Previously the root `SceneNode` was co-owned by two `shared_ptr`s — one in
// the CalendarSceneComposer, one in the GraphicsEngine — which left ownership
// ambiguous. `Scene` makes it unambiguous: it owns the root, and every other
// component *borrows* the Scene (by reference or non-owning pointer) rather
// than owning the graph. The builder mutates the graph through `Root()`, the
// engine renders it through `Draw()`, and the snapshot/picking code reads it
// through `Root()`.
//
// It deliberately exposes the node directly (rather than wrapping every
// SceneNode operation) so it stays a thin ownership boundary, not a second API
// surface over the scene graph.
class Scene {
 public:
  Scene() : root_(std::make_unique<SceneNode>(kRootName)) {}

  [[nodiscard]] SceneNode& Root() { return *root_; }
  [[nodiscard]] const SceneNode& Root() const { return *root_; }

  // Renders the whole graph. Painter-order layering is handled inside
  // SceneNode::Draw; this is just the entry point the engine calls per frame.
  void Draw(const glm::mat4& parent_world = glm::mat4(1.0F)) {
    root_->Draw(parent_world);
  }

 private:
  static constexpr const char* kRootName = "root";
  // Sole ownership: the children below the root remain shared_ptr (co-owned by
  // their parent and the builder's named-node members), but the root itself has
  // exactly one owner — this Scene.
  std::unique_ptr<SceneNode> root_;
};

#endif  // SCENE_HPP
