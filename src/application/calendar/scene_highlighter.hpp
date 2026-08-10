#ifndef SCENE_HIGHLIGHTER_HPP
#define SCENE_HIGHLIGHTER_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "../../domain/shape_configuration.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"
#include "../../infrastructure/graphics/rect.hpp"
#include "../../infrastructure/graphics/scene.hpp"
#include "../../infrastructure/graphics/scene_graph.hpp"
#include "../../infrastructure/graphics/shape_node.hpp"
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
  SceneHighlighter(const Scene& scene, const ShapeNode<FillShape>& overlay_node,
                   const ShapeNode<BoxesShape>& title_area_node,
                   const ShapeConfigSet& shape_config);

  // Adopts the bar nodes from the latest rebuild and re-applies the persisted
  // hover and selection highlights to the fresh geometry.
  void Refresh(
      std::unordered_map<std::size_t, std::shared_ptr<SceneNode>> bar_nodes);

  // Highlights the hovered element (and restores the previously hovered one) by
  // recolouring its shape in place — no scene rebuild. A null value clears it.
  void SetHovered(const std::optional<PickId>& hovered);

  // The scene node a hit element means — empty when its index points into the
  // void after a rebuild. The highlighter keeps the nodes anyway, so nobody has
  // to hold them a second time.
  [[nodiscard]] std::shared_ptr<SceneNode> NodeFor(const PickId& picked) const;

  // Highlights the scene node identified by `path` (and its subtree) with a
  // translucent overlay — no rebuild. A null/unknown path clears the overlay.
  void SetSelectedNode(const std::optional<std::string>& path);

 private:
  // Positions the selection overlay over the currently selected node's world
  // bounds, or hides it (zero-area quad) when there is no resolvable selection.
  void ApplySelectionOverlay();

  // Resolves a "root/.../name" path to the world-space bounds of that node's
  // subtree (page space, matching the bars' pick boxes). Returns nullopt when
  // any path segment does not resolve or the subtree carries no geometry.
  [[nodiscard]] std::optional<RectF> NodeWorldBounds(
      const std::string& path) const;

  // Recolours the hovered element's outline: highlighted gets the hover accent,
  // otherwise the colours of the configuration its style id points at. Fill is
  // left as configured so the hover reads as an outline accent.
  void ApplyHover(const PickId& picked, bool highlighted);

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
  const ShapeNode<FillShape>& overlay_node_;
  const ShapeNode<BoxesShape>& title_area_node_;
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
