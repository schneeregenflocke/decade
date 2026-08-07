#ifndef CALENDAR_SCENE_NODES_HPP
#define CALENDAR_SCENE_NODES_HPP

#include <memory>
#include <string>
#include <string_view>

#include "../../infrastructure/graphics/font.hpp"
#include "../../infrastructure/graphics/scene.hpp"
#include "../../infrastructure/graphics/scene_graph.hpp"
#include "../../infrastructure/graphics/shaders.hpp"
#include "../../infrastructure/graphics/shape_node.hpp"
#include "../../infrastructure/graphics/shapes.hpp"

// Painter draw layers for the calendar (lower = further back). The bars sit
// above the grid background so the day/sunday/month cells no longer cover them;
// text sits on top, and the scene-tree selection overlay on top of everything.
// Shared by the skeleton factory (fixed nodes) and the section builders, whose
// dynamic children take the same layers.
namespace calendar_layers {
inline constexpr int kPage = 0;
inline constexpr int kArea = 10;
inline constexpr int kGrid = 20;
inline constexpr int kBars = 30;
inline constexpr int kTextSelection = 35;
inline constexpr int kText = 40;
inline constexpr int kCaret = 45;
inline constexpr int kOverlay = 50;
}  // namespace calendar_layers

// Stable handles to the fixed scene-skeleton nodes, created once and reached
// directly by the section builders. Container nodes (the date bars, the text
// groups, …) carry no shape; their dynamic children are (re)built each frame.
//
// Each node's display name lives here as a `k…Name` constant right next to its
// handle, so the name and the variable are defined together in one place (the
// single source of truth). The factory below wires both; these names are what
// the scene-tree panel shows, so they are written for humans (Title Case,
// describing the node's purpose).
struct CalendarSceneNodes {
  static constexpr std::string_view kPageName = "Page";
  ShapeNode<FillShape> page;

  static constexpr std::string_view kSelectionOverlayName = "Selection Overlay";
  ShapeNode<FillShape> selection_overlay;

  static constexpr std::string_view kPrintAreaName = "Print Area";
  ShapeNode<BoxesShape> print_area;

  static constexpr std::string_view kTitleFrameName = "Title Frame";
  ShapeNode<BoxesShape> title_area;

  static constexpr std::string_view kTitleTextName = "Title Text";
  ShapeNode<FontShape> title_text;

  // The cursor and selection area of the running title edit. Pure operating
  // aids, and therefore hidden from the user's scene tree.
  static constexpr std::string_view kTitleSelectionName = "Title Selection";
  ShapeNode<FillShape> title_selection;

  static constexpr std::string_view kTitleCaretName = "Title Caret";
  ShapeNode<FillShape> title_caret;

  static constexpr std::string_view kRowLabelsName = "Row Labels";
  ShapeNode<BoxesShape> row_labels;

  static constexpr std::string_view kColumnLabelsName = "Column Labels";
  ShapeNode<BoxesShape> column_labels;

  static constexpr std::string_view kYearCellsName = "Year Cells";
  ShapeNode<BoxesShape> year_cells;

  static constexpr std::string_view kMonthCellsName = "Month Cells";
  ShapeNode<BoxesShape> month_cells;

  static constexpr std::string_view kDayCellsName = "Day Cells";
  ShapeNode<BoxesShape> day_cells;

  static constexpr std::string_view kSundayCellsName = "Sunday Cells";
  ShapeNode<BoxesShape> sunday_cells;

  static constexpr std::string_view kDateBarsName = "Date Bars";
  std::shared_ptr<SceneNode> date_bars;

  static constexpr std::string_view kYearTotalsName = "Year Totals";
  ShapeNode<BoxesShape> year_totals;

  static constexpr std::string_view kYearTotalLabelsName = "Year Total Labels";
  std::shared_ptr<SceneNode> year_total_labels;

  static constexpr std::string_view kLegendFrameName = "Legend Frame";
  // The legend frame is a leaf with no later updates, so it has no handle here;
  // its name constant lives with the others to keep them all in one place.

  static constexpr std::string_view kLegendEntriesName = "Legend Entries";
  std::shared_ptr<SceneNode> legend_entries;

  static constexpr std::string_view kLegendLabelsName = "Legend Labels";
  std::shared_ptr<SceneNode> legend_labels;

  static constexpr std::string_view kMonthLabelsName = "Month Labels";
  std::shared_ptr<SceneNode> month_labels;

  static constexpr std::string_view kYearLabelsName = "Year Labels";
  std::shared_ptr<SceneNode> year_labels;

  static constexpr std::string_view kDateBarLabelsName = "Date Bar Labels";
  std::shared_ptr<SceneNode> date_bar_labels;
};

// Builds the fixed scene skeleton under the scene's root once: every named node
// plus its painter layer and parent attachment. Returns the handles for the
// section builders. The hierarchy is page -> print area -> (the calendar's
// areas), with the selection overlay as a hidden top-layer sibling of the page.
[[nodiscard]] inline CalendarSceneNodes BuildCalendarSceneNodes(
    Scene& scene, Shader& simple_shader, Shader& rectangles_shader,
    Shader& font_shader, const std::shared_ptr<Font>& font) {
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

#endif  // CALENDAR_SCENE_NODES_HPP
