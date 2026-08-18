#include "scene_graph.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "drawable.hpp"
#include "rect.hpp"

SceneNode::SceneNode() : model_matrix_(1.0F) {}

SceneNode::SceneNode(const std::string& name) : SceneNode() {
  node_name_ = name;
}

SceneNode::SceneNode(const std::string& name,
                     std::unique_ptr<Drawable> shape_ptr)
    : SceneNode(name) {
  shape_ = std::move(shape_ptr);
}

void SceneNode::AddChild(const std::shared_ptr<SceneNode>& child) {
  children_.push_back(child);
}

const std::vector<std::shared_ptr<SceneNode>>& SceneNode::GetChildren() const {
  return children_;
}

void SceneNode::RemoveChildren() { children_.clear(); }

void SceneNode::TruncateChildren(std::size_t count) {
  if (count < children_.size()) {
    children_.resize(count);
  }
}

void SceneNode::SetShape(std::unique_ptr<Drawable> shape_ptr) {
  shape_ = std::move(shape_ptr);
}

Drawable* SceneNode::GetShape() { return shape_.get(); }

const Drawable* SceneNode::GetShape() const { return shape_.get(); }

const std::string& SceneNode::GetNodeName() const { return node_name_; }

void SceneNode::SetNodeName(const std::string& name) { node_name_ = name; }

void SceneNode::SetStyleId(const std::string& style_id) {
  style_id_ = style_id;
}

const std::string& SceneNode::GetStyleId() const { return style_id_; }

std::optional<RectF> SceneNode::WorldBounds(
    const glm::mat4& parent_world) const {
  float min_x = std::numeric_limits<float>::max();
  float min_y = std::numeric_limits<float>::max();
  float max_x = std::numeric_limits<float>::lowest();
  float max_y = std::numeric_limits<float>::lowest();
  bool found = false;

  VisitDepthFirst(*this, parent_world,
                  [&](const SceneNode& node, const glm::mat4& world) {
                    if (node.shape_ == nullptr) {
                      return;
                    }
                    const RectF& bounds = node.shape_->LocalBounds();
                    if (bounds.Width() <= 0.0F && bounds.Height() <= 0.0F) {
                      return;
                    }
                    const std::array<glm::vec4, 4> corners = {
                        glm::vec4(bounds.Left(), bounds.Bottom(), 0.0F, 1.0F),
                        glm::vec4(bounds.Right(), bounds.Bottom(), 0.0F, 1.0F),
                        glm::vec4(bounds.Left(), bounds.Top(), 0.0F, 1.0F),
                        glm::vec4(bounds.Right(), bounds.Top(), 0.0F, 1.0F)};
                    for (const auto& corner : corners) {
                      const glm::vec4 world_corner = world * corner;
                      min_x = std::min(min_x, world_corner.x);
                      min_y = std::min(min_y, world_corner.y);
                      max_x = std::max(max_x, world_corner.x);
                      max_y = std::max(max_y, world_corner.y);
                    }
                    found = true;
                  });

  if (!found) {
    return std::nullopt;
  }
  return RectF(min_x, max_x, min_y, max_y);
}

void SceneNode::SetModelMatrix(const glm::mat4& matrix) {
  model_matrix_ = matrix;
}

const glm::mat4& SceneNode::GetModelMatrix() const { return model_matrix_; }

void SceneNode::SetDrawLayer(int layer) { draw_layer_ = layer; }

int SceneNode::GetDrawLayer() const { return draw_layer_; }

void SceneNode::Draw(const glm::mat4& parent_world) {
  struct DrawCall {
    Drawable* shape;
    glm::mat4 world;
    int layer;
  };
  std::vector<DrawCall> draw_calls;

  VisitDepthFirst(*this, parent_world,
                  [&](SceneNode& node, const glm::mat4& world) {
                    if (node.shape_ == nullptr) {
                      return;
                    }
                    draw_calls.push_back({.shape = node.shape_.get(),
                                          .world = world,
                                          .layer = node.draw_layer_});
                  });

  std::ranges::stable_sort(draw_calls,
                           [](const DrawCall& lhs, const DrawCall& rhs) {
                             return lhs.layer < rhs.layer;
                           });

  for (const auto& draw_call : draw_calls) {
    draw_call.shape->Draw(draw_call.world);
  }
}

std::optional<std::string> FindNodePath(const SceneNode& root,
                                        const SceneNode& target) {
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
