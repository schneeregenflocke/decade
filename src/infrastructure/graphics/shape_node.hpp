#ifndef SHAPE_NODE_HPP
#define SHAPE_NODE_HPP

#include <memory>
#include <string>
#include <utility>

#include "scene_graph.hpp"

// A scene node whose shape type is settled at creation and carried in the type
// from then on.
//
// The skeleton builds its nodes once and the section builders refill them on
// every rebuild. Without this handle each of those refills had to ask the RTTI
// what it was holding — for a type that had stood fixed since the skeleton was
// built (#68). `Make` is the only way in, so `Shape()` needs no check.
template <typename ShapeT>
class ShapeNode {
 public:
  ShapeNode() = default;

  template <typename... Args>
  [[nodiscard]] static ShapeNode Make(const std::string& name,
                                      Args&&... shape_args) {
    return ShapeNode(std::make_shared<SceneNode>(
        name, std::make_unique<ShapeT>(std::forward<Args>(shape_args)...)));
  }

  [[nodiscard]] const std::shared_ptr<SceneNode>& Node() const { return node_; }

  // A plain static_cast, and correct by construction: Make built the shape as a
  // ShapeT, and no other constructor exists. Calling SceneNode::SetShape on the
  // node behind Node() would break that promise — do not.
  [[nodiscard]] ShapeT& Shape() const {
    return static_cast<ShapeT&>(*node_->GetShape());
  }

 private:
  explicit ShapeNode(std::shared_ptr<SceneNode> node)
      : node_(std::move(node)) {}

  std::shared_ptr<SceneNode> node_;
};

#endif  // SHAPE_NODE_HPP
