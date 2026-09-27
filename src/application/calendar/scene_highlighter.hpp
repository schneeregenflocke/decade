#ifndef SCENE_HIGHLIGHTER_HPP
#define SCENE_HIGHLIGHTER_HPP

#include <cstddef>
#include <glm/mat4x4.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

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
//   * the hovered element (a date entry or the title frame), recoloured in
//     place via its shapes; and
//   * the selected nodes, covered by translucent overlay boxes.
//
// Both are applied without a scene rebuild. The coordinator calls Refresh()
// once per rebuild to hand over the freshly built entry nodes and re-apply the
// persisted highlights to the new geometry.
class SceneHighlighter {
 public:
  SceneHighlighter(const Scene& scene, const ShapeNode<BoxesShape>& overlay_node,
                   const ShapeNode<BoxesShape>& title_area_node,
                   const ShapeConfigSet& shape_config);

  // Adopts the entry nodes from the latest rebuild, indexed like the entries,
  // and re-applies the persisted hover and selection highlights.
  void Refresh(std::vector<std::shared_ptr<SceneNode>> entry_nodes);

  // Highlights the hovered element (and restores the previously hovered one) by
  // recolouring its shapes in place — no scene rebuild. A null value clears it.
  void SetHovered(const std::optional<PickId>& hovered);

  // The scene node a hit element means — empty when its index points into the
  // void after a rebuild. The highlighter keeps the nodes anyway, so nobody has
  // to hold them a second time.
  [[nodiscard]] std::shared_ptr<SceneNode> NodeFor(const PickId& picked) const;

  // Covers the nodes the paths identify with the overlay — no rebuild. A path
  // that resolves to nothing covers nothing.
  void SetSelectedNodes(const std::vector<std::string>& paths);

 private:
  // A node the scene holds, with the world transform of its parent.
  struct PlacedNode {
    const SceneNode* node;
    glm::mat4 parent_world;
  };

  void ApplySelectionOverlay();

  // Resolves a "root/.../name" path; empty when a segment does not resolve.
  [[nodiscard]] std::optional<PlacedNode> Resolve(
      const std::string& path) const;

  // The page-space boxes that show a node as selected. A drawing node gets one
  // box around its subtree; a group dissolves into its members, so an entry
  // whose bar breaks across rows shows its pieces rather than one box spanning
  // the rows between them.
  static void AppendCoverage(const PlacedNode& placed,
                             std::vector<RectF>& boxes);

  // Recolours the outlines of the element's boxes: highlighted gets the hover
  // accent, otherwise the colours of the style each box shows. Fill is left as
  // configured so the hover reads as an outline accent.
  void ApplyHover(const PickId& picked, bool highlighted);

  static constexpr float kZero = 0.0F;
  static constexpr float kOne = 1.0F;
  static constexpr float kHoverOutlineGreen = 0.55F;

  // Translucent accent for the selection overlay.
  static constexpr float kSelectionRed = 1.0F;
  static constexpr float kSelectionGreen = 0.6F;
  static constexpr float kSelectionBlue = 0.0F;
  static constexpr float kSelectionAlpha = 0.35F;

  // Borrowed (non-owning) collaborators, owned by CalendarPage / the builder.
  const Scene& scene_;
  const ShapeNode<BoxesShape>& overlay_node_;
  const ShapeNode<BoxesShape>& title_area_node_;
  const ShapeConfigSet& shape_config_;

  std::vector<std::shared_ptr<SceneNode>> entry_nodes_;
  std::optional<PickId> hovered_;
  // Paths of the selected nodes ("root/.../name"); they persist across
  // rebuilds so the overlay is re-applied to the fresh geometry.
  std::vector<std::string> selected_paths_;
};

#endif  // SCENE_HIGHLIGHTER_HPP
