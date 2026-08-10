#include "scene_highlighter.hpp"

#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
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
                                   const ShapeNode<FillShape>& overlay_node,
                                   const ShapeNode<BoxesShape>& title_area_node,
                                   const ShapeConfigSet& shape_config)
    : scene_(scene),
      overlay_node_(overlay_node),
      title_area_node_(title_area_node),
      shape_config_(shape_config) {}

void SceneHighlighter::Refresh(
    std::unordered_map<std::size_t, std::shared_ptr<SceneNode>> bar_nodes) {
  bar_nodes_ = std::move(bar_nodes);
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
    case PickId::Kind::kBar: {
      const auto iterator = bar_nodes_.find(picked.index);
      return iterator == bar_nodes_.end() ? nullptr : iterator->second;
    }
    case PickId::Kind::kTitle:
      return title_area_node_.Node();
  }
  return nullptr;
}

void SceneHighlighter::SetSelectedNode(const std::optional<std::string>& path) {
  selected_path_ = path;
  ApplySelectionOverlay();
}

void SceneHighlighter::ApplySelectionOverlay() {
  FillShape& shape = overlay_node_.Shape();
  std::optional<RectF> bounds;
  if (selected_path_.has_value()) {
    bounds = NodeWorldBounds(*selected_path_);
  }
  if (bounds.has_value()) {
    shape.SetShape(*bounds);
    shape.SetColor(glm::vec4(kSelectionRed, kSelectionGreen, kSelectionBlue,
                             kSelectionAlpha));
  } else {
    shape.SetShape(RectF(kZero, kZero, kZero, kZero));
  }
}

std::optional<RectF> SceneHighlighter::NodeWorldBounds(
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
  return node->WorldBounds(parent_world);
}

void SceneHighlighter::ApplyHover(const PickId& picked, bool highlighted) {
  const auto node = NodeFor(picked);
  if (!node) {
    return;
  }
  // A bar node comes out of the map by index, so its type is not settled by a
  // handle here — but the drawable says what it is, which needs no RTTI.
  Drawable* drawable = node->GetShape();
  if (drawable == nullptr || drawable->Kind() != DrawableKind::kBoxes) {
    return;
  }
  auto& shape = static_cast<BoxesShape&>(*drawable);
  const auto config = shape_config_.GetShapeConfiguration(node->GetStyleId());
  const glm::vec4 hover_outline(kOne, kHoverOutlineGreen, kZero, kOne);
  shape.SetColors(highlighted ? hover_outline : config.OutlineColor(),
                  config.FillColor());
}
