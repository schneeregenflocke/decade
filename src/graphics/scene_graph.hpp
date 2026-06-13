#ifndef SCENE_GRAPH_HPP
#define SCENE_GRAPH_HPP

#include <algorithm>
#include <glm/mat4x4.hpp>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "pick_id.hpp"
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

  // Painter's draw layer. Lower layers are drawn first (further back), higher
  // layers on top. This makes the blend/overlap order an explicit property of
  // the node, independent of where it sits in the hierarchy: the transform
  // hierarchy decides *position*, the layer decides *what covers what*. Nodes
  // sharing a layer keep their traversal order. Default 0.
  void set_draw_layer(int layer) { draw_layer_ = layer; }

  [[nodiscard]] int get_draw_layer() const { return draw_layer_; }

  // Optional pick identity: set on nodes that hit-testing can return (e.g. each
  // bar). Carried here so a picked element maps straight back to its node.
  void set_pick_id(const PickId& pick_id) { pick_id_ = pick_id; }

  [[nodiscard]] const std::optional<PickId>& get_pick_id() const {
    return pick_id_;
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

  // Draws the subtree in two phases. First, a depth-first walk accumulates each
  // node's world transform (parent_world * local model matrix) and collects its
  // shape together with its draw layer. Second, the collected shapes are drawn
  // in painter's order: a stable sort by layer, so equal layers keep the
  // traversal order. With every layer left at its default this reproduces the
  // previous traversal draw order exactly; assigning layers lets the overlap
  // order be controlled independently of the tree structure.
  void draw(const glm::mat4& parent_world = glm::mat4(1.0F)) {
    struct Drawable {
      Shape* shape;
      glm::mat4 world;
      int layer;
    };
    std::vector<Drawable> drawables;

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
        drawables.push_back({.shape = current.node->shape_.get(),
                             .world = current.world,
                             .layer = current.node->draw_layer_});
      }
      for (const auto& child : current.node->children_) {
        stack.push_back({.node = child.get(),
                         .world = current.world * child->model_matrix_});
      }
    }

    std::ranges::stable_sort(drawables,
                             [](const Drawable& lhs, const Drawable& rhs) {
                               return lhs.layer < rhs.layer;
                             });

    for (const auto& drawable : drawables) {
      drawable.shape->draw(drawable.world);
    }
  }

 private:
  std::string node_name_;
  std::vector<std::shared_ptr<SceneNode>> children_;
  glm::mat4 model_matrix_;
  std::shared_ptr<Shape> shape_;
  int draw_layer_{0};
  std::optional<PickId> pick_id_;
};
#endif  // SCENE_GRAPH_HPP
