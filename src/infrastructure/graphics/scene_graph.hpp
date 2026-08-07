#ifndef SCENE_GRAPH_HPP
#define SCENE_GRAPH_HPP

#include <algorithm>
#include <array>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "drawable.hpp"
#include "rect.hpp"

class SceneNode {
 public:
  SceneNode() : model_matrix_(1.0F) {}

  explicit SceneNode(const std::string& name) : SceneNode() {
    node_name_ = name;
  }

  SceneNode(const std::string& name, std::unique_ptr<Drawable> shape_ptr)
      : SceneNode(name) {
    shape_ = std::move(shape_ptr);
  }

  void AddChild(const std::shared_ptr<SceneNode>& child) {
    children_.push_back(child);
  }

  [[nodiscard]] const std::vector<std::shared_ptr<SceneNode>>& GetChildren()
      const {
    return children_;
  }

  void RemoveChildren() { children_.clear(); }

  void SetShape(std::unique_ptr<Drawable> shape_ptr) {
    shape_ = std::move(shape_ptr);
  }

  // The node owns its shape alone; callers merely observe it, hence a
  // non-owning pointer (never store it as a data member).
  [[nodiscard]] Drawable* GetShape() { return shape_.get(); }

  [[nodiscard]] const Drawable* GetShape() const { return shape_.get(); }

  [[nodiscard]] const std::string& GetNodeName() const { return node_name_; }

  // The name of the domain ShapeConfiguration this node's appearance comes from
  // (empty on nodes without such a binding, text and container nodes for
  // instance). The stable back reference to the configuration the rebuild
  // reproduces — the scene tree shows the node's values by it. Set by the scene
  // builder wherever it applies a configuration.
  void SetStyleId(const std::string& style_id) { style_id_ = style_id; }

  [[nodiscard]] const std::string& GetStyleId() const { return style_id_; }

  // Marks this node (and its subtree) as an internal rendering aid that should
  // not appear in the user-facing scene tree — e.g. the selection-highlight
  // overlay. The snapshot builder skips hidden subtrees.
  void SetSnapshotHidden(bool hidden) { snapshot_hidden_ = hidden; }

  [[nodiscard]] bool IsSnapshotHidden() const { return snapshot_hidden_; }

  // Axis-aligned bounding box of this subtree's shapes in world space, given
  // the accumulated parent world transform. Returns nullopt when no descendant
  // carries geometry. Mirrors Draw()'s transform accumulation; only shapes with
  // a non-empty local box contribute.
  [[nodiscard]] std::optional<rectf> WorldBounds(
      const glm::mat4& parent_world = glm::mat4(1.0F)) const {
    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float max_y = std::numeric_limits<float>::lowest();
    bool found = false;

    struct Entry {
      const SceneNode* node;
      glm::mat4 world;
    };
    std::vector<Entry> stack;
    stack.push_back({.node = this, .world = parent_world * model_matrix_});

    while (!stack.empty()) {
      const Entry current = stack.back();
      stack.pop_back();
      if (current.node->shape_ != nullptr) {
        const rectf& bounds = current.node->shape_->LocalBounds();
        if (bounds.Width() > 0.0F || bounds.Height() > 0.0F) {
          const std::array<glm::vec4, 4> corners = {
              glm::vec4(bounds.Left(), bounds.Bottom(), 0.0F, 1.0F),
              glm::vec4(bounds.Right(), bounds.Bottom(), 0.0F, 1.0F),
              glm::vec4(bounds.Left(), bounds.Top(), 0.0F, 1.0F),
              glm::vec4(bounds.Right(), bounds.Top(), 0.0F, 1.0F)};
          for (const auto& corner : corners) {
            const glm::vec4 world_corner = current.world * corner;
            min_x = std::min(min_x, world_corner.x);
            min_y = std::min(min_y, world_corner.y);
            max_x = std::max(max_x, world_corner.x);
            max_y = std::max(max_y, world_corner.y);
          }
          found = true;
        }
      }
      for (const auto& child : current.node->children_) {
        stack.push_back({.node = child.get(),
                         .world = current.world * child->model_matrix_});
      }
    }

    if (!found) {
      return std::nullopt;
    }
    return rectf(min_x, max_x, min_y, max_y);
  }

  // Local transform of this node, relative to its parent. Draw() composes it
  // with the accumulated parent world transform; the default identity leaves a
  // node positioned exactly by its shape's own (absolute) vertices.
  void SetModelMatrix(const glm::mat4& matrix) { model_matrix_ = matrix; }

  [[nodiscard]] const glm::mat4& GetModelMatrix() const {
    return model_matrix_;
  }

  // Painter's Draw layer. Lower layers are drawn first (further back), higher
  // layers on top. This makes the blend/overlap order an explicit property of
  // the node, independent of where it sits in the hierarchy: the transform
  // hierarchy decides *position*, the layer decides *what covers what*. Nodes
  // sharing a layer keep their traversal order. Default 0.
  void SetDrawLayer(int layer) { draw_layer_ = layer; }

  [[nodiscard]] int GetDrawLayer() const { return draw_layer_; }

  // Draws the subtree in two phases. First, a depth-first walk accumulates each
  // node's world transform (parent_world * local model matrix) and collects its
  // shape together with its Draw layer. Second, the collected shapes are drawn
  // in painter's order: a stable sort by layer, so equal layers keep the
  // traversal order. With every layer left at its default this reproduces the
  // previous traversal Draw order exactly; assigning layers lets the overlap
  // order be controlled independently of the tree structure.
  void Draw(const glm::mat4& parent_world = glm::mat4(1.0F)) {
    struct DrawCall {
      Drawable* shape;
      glm::mat4 world;
      int layer;
    };
    std::vector<DrawCall> draw_calls;

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
        draw_calls.push_back({.shape = current.node->shape_.get(),
                              .world = current.world,
                              .layer = current.node->draw_layer_});
      }
      for (const auto& child : current.node->children_) {
        stack.push_back({.node = child.get(),
                         .world = current.world * child->model_matrix_});
      }
    }

    std::ranges::stable_sort(draw_calls,
                             [](const DrawCall& lhs, const DrawCall& rhs) {
                               return lhs.layer < rhs.layer;
                             });

    for (const auto& draw_call : draw_calls) {
      draw_call.shape->Draw(draw_call.world);
    }
  }

 private:
  std::string node_name_;
  std::string style_id_;
  std::vector<std::shared_ptr<SceneNode>> children_;
  glm::mat4 model_matrix_;
  std::unique_ptr<Drawable> shape_;
  int draw_layer_{0};
  bool snapshot_hidden_{false};
};

// The path "root/.../name" to a node of the tree, or empty when it does not lie
// in it. The counterpart to resolving a path: the same notation the scene tree
// panel and the selection highlight use.
[[nodiscard]] inline std::optional<std::string> FindNodePath(
    const SceneNode& root, const SceneNode& target) {
  struct Entry {
    const SceneNode* node;
    std::string path;
  };
  std::vector<Entry> stack;
  stack.push_back({.node = &root, .path = root.GetNodeName()});

  while (!stack.empty()) {
    const Entry current = stack.back();
    stack.pop_back();
    if (current.node == &target) {
      return current.path;
    }
    for (const auto& child : current.node->GetChildren()) {
      stack.push_back({.node = child.get(),
                       .path = current.path + '/' + child->GetNodeName()});
    }
  }

  return std::nullopt;
}
#endif  // SCENE_GRAPH_HPP
