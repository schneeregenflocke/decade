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

#include "rect.hpp"
#include "shapes.hpp"

class SceneNode {
 public:
  SceneNode() : model_matrix_(1.0F) {}

  explicit SceneNode(const std::string& name) : SceneNode() {
    node_name_ = name;
  }

  SceneNode(const std::string& name, std::shared_ptr<Shape> shape_ptr)
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

  void SetShape(std::shared_ptr<Shape> shape_ptr) {
    shape_ = std::move(shape_ptr);
  }

  [[nodiscard]] std::shared_ptr<Shape> GetShape() const { return shape_; }

  [[nodiscard]] const std::string& GetNodeName() const { return node_name_; }

  // Name of the domain ShapeConfiguration this node's appearance derives from
  // (empty when the node has no config-bound style, e.g. text or container
  // nodes). It is the stable link from a scene node back to the domain styling
  // that the rebuild reproduces, so the scene tree can route an edit to the
  // right config instead of mutating the transient node. Set by the scene
  // builder where it applies a configuration.
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
        if (bounds.width() > 0.0F || bounds.height() > 0.0F) {
          const std::array<glm::vec4, 4> corners = {
              glm::vec4(bounds.l(), bounds.b(), 0.0F, 1.0F),
              glm::vec4(bounds.r(), bounds.b(), 0.0F, 1.0F),
              glm::vec4(bounds.l(), bounds.t(), 0.0F, 1.0F),
              glm::vec4(bounds.r(), bounds.t(), 0.0F, 1.0F)};
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
      drawable.shape->Draw(drawable.world);
    }
  }

 private:
  std::string node_name_;
  std::string style_id_;
  std::vector<std::shared_ptr<SceneNode>> children_;
  glm::mat4 model_matrix_;
  std::shared_ptr<Shape> shape_;
  int draw_layer_{0};
  bool snapshot_hidden_{false};
};
#endif  // SCENE_GRAPH_HPP
