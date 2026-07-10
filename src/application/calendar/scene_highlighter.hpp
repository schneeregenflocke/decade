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

#include "../../infrastructure/graphics/pick_id.hpp"
#include "../../infrastructure/graphics/rect.hpp"
#include "../../infrastructure/graphics/scene.hpp"
#include "../../infrastructure/graphics/scene_graph.hpp"
#include "../../infrastructure/graphics/shapes.hpp"
#include "../../domain/date_entry_bar_store.hpp"
#include "../../domain/shape_configuration.hpp"

// Application/Infrastructure bridge: the interactive highlighting of the
// calendar scene, kept apart from the construction concern
// (CalendarSceneComposer / section builders). It owns the two transient
// highlights:
//
//   * the hovered bar, recoloured in place via its shape; and
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
                   const ShapeConfigSet& shape_config,
                   const DateEntryBarStore& bar_store)
      : scene_(scene),
        overlay_node_(overlay_node),
        shape_config_(shape_config),
        bar_store_(bar_store) {}

  // Adopts the bar nodes from the latest rebuild and re-applies the persisted
  // hover and selection highlights to the fresh geometry.
  void Refresh(
      std::unordered_map<std::size_t, std::shared_ptr<SceneNode>> bar_nodes) {
    bar_nodes_ = std::move(bar_nodes);
    if (hovered_bar_.has_value()) {
      ApplyBarColor(hovered_bar_->index, /*highlighted=*/true);
    }
    ApplySelectionOverlay();
  }

  // Highlights the hovered bar (and restores the previously hovered one) by
  // recolouring its shape in place — no scene rebuild. A null value clears it.
  void SetHoveredBar(const std::optional<PickId>& hovered) {
    if (hovered == hovered_bar_) {
      return;
    }
    if (hovered_bar_.has_value()) {
      ApplyBarColor(hovered_bar_->index, /*highlighted=*/false);
    }
    hovered_bar_ = hovered;
    if (hovered_bar_.has_value()) {
      ApplyBarColor(hovered_bar_->index, /*highlighted=*/true);
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

  // Recolours one bar's shape: highlighted bars get a distinct outline, normal
  // bars are restored to their group's configured colours. Fill is left as
  // configured so the hover reads as an outline accent.
  void ApplyBarColor(std::size_t bar_index, bool highlighted) {
    const auto iterator = bar_nodes_.find(bar_index);
    if (iterator == bar_nodes_.end() ||
        bar_index >= bar_store_.GetNumberBars()) {
      return;
    }
    auto shape = std::dynamic_pointer_cast<RectanglesShape>(
        iterator->second->GetShape());
    if (!shape) {
      return;
    }
    const auto group =
        static_cast<std::size_t>(bar_store_.GetBar(bar_index).GetGroup());
    const auto config = shape_config_.GetDynamicConfiguration(group);
    if (highlighted) {
      const glm::vec4 hover_outline(kOne, kHoverOutlineGreen, kZero, kOne);
      shape->SetColor({hover_outline, config.FillColor()});
    } else {
      shape->SetColor({config.OutlineColor(), config.FillColor()});
    }
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
  const ShapeConfigSet& shape_config_;
  const DateEntryBarStore& bar_store_;

  // Bar nodes by index from the latest rebuild, for the in-place hover
  // recolour.
  std::unordered_map<std::size_t, std::shared_ptr<SceneNode>> bar_nodes_;
  std::optional<PickId> hovered_bar_;
  // Path of the scene-tree-selected node ("root/.../name"); persists across
  // rebuilds so the overlay is re-applied to the fresh geometry.
  std::optional<std::string> selected_path_;
};

#endif  // SCENE_HIGHLIGHTER_HPP
