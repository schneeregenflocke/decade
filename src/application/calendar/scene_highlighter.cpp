#include "scene_highlighter.hpp"

#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../../domain/shape_configuration.hpp"
#include "../../infrastructure/graphics/drawable.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"
#include "../../infrastructure/graphics/rect.hpp"
#include "../../infrastructure/graphics/scene.hpp"
#include "../../infrastructure/graphics/scene_graph.hpp"
#include "../../infrastructure/graphics/shape_node.hpp"
#include "../../infrastructure/graphics/shapes.hpp"

SceneHighlighter::SceneHighlighter(const Scene& scene,
                                   const ShapeNode<BoxesShape>& overlay_node,
                                   const ShapeNode<BoxesShape>& title_area_node,
                                   const ShapeConfigSet& shape_config)
    : scene_(scene),
      overlay_node_(overlay_node),
      title_area_node_(title_area_node),
      shape_config_(shape_config) {}

void SceneHighlighter::Refresh(
    std::vector<std::shared_ptr<SceneNode>> entry_nodes) {
  entry_nodes_ = std::move(entry_nodes);
  if (hovered_.has_value()) {
    ApplyHover(*hovered_, /*highlighted=*/true);
  }
  ApplySelectionOverlay();
}

void SceneHighlighter::SetHovered(const std::optional<PickId>& hovered) {
  if (hovered == hovered_) {
    return;
  }
  if (hovered_.has_value()) {
    ApplyHover(*hovered_, /*highlighted=*/false);
  }
  hovered_ = hovered;
  if (hovered_.has_value()) {
    ApplyHover(*hovered_, /*highlighted=*/true);
  }
}

std::shared_ptr<SceneNode> SceneHighlighter::NodeFor(
    const PickId& picked) const {
  switch (picked.kind) {
    case PickId::Kind::kBar:
      return picked.index < entry_nodes_.size() ? entry_nodes_[picked.index]
                                                : nullptr;
    case PickId::Kind::kTitle:
      return title_area_node_.Node();
  }
  return nullptr;
}

void SceneHighlighter::SetSelectedNodes(const std::vector<std::string>& paths) {
  selected_paths_ = paths;
  ApplySelectionOverlay();
}

void SceneHighlighter::ApplySelectionOverlay() {
  std::vector<RectF> boxes;
  for (const std::string& path : selected_paths_) {
    if (const auto placed = Resolve(path); placed.has_value()) {
      AppendCoverage(*placed, boxes);
    }
  }
  BoxesShape& shape = overlay_node_.Shape();
  if (boxes.empty()) {
    shape.Hide();
    return;
  }
  const glm::vec4 accent(kSelectionRed, kSelectionGreen, kSelectionBlue,
                         kSelectionAlpha);
  shape.SetShape(boxes, kZero);
  shape.SetColors(accent, accent);
}

std::optional<SceneHighlighter::PlacedNode> SceneHighlighter::Resolve(
    const std::string& path) const {
  std::vector<std::string> segments;
  std::size_t start = 0;
  while (start <= path.size()) {
    const std::size_t slash = path.find('/', start);
    const std::size_t end = (slash == std::string::npos) ? path.size() : slash;
    segments.push_back(path.substr(start, end - start));
    if (slash == std::string::npos) {
      break;
    }
    start = slash + 1;
  }
  if (segments.empty() || scene_.Root().GetNodeName() != segments.front()) {
    return std::nullopt;
  }

  const SceneNode* node = &scene_.Root();
  glm::mat4 parent_world(1.0F);
  for (std::size_t index = 1; index < segments.size(); ++index) {
    parent_world = parent_world * node->GetModelMatrix();
    const SceneNode* next = nullptr;
    for (const auto& child : node->GetChildren()) {
      if (child->GetNodeName() == segments[index]) {
        next = child.get();
        break;
      }
    }
    if (next == nullptr) {
      return std::nullopt;
    }
    node = next;
  }
  return PlacedNode{.node = node, .parent_world = parent_world};
}

void SceneHighlighter::AppendCoverage(const PlacedNode& placed,
                                      std::vector<RectF>& boxes) {
  std::vector<PlacedNode> stack{placed};
  while (!stack.empty()) {
    const PlacedNode current = stack.back();
    stack.pop_back();
    if (current.node->GetShape() != nullptr) {
      if (const auto bounds = current.node->WorldBounds(current.parent_world);
          bounds.has_value()) {
        boxes.push_back(*bounds);
      }
      continue;
    }
    const glm::mat4 world =
        current.parent_world * current.node->GetModelMatrix();
    for (const auto& child : current.node->GetChildren()) {
      stack.push_back({.node = child.get(), .parent_world = world});
    }
  }
}

void SceneHighlighter::ApplyHover(const PickId& picked, bool highlighted) {
  const auto root = NodeFor(picked);
  if (!root) {
    return;
  }
  struct Visit {
    SceneNode* node;
    const SceneNode* handed_down;
  };
  std::vector<Visit> stack{{.node = root.get(), .handed_down = nullptr}};
  while (!stack.empty()) {
    const Visit current = stack.back();
    stack.pop_back();
    const SceneNode* for_members =
        HandedDownStyleSource(*current.node, current.handed_down);
    for (const auto& child : current.node->GetChildren()) {
      stack.push_back({.node = child.get(), .handed_down = for_members});
    }
    // The drawable says what it is, which needs no RTTI.
    Drawable* drawable = current.node->GetShape();
    const SceneNode* style = StyleSource(*current.node, current.handed_down);
    if (drawable == nullptr || drawable->Kind() != DrawableKind::kBoxes ||
        style == nullptr) {
      continue;
    }
    auto& shape = static_cast<BoxesShape&>(*drawable);
    const auto config =
        shape_config_.GetShapeConfiguration(style->GetStyleId());
    const glm::vec4 hover_outline(kOne, kHoverOutlineGreen, kZero, kOne);
    shape.SetColors(highlighted ? hover_outline : config.OutlineColor(),
                    config.FillColor());
  }
}
