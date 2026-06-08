#ifndef SCENE_GRAPH_HPP
#define SCENE_GRAPH_HPP

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

  void update() {
    std::vector<std::shared_ptr<SceneNode>> stack;
    stack.push_back(shared_from_this());

    while (!stack.empty()) {
      auto current = stack.back();
      stack.pop_back();
      for (const auto& child : current->children_) {
        stack.push_back(child);
      }
    }
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

  void draw() {
    std::vector<std::shared_ptr<SceneNode>> stack;
    stack.push_back(shared_from_this());

    while (!stack.empty()) {
      auto current = stack.back();
      stack.pop_back();
      if (current->shape_ != nullptr) {
        current->shape_->draw();
      }
      for (const auto& child : current->children_) {
        stack.push_back(child);
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
