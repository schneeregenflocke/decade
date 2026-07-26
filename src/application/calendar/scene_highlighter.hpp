#ifndef SCENE_HIGHLIGHTER_HPP
#define SCENE_HIGHLIGHTER_HPP

#include <cstddef>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../domain/shape_configuration.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"
#include "../../infrastructure/graphics/rect.hpp"
#include "../../infrastructure/graphics/scene.hpp"
#include "../../infrastructure/graphics/scene_graph.hpp"
#include "../../infrastructure/graphics/shapes.hpp"

// Application/Infrastructure bridge: the interactive highlighting of the
// calendar scene, kept apart from the construction concern
// (CalendarSceneComposer / section builders). It owns the two transient
// highlights:
//
//   * the hovered element (a bar or the title frame), recoloured in place via
//     its shape; and
//   * the scene-tree-selected node and its subtree, covered by a translucent
//     overlay quad.
//
// Both are applied without a scene rebuild. The coordinator calls Refresh()
// once per rebuild to hand over the freshly built bar nodes and re-apply the
// persisted highlights to the new geometry.
class SceneHighlighter {
 public:
  SceneHighlighter(const Scene& scene,
                   const std::shared_ptr<SceneNode>& overlay_node,
                   const std::shared_ptr<SceneNode>& title_frame_node,
                   const ShapeConfigSet& shape_config)
      : scene_(scene),
        overlay_node_(overlay_node),
        title_frame_node_(title_frame_node),
        shape_config_(shape_config) {}

  // Adopts the bar nodes from the latest rebuild and re-applies the persisted
  // hover and selection highlights to the fresh geometry.
  void Refresh(
      std::unordered_map<std::size_t, std::shared_ptr<SceneNode>> bar_nodes) {
    bar_nodes_ = std::move(bar_nodes);
    if (hovered_.has_value()) {
      ApplyHover(*hovered_, /*highlighted=*/true);
    }
    ApplySelectionOverlay();
  }

  // Highlights the hovered element (and restores the previously hovered one) by
  // recolouring its shape in place — no scene rebuild. A null value clears it.
  void SetHovered(const std::optional<PickId>& hovered) {
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

  // Highlights the scene node identified by `path` (and its subtree) with a
  // translucent overlay — no rebuild. A null/unknown path clears the overlay.
  void SetSelectedNode(const std::optional<std::string>& path) {
    selected_path_ = path;
    ApplySelectionOverlay();
  }

 private:
  // Positions the selection overlay over the currently selected node's world
  // bounds, or hides it (zero-area quad) when there is no resolvable selection.
  void ApplySelectionOverlay() {
    auto shape = std::dynamic_pointer_cast<QuadrilateralShape>(
        overlay_node_->GetShape());
    if (!shape) {
      return;
    }
    std::optional<rectf> bounds;
    if (selected_path_.has_value()) {
      bounds = NodeWorldBounds(*selected_path_);
    }
    if (bounds.has_value()) {
      shape->SetShape(*bounds);
      shape->SetColor(glm::vec4(kSelectionRed, kSelectionGreen, kSelectionBlue,
                                kSelectionAlpha));
    } else {
      shape->SetShape(rectf(kZero, kZero, kZero, kZero));
    }
  }

  // Resolves a "root/.../name" path to the world-space bounds of that node's
  // subtree (page space, matching the bars' pick boxes). Returns nullopt when
  // any path segment does not resolve or the subtree carries no geometry.
  [[nodiscard]] std::optional<rectf> NodeWorldBounds(
      const std::string& path) const {
    std::vector<std::string> segments;
    std::size_t start = 0;
    while (start <= path.size()) {
      const std::size_t slash = path.find('/', start);
      const std::size_t end =
          (slash == std::string::npos) ? path.size() : slash;
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

  // Der Knoten, den ein Treffer meint — leer, wenn sein Index nach einem
  // Rebuild ins Leere zeigt.
  [[nodiscard]] std::shared_ptr<SceneNode> HoveredNode(
      const PickId& picked) const {
    switch (picked.kind) {
      case PickId::Kind::kBar: {
        const auto iterator = bar_nodes_.find(picked.index);
        return iterator == bar_nodes_.end() ? nullptr : iterator->second;
      }
      case PickId::Kind::kTitle:
        return title_frame_node_;
    }
    return nullptr;
  }

  // Recolours the hovered element's outline: highlighted gets the hover accent,
  // otherwise the colours of the configuration its style id points at. Fill is
  // left as configured so the hover reads as an outline accent.
  void ApplyHover(const PickId& picked, bool highlighted) {
    const auto node = HoveredNode(picked);
    if (!node) {
      return;
    }
    auto shape = std::dynamic_pointer_cast<RectanglesShape>(node->GetShape());
    if (!shape) {
      return;
    }
    const auto config = shape_config_.GetShapeConfiguration(node->GetStyleId());
    const glm::vec4 hover_outline(kOne, kHoverOutlineGreen, kZero, kOne);
    shape->SetColor({highlighted ? hover_outline : config.OutlineColor(),
                     config.FillColor()});
  }

  static constexpr float kZero = 0.0F;
  static constexpr float kOne = 1.0F;
  static constexpr float kHoverOutlineGreen = 0.55F;

  // Translucent accent for the scene-tree selection overlay.
  static constexpr float kSelectionRed = 1.0F;
  static constexpr float kSelectionGreen = 0.6F;
  static constexpr float kSelectionBlue = 0.0F;
  static constexpr float kSelectionAlpha = 0.35F;

  // Borrowed (non-owning) collaborators, owned by CalendarPage / the builder.
  const Scene& scene_;
  const std::shared_ptr<SceneNode>& overlay_node_;
  const std::shared_ptr<SceneNode>& title_frame_node_;
  const ShapeConfigSet& shape_config_;

  // Bar nodes by index from the latest rebuild, for the in-place hover
  // recolour.
  std::unordered_map<std::size_t, std::shared_ptr<SceneNode>> bar_nodes_;
  std::optional<PickId> hovered_;
  // Path of the scene-tree-selected node ("root/.../name"); persists across
  // rebuilds so the overlay is re-applied to the fresh geometry.
  std::optional<std::string> selected_path_;
};

#endif  // SCENE_HIGHLIGHTER_HPP
