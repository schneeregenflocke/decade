#ifndef HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_SCENE_GRAPH_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_SCENE_GRAPH_HPP

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "shapes.hpp"
class SceneNode : public std::enable_shared_from_this<SceneNode> {
 public:
  SceneNode() : model_matrix(1.0F), is_dirty(false) {}

  explicit SceneNode(const std::string& name) : SceneNode() {
    node_name = name;
  }

  SceneNode(const std::string& name, std::shared_ptr<Shape> shape_ptr)
      : SceneNode(name) {
    shape = std::move(shape_ptr);
  }

  void add_child(const std::shared_ptr<SceneNode>& child) {
    children.push_back(child);
  }

  auto add_child() {
    auto child = std::make_shared<SceneNode>();
    children.push_back(child);
    return child;
  }

  [[nodiscard]] const std::vector<std::shared_ptr<SceneNode>>& get_children()
      const {
    return children;
  }

  void remove_children() { children.clear(); }

  void set_shape(std::shared_ptr<Shape> shape_ptr) {
    shape = std::move(shape_ptr);
  }

  std::shared_ptr<Shape> get_shape() const { return shape; }

  void update() {
    std::vector<std::shared_ptr<SceneNode>> stack;
    stack.push_back(shared_from_this());

    while (!stack.empty()) {
      auto current = stack.back();
      stack.pop_back();
      for (const auto& child : current->children) {
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
      if (current->node_name == search_name) {
        return {current};
      }
      for (const auto& child : current->children) {
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
      if (current->shape != nullptr) {
        current->shape->draw();
      }
      for (const auto& child : current->children) {
        stack.push_back(child);
      }
    }
  }

 private:
  std::string node_name;
  std::vector<std::shared_ptr<SceneNode>> children;
  glm::mat4 model_matrix;
  std::shared_ptr<Shape> shape;
  bool is_dirty;
};
#endif  // HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_SCENE_GRAPH_HPP
