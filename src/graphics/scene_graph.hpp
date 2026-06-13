#ifndef SCENE_GRAPH_HPP
#define SCENE_GRAPH_HPP

#include <glm/mat4x4.hpp>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "shapes.hpp"
class SceneNode : public std::enable_shared_from_this<SceneNode> {
 public:
  SceneNode() : model_matrix_(1.0F) {}

  explicit SceneNode(const std::string& name) : SceneNode() {
    node_name_ = name;
  }

  SceneNode(const std::string& name, std::shared_ptr<Shape> shape_ptr)
      : SceneNode(name) {
    shape_ = std::move(shape_ptr);
  }

  void add_child(const std::shared_ptr<SceneNode>& child) {
    children_.push_back(child);
  }

  auto add_child() {
    auto child = std::make_shared<SceneNode>();
    children_.push_back(child);
    return child;
  }

  [[nodiscard]] const std::vector<std::shared_ptr<SceneNode>>& get_children()
      const {
    return children_;
  }

  void remove_children() { children_.clear(); }

  void set_shape(std::shared_ptr<Shape> shape_ptr) {
    shape_ = std::move(shape_ptr);
  }

  std::shared_ptr<Shape> get_shape() const { return shape_; }

  [[nodiscard]] const std::string& get_node_name() const { return node_name_; }

  // Local transform of this node, relative to its parent. draw() composes it
  // with the accumulated parent world transform; the default identity leaves a
  // node positioned exactly by its shape's own (absolute) vertices.
  void set_model_matrix(const glm::mat4& matrix) { model_matrix_ = matrix; }

  [[nodiscard]] const glm::mat4& get_model_matrix() const {
    return model_matrix_;
  }

  std::optional<std::shared_ptr<SceneNode>> search_node(
      const std::string& search_name) {
    std::vector<std::shared_ptr<SceneNode>> stack;
    stack.push_back(shared_from_this());

    while (!stack.empty()) {
      auto current = stack.back();
      stack.pop_back();
      if (current->node_name_ == search_name) {
        return {current};
      }
      for (const auto& child : current->children_) {
        stack.push_back(child);
      }
    }

    return std::nullopt;
  }

  // Traverses the subtree and draws each shape with its accumulated world
  // transform (parent_world * local model matrix). The traversal order is the
  // same depth-first order as before; only the per-node model matrix is new.
  void draw(const glm::mat4& parent_world = glm::mat4(1.0F)) {
    struct Entry {
      SceneNode* node;
      glm::mat4 world;
    };

    std::vector<Entry> stack;
    stack.push_back({.node = this, .world = parent_world * model_matrix_});

    while (!stack.empty()) {
      const Entry current = stack.back();
      stack.pop_back();
      if (current.node->shape_ != nullptr) {
        current.node->shape_->draw(current.world);
      }
      for (const auto& child : current.node->children_) {
        stack.push_back({.node = child.get(),
                         .world = current.world * child->model_matrix_});
      }
    }
  }

 private:
  std::string node_name_;
  std::vector<std::shared_ptr<SceneNode>> children_;
  glm::mat4 model_matrix_;
  std::shared_ptr<Shape> shape_;
};
#endif  // SCENE_GRAPH_HPP
