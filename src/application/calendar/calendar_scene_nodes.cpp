#include "calendar_scene_nodes.hpp"

#include <memory>
#include <string>
#include <string_view>

#include "../../infrastructure/graphics/font.hpp"
#include "../../infrastructure/graphics/scene.hpp"
#include "../../infrastructure/graphics/scene_graph.hpp"
#include "../../infrastructure/graphics/shaders.hpp"
#include "../../infrastructure/graphics/shape_node.hpp"
#include "../../infrastructure/graphics/shapes.hpp"

CalendarSceneNodes BuildCalendarSceneNodes(Scene& scene, Shader& simple_shader,
                                           Shader& rectangles_shader,
                                           Shader& font_shader,
                                           const std::shared_ptr<Font>& font) {
  CalendarSceneNodes nodes;

  nodes.page = ShapeNode<FillShape>::Make(
      std::string(CalendarSceneNodes::kPageName), simple_shader);
  scene.Root().AddChild(nodes.page.Node());

  // Selection-highlight overlay: a single translucent quad drawn on top of
  // everything, covering the scene-tree-selected node and its subtree. It is a
  // rendering aid, not part of the user's scene, so it is hidden from the
  // snapshot. Updated in place (no rebuild) when the selection changes.
  nodes.selection_overlay = ShapeNode<FillShape>::Make(
      std::string(CalendarSceneNodes::kSelectionOverlayName), simple_shader);
  nodes.selection_overlay.Node()->SetSnapshotHidden(true);
  scene.Root().AddChild(nodes.selection_overlay.Node());

  nodes.print_area = ShapeNode<BoxesShape>::Make(
      std::string(CalendarSceneNodes::kPrintAreaName), rectangles_shader);
  nodes.page.Node()->AddChild(nodes.print_area.Node());

  // Everything below hangs under the print area. The handle is fetched once so
  // the attachments below read as one list rather than repeating the path.
  const std::shared_ptr<SceneNode>& print_area = nodes.print_area.Node();

  const auto boxes_under_print_area = [&](std::string_view name) {
    auto node =
        ShapeNode<BoxesShape>::Make(std::string(name), rectangles_shader);
    print_area->AddChild(node.Node());
    return node;
  };
  const auto fill_under_print_area = [&](std::string_view name) {
    auto node = ShapeNode<FillShape>::Make(std::string(name), simple_shader);
    print_area->AddChild(node.Node());
    return node;
  };
  const auto container_under_print_area = [&](std::string_view name) {
    auto node = std::make_shared<SceneNode>(std::string(name));
    print_area->AddChild(node);
    return node;
  };

  nodes.title_area =
      boxes_under_print_area(CalendarSceneNodes::kTitleFrameName);
  nodes.row_labels = boxes_under_print_area(CalendarSceneNodes::kRowLabelsName);
  nodes.column_labels =
      boxes_under_print_area(CalendarSceneNodes::kColumnLabelsName);
  nodes.year_cells = boxes_under_print_area(CalendarSceneNodes::kYearCellsName);
  nodes.month_cells =
      boxes_under_print_area(CalendarSceneNodes::kMonthCellsName);
  nodes.day_cells = boxes_under_print_area(CalendarSceneNodes::kDayCellsName);
  nodes.sunday_cells =
      boxes_under_print_area(CalendarSceneNodes::kSundayCellsName);
  nodes.date_bars =
      container_under_print_area(CalendarSceneNodes::kDateBarsName);
  nodes.year_totals =
      boxes_under_print_area(CalendarSceneNodes::kYearTotalsName);
  nodes.year_total_labels =
      container_under_print_area(CalendarSceneNodes::kYearTotalLabelsName);

  // A leaf with no later updates, hence no handle in the struct — but it needs
  // its draw layer below, so it stays named here.
  const ShapeNode<BoxesShape> legend_area =
      boxes_under_print_area(CalendarSceneNodes::kLegendFrameName);

  nodes.legend_entries =
      container_under_print_area(CalendarSceneNodes::kLegendEntriesName);
  nodes.legend_labels =
      container_under_print_area(CalendarSceneNodes::kLegendLabelsName);

  nodes.title_text = ShapeNode<FontShape>::Make(
      std::string(CalendarSceneNodes::kTitleTextName), font_shader);
  nodes.title_text.Shape().SetFont(font);
  print_area->AddChild(nodes.title_text.Node());

  nodes.title_selection =
      fill_under_print_area(CalendarSceneNodes::kTitleSelectionName);
  nodes.title_selection.Node()->SetSnapshotHidden(true);

  nodes.title_caret =
      fill_under_print_area(CalendarSceneNodes::kTitleCaretName);
  nodes.title_caret.Node()->SetSnapshotHidden(true);

  nodes.month_labels =
      container_under_print_area(CalendarSceneNodes::kMonthLabelsName);
  nodes.year_labels =
      container_under_print_area(CalendarSceneNodes::kYearLabelsName);
  nodes.date_bar_labels =
      container_under_print_area(CalendarSceneNodes::kDateBarLabelsName);

  nodes.page.Node()->SetDrawLayer(calendar_layers::kPage);
  nodes.print_area.Node()->SetDrawLayer(calendar_layers::kArea);
  nodes.title_area.Node()->SetDrawLayer(calendar_layers::kArea);
  legend_area.Node()->SetDrawLayer(calendar_layers::kArea);
  nodes.row_labels.Node()->SetDrawLayer(calendar_layers::kGrid);
  nodes.column_labels.Node()->SetDrawLayer(calendar_layers::kGrid);
  nodes.year_cells.Node()->SetDrawLayer(calendar_layers::kGrid);
  nodes.month_cells.Node()->SetDrawLayer(calendar_layers::kGrid);
  nodes.day_cells.Node()->SetDrawLayer(calendar_layers::kGrid);
  nodes.sunday_cells.Node()->SetDrawLayer(calendar_layers::kGrid);
  nodes.year_totals.Node()->SetDrawLayer(calendar_layers::kBars);
  nodes.title_text.Node()->SetDrawLayer(calendar_layers::kText);
  nodes.title_selection.Node()->SetDrawLayer(calendar_layers::kTextSelection);
  nodes.title_caret.Node()->SetDrawLayer(calendar_layers::kCaret);
  nodes.selection_overlay.Node()->SetDrawLayer(calendar_layers::kOverlay);

  return nodes;
}
