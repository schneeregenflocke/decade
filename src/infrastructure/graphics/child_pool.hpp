#ifndef CHILD_POOL_HPP
#define CHILD_POOL_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include "drawable.hpp"
#include "scene_graph.hpp"
#include "shaders.hpp"

// A parent's dynamic children, reused across rebuilds instead of thrown away
// and made anew.
//
// Every section used to call RemoveChildren and build its children from
// scratch, so the same twelve month names cost twelve fresh nodes with twelve
// fresh shapes — and with them twelve new VAOs and VBOs — on every state change
// and on every keystroke in the title (#69). A pool walks the children it
// already has instead: `Next` hands back the one at the current position and
// creates it only once the pool has run out, and the destructor drops whatever
// is left over when the set has shrunk.
//
// One pool per parent per rebuild, meant as a local of the section builder.
//
// Parent and children travel as shared_ptr **values**, never as references into
// a child vector. That vector grows while the scene is being built, and growing
// reallocates: a pool holding a reference to its parent — or a caller holding
// one to a child it was handed — would then read freed memory. `BuildBars`
// does exactly that, one pool per date category, all alive while the next is
// made, so the fault stayed invisible until a second date category existed
// (#71). The copy costs one atomic increment per node and buys the whole class
// of bug away.

// What a pool hands back for a child that draws: the node, and its shape as
// the type it was asked for.
template <typename ShapeT>
struct ShapeChild {
  std::shared_ptr<SceneNode> node;
  ShapeT& shape;
};

// A parent's children in the order one rebuild asks for them. A group child
// carries no shape; a drawing child keeps its shape from the last rebuild as
// long as it asks for the same kind at the same position, and gets a new one
// otherwise — so a parent may hold children of several kinds.
class ChildPool {
 public:
  explicit ChildPool(std::shared_ptr<SceneNode> parent);

  ~ChildPool();

  ChildPool(const ChildPool&) = delete;
  ChildPool& operator=(const ChildPool&) = delete;
  ChildPool(ChildPool&&) = delete;
  ChildPool& operator=(ChildPool&&) = delete;

  [[nodiscard]] std::shared_ptr<SceneNode> Next(const std::string& name);

  // `shape_arguments` build the shape when the child needs a new one.
  template <typename ShapeT, typename... ShapeArguments>
  [[nodiscard]] ShapeChild<ShapeT> NextShape(
      const std::string& name, int draw_layer,
      ShapeArguments&... shape_arguments) {
    const std::shared_ptr<SceneNode> child = NextNode(name);
    const Drawable* shape = child->GetShape();
    if (shape == nullptr || shape->Kind() != ShapeT::kKind) {
      child->SetShape(std::make_unique<ShapeT>(shape_arguments...));
    }
    child->SetDrawLayer(draw_layer);
    auto& typed_shape = static_cast<ShapeT&>(*child->GetShape());
    return {.node = child, .shape = typed_shape};
  }

 private:
  [[nodiscard]] std::shared_ptr<SceneNode> NextNode(const std::string& name);

  std::shared_ptr<SceneNode> parent_;
  std::size_t used_{0};
};

// Children of one kind, all built with the same shader on the same layer.
template <typename ShapeT>
class ShapeChildPool {
 public:
  ShapeChildPool(std::shared_ptr<SceneNode> parent, Shader& shader,
                 int draw_layer)
      : pool_(std::move(parent)), shader_(shader), draw_layer_(draw_layer) {}

  [[nodiscard]] ShapeChild<ShapeT> Next(const std::string& name) {
    return pool_.NextShape<ShapeT>(name, draw_layer_, shader_);
  }

 private:
  ChildPool pool_;
  Shader& shader_;
  int draw_layer_;
};

#endif  // CHILD_POOL_HPP
