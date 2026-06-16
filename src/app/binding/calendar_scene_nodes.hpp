#ifndef CALENDAR_SCENE_NODES_HPP
#define CALENDAR_SCENE_NODES_HPP

#include <memory>
#include <string>

#include "../../graphics/font.hpp"
#include "../../graphics/scene.hpp"
#include "../../graphics/scene_graph.hpp"
#include "../../graphics/shaders.hpp"
#include "../../graphics/shapes.hpp"

// Painter draw layers for the calendar (lower = further back). The bars sit
// above the grid background so the day/sunday/month cells no longer cover them;
// text sits on top, and the scene-tree selection overlay on top of everything.
// Shared by the skeleton factory (fixed nodes) and the section builders, whose
// dynamic children take the same layers.
namespace calendar_layers {
inline constexpr int kPage = 0;
inline constexpr int kFrame = 10;
inline constexpr int kGrid = 20;
inline constexpr int kBars = 30;
inline constexpr int kText = 40;
inline constexpr int kOverlay = 50;
}  // namespace calendar_layers

// Stable handles to the fixed scene-skeleton nodes, created once and reached
// directly by the section builders. Container nodes (bar cells, the text
// groups, …) carry no shape; their dynamic children are (re)built each frame.
struct CalendarSceneNodes {
  std::shared_ptr<SceneNode> page;
  std::shared_ptr<SceneNode> selection_overlay;
  std::shared_ptr<SceneNode> print_area;
  std::shared_ptr<SceneNode> title_area;
  std::shared_ptr<SceneNode> title_font;
  std::shared_ptr<SceneNode> row_labels;
  std::shared_ptr<SceneNode> column_labels;
  std::shared_ptr<SceneNode> years_cells;
  std::shared_ptr<SceneNode> months_cells;
  std::shared_ptr<SceneNode> days_cells0;
  std::shared_ptr<SceneNode> days_cells1;
  std::shared_ptr<SceneNode> bars_cells;
  std::shared_ptr<SceneNode> years_totals;
  std::shared_ptr<SceneNode> years_totals_text;
  std::shared_ptr<SceneNode> legend_entries;
  std::shared_ptr<SceneNode> legend_text;
  std::shared_ptr<SceneNode> month_text;
  std::shared_ptr<SceneNode> year_text;
  std::shared_ptr<SceneNode> bar_labels;
};

// Builds the fixed scene skeleton under the scene's root once: every named node
// plus its painter layer and parent attachment. Returns the handles for the
// section builders. The hierarchy is page -> print area -> (the calendar's
// areas), with the selection overlay as a hidden top-layer sibling of the page.
[[nodiscard]] inline CalendarSceneNodes BuildCalendarSceneNodes(
    Scene& scene, Shader* simple_shader, Shader* rectangles_shader,
    Shader* font_shader, const std::shared_ptr<Font>& font) {
  CalendarSceneNodes nodes;

  auto page_shape = std::make_shared<QuadrilateralShape>(simple_shader);
  nodes.page = std::make_shared<SceneNode>("page", page_shape);
  scene.Root().AddChild(nodes.page);

  // Selection-highlight overlay: a single translucent quad drawn on top of
  // everything, covering the scene-tree-selected node and its subtree. It is a
  // rendering aid, not part of the user's scene, so it is hidden from the
  // snapshot. Updated in place (no rebuild) when the selection changes.
  auto selection_shape = std::make_shared<QuadrilateralShape>(simple_shader);
  nodes.selection_overlay =
      std::make_shared<SceneNode>("selection overlay", selection_shape);
  nodes.selection_overlay->SetSnapshotHidden(true);
  scene.Root().AddChild(nodes.selection_overlay);

  auto print_area_shape = std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.print_area =
      std::make_shared<SceneNode>("print area", print_area_shape);
  nodes.page->AddChild(nodes.print_area);

  auto title_area_shape = std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.title_area =
      std::make_shared<SceneNode>("title area", title_area_shape);
  nodes.print_area->AddChild(nodes.title_area);

  auto row_labels_shape = std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.row_labels =
      std::make_shared<SceneNode>("row label area", row_labels_shape);
  nodes.print_area->AddChild(nodes.row_labels);

  auto column_labels_shape =
      std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.column_labels =
      std::make_shared<SceneNode>("column label area", column_labels_shape);
  nodes.print_area->AddChild(nodes.column_labels);

  auto years_cells_shape = std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.years_cells =
      std::make_shared<SceneNode>("year cells", years_cells_shape);
  nodes.print_area->AddChild(nodes.years_cells);

  auto months_cells_shape =
      std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.months_cells =
      std::make_shared<SceneNode>("month cells", months_cells_shape);
  nodes.print_area->AddChild(nodes.months_cells);

  auto days_cells0_shape = std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.days_cells0 =
      std::make_shared<SceneNode>("day cells 0", days_cells0_shape);
  nodes.print_area->AddChild(nodes.days_cells0);

  auto days_cells1_shape = std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.days_cells1 =
      std::make_shared<SceneNode>("day cells 1", days_cells1_shape);
  nodes.print_area->AddChild(nodes.days_cells1);

  nodes.bars_cells = std::make_shared<SceneNode>("bar cells");
  nodes.print_area->AddChild(nodes.bars_cells);

  auto years_totals_shape =
      std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.years_totals =
      std::make_shared<SceneNode>("year total cells", years_totals_shape);
  nodes.print_area->AddChild(nodes.years_totals);

  nodes.years_totals_text = std::make_shared<SceneNode>("year total text");
  nodes.print_area->AddChild(nodes.years_totals_text);

  auto legend_shape = std::make_shared<RectanglesShape>(rectangles_shader);
  auto legend_area = std::make_shared<SceneNode>("legend area", legend_shape);
  nodes.print_area->AddChild(legend_area);

  nodes.legend_entries = std::make_shared<SceneNode>("legend entries");
  nodes.print_area->AddChild(nodes.legend_entries);

  nodes.legend_text = std::make_shared<SceneNode>("legend text");
  nodes.print_area->AddChild(nodes.legend_text);

  auto title_font_shape = std::make_shared<FontShape>(font_shader);
  title_font_shape->SetFont(font);
  nodes.title_font =
      std::make_shared<SceneNode>("title text", title_font_shape);
  nodes.print_area->AddChild(nodes.title_font);

  nodes.month_text = std::make_shared<SceneNode>("month text");
  nodes.print_area->AddChild(nodes.month_text);

  nodes.year_text = std::make_shared<SceneNode>("year text");
  nodes.print_area->AddChild(nodes.year_text);

  nodes.bar_labels = std::make_shared<SceneNode>("bar labels");
  nodes.print_area->AddChild(nodes.bar_labels);

  nodes.page->SetDrawLayer(calendar_layers::kPage);
  nodes.print_area->SetDrawLayer(calendar_layers::kFrame);
  nodes.title_area->SetDrawLayer(calendar_layers::kFrame);
  legend_area->SetDrawLayer(calendar_layers::kFrame);
  nodes.row_labels->SetDrawLayer(calendar_layers::kGrid);
  nodes.column_labels->SetDrawLayer(calendar_layers::kGrid);
  nodes.years_cells->SetDrawLayer(calendar_layers::kGrid);
  nodes.months_cells->SetDrawLayer(calendar_layers::kGrid);
  nodes.days_cells0->SetDrawLayer(calendar_layers::kGrid);
  nodes.days_cells1->SetDrawLayer(calendar_layers::kGrid);
  nodes.years_totals->SetDrawLayer(calendar_layers::kBars);
  nodes.title_font->SetDrawLayer(calendar_layers::kText);
  nodes.selection_overlay->SetDrawLayer(calendar_layers::kOverlay);

  return nodes;
}

#endif  // CALENDAR_SCENE_NODES_HPP
