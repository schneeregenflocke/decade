#ifndef SCENE_GRAPH_HPP
#define SCENE_GRAPH_HPP

#include <cstddef>
#include <glm/mat4x4.hpp>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

#include "drawable.hpp"
#include "rect.hpp"

class SceneNode {
 public:
  SceneNode();

  explicit SceneNode(const std::string& name);

  SceneNode(const std::string& name, std::unique_ptr<Drawable> shape_ptr);

  void AddChild(const std::shared_ptr<SceneNode>& child);

  [[nodiscard]] const std::vector<std::shared_ptr<SceneNode>>& GetChildren()
      const;

  void RemoveChildren();

  // Drops every child past the first `count`. The counterpart to reusing
  // children across rebuilds: whatever the shorter set no longer needs falls
  // away, while the kept ones hold on to their GL buffers (#69).
  void TruncateChildren(std::size_t count);

  void SetShape(std::unique_ptr<Drawable> shape_ptr);

  // The node owns its shape alone; callers merely observe it, hence a
  // non-owning pointer (never store it as a data member).
  [[nodiscard]] Drawable* GetShape();

  [[nodiscard]] const Drawable* GetShape() const;

  [[nodiscard]] const std::string& GetNodeName() const;

  // A reused child stands for something else than it did last rebuild — the
  // seventh bar label may now read a different date.
  void SetNodeName(const std::string& name);

  // The name of the domain ShapeConfiguration this node's appearance comes from
  // (empty on nodes without such a binding, text and container nodes for
  // instance). The stable back reference to the configuration the rebuild
  // reproduces — the scene tree shows the node's values by it. Set by the scene
  // builder wherever it applies a configuration.
  void SetStyleId(const std::string& style_id);

  [[nodiscard]] const std::string& GetStyleId() const;

  // Marks this node (and its subtree) as an internal rendering aid that should
  // not appear in the user-facing scene tree — e.g. the selection-highlight
  // overlay. The snapshot builder skips hidden subtrees.
  void SetSnapshotHidden(bool hidden);

  [[nodiscard]] bool IsSnapshotHidden() const;

  // Axis-aligned bounding box of this subtree's shapes in world space, given
  // the accumulated parent world transform. Returns nullopt when no descendant
  // carries geometry. Mirrors Draw()'s transform accumulation; only shapes with
  // a non-empty local box contribute.
  [[nodiscard]] std::optional<RectF> WorldBounds(
      const glm::mat4& parent_world = glm::mat4(1.0F)) const;

  // Local transform of this node, relative to its parent. Draw() composes it
  // with the accumulated parent world transform; the default identity leaves a
  // node positioned exactly by its shape's own (absolute) vertices.
  void SetModelMatrix(const glm::mat4& matrix);

  [[nodiscard]] const glm::mat4& GetModelMatrix() const;

  // Painter's Draw layer. Lower layers are drawn first (further back), higher
  // layers on top. This makes the blend/overlap order an explicit property of
  // the node, independent of where it sits in the hierarchy: the transform
  // hierarchy decides *position*, the layer decides *what covers what*. Nodes
  // sharing a layer keep their traversal order. Default 0.
  void SetDrawLayer(int layer);

  [[nodiscard]] int GetDrawLayer() const;

  // Draws the subtree in two phases. First, a depth-first walk accumulates each
  // node's world transform (parent_world * local model matrix) and collects its
  // shape together with its Draw layer. Second, the collected shapes are drawn
  // in painter's order: a stable sort by layer, so equal layers keep the
  // traversal order.
  void Draw(const glm::mat4& parent_world = glm::mat4(1.0F));

 private:
  // The one depth-first walk both traversals above run (#35): they accumulate
  // the world matrix identically and differ only in what they collect. The
  // visitor sees each node together with its accumulated transform, this node
  // before its children.
  //
  // `Node` deduces the constness from the caller, so WorldBounds gets a const
  // walk and Draw a mutable one out of the same body. Children go onto the
  // stack back to front, because it pops from the back: pushed in order, the
  // last child would come off first and end up painted underneath its earlier
  // siblings (#29).
  template <typename Node, typename Visit>
  static void VisitDepthFirst(Node& root, const glm::mat4& parent_world,
                              Visit visit) {
    struct Entry {
      Node* node;
      glm::mat4 world;
    };
    std::vector<Entry> stack;
    stack.push_back(
        {.node = &root, .world = parent_world * root.model_matrix_});

    while (!stack.empty()) {
      const Entry current = stack.back();
      stack.pop_back();
      visit(*current.node, current.world);
      for (const auto& child : std::views::reverse(current.node->children_)) {
        stack.push_back({.node = child.get(),
                         .world = current.world * child->model_matrix_});
      }
    }
  }

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
[[nodiscard]] std::optional<std::string> FindNodePath(const SceneNode& root,
                                                      const SceneNode& target);
#endif  // SCENE_GRAPH_HPP
