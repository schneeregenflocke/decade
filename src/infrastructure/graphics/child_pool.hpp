#ifndef CHILD_POOL_HPP
#define CHILD_POOL_HPP

#include <cstddef>
#include <memory>
#include <string>

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
// One pool per parent per rebuild. It holds the parent by reference and is
// meant as a local of the section builder, nothing else.

// Children without a shape — pure grouping nodes.
class ChildPool {
 public:
  explicit ChildPool(const std::shared_ptr<SceneNode>& parent)
      : parent_(parent) {}

  ~ChildPool() { parent_->TruncateChildren(used_); }

  ChildPool(const ChildPool&) = delete;
  ChildPool& operator=(const ChildPool&) = delete;
  ChildPool(ChildPool&&) = delete;
  ChildPool& operator=(ChildPool&&) = delete;

  [[nodiscard]] const std::shared_ptr<SceneNode>& Next(
      const std::string& name) {
    if (used_ == parent_->GetChildren().size()) {
      parent_->AddChild(std::make_shared<SceneNode>(name));
    }
    const std::shared_ptr<SceneNode>& child = parent_->GetChildren()[used_];
    ++used_;
    child->SetNodeName(name);
    return child;
  }

 private:
  const std::shared_ptr<SceneNode>& parent_;
  std::size_t used_{0};
};

// Children carrying a shape of one kind. The pool fills the parent alone, so
// every child it hands back is a ShapeT — which is what lets the cast stand
// without a check, as with ShapeNode.
template <typename ShapeT>
class ShapeChildPool {
 public:
  struct Child {
    const std::shared_ptr<SceneNode>& node;
    ShapeT& shape;
  };

  ShapeChildPool(const std::shared_ptr<SceneNode>& parent, Shader& shader,
                 int draw_layer)
      : parent_(parent), shader_(shader), draw_layer_(draw_layer) {}

  ~ShapeChildPool() { parent_->TruncateChildren(used_); }

  ShapeChildPool(const ShapeChildPool&) = delete;
  ShapeChildPool& operator=(const ShapeChildPool&) = delete;
  ShapeChildPool(ShapeChildPool&&) = delete;
  ShapeChildPool& operator=(ShapeChildPool&&) = delete;

  [[nodiscard]] Child Next(const std::string& name) {
    if (used_ == parent_->GetChildren().size()) {
      parent_->AddChild(std::make_shared<SceneNode>(
          name, std::make_unique<ShapeT>(shader_)));
    }
    const std::shared_ptr<SceneNode>& child = parent_->GetChildren()[used_];
    ++used_;
    child->SetNodeName(name);
    child->SetDrawLayer(draw_layer_);
    return Child{.node = child,
                 .shape = static_cast<ShapeT&>(*child->GetShape())};
  }

 private:
  const std::shared_ptr<SceneNode>& parent_;
  Shader& shader_;
  int draw_layer_;
  std::size_t used_{0};
};

#endif  // CHILD_POOL_HPP
