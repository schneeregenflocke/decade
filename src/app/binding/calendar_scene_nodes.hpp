#ifndef CALENDAR_SCENE_NODES_HPP
#define CALENDAR_SCENE_NODES_HPP

#include <memory>
#include <string>
#include <string_view>

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
  std::shared_ptr<SceneNode> page;

  static constexpr std::string_view kSelectionOverlayName = "Selection Overlay";
  std::shared_ptr<SceneNode> selection_overlay;

  static constexpr std::string_view kPrintAreaName = "Print Area";
  std::shared_ptr<SceneNode> print_area;

  static constexpr std::string_view kTitleFrameName = "Title Frame";
  std::shared_ptr<SceneNode> title_frame;

  static constexpr std::string_view kTitleTextName = "Title Text";
  std::shared_ptr<SceneNode> title_text;

  static constexpr std::string_view kRowLabelsName = "Row Labels";
  std::shared_ptr<SceneNode> row_labels;

  static constexpr std::string_view kColumnLabelsName = "Column Labels";
  std::shared_ptr<SceneNode> column_labels;

  static constexpr std::string_view kYearCellsName = "Year Cells";
  std::shared_ptr<SceneNode> year_cells;

  static constexpr std::string_view kMonthCellsName = "Month Cells";
  std::shared_ptr<SceneNode> month_cells;

  static constexpr std::string_view kDayCellsName = "Day Cells";
  std::shared_ptr<SceneNode> day_cells;

  static constexpr std::string_view kSundayCellsName = "Sunday Cells";
  std::shared_ptr<SceneNode> sunday_cells;

  static constexpr std::string_view kDateBarsName = "Date Bars";
  std::shared_ptr<SceneNode> date_bars;

  static constexpr std::string_view kYearTotalsName = "Year Totals";
  std::shared_ptr<SceneNode> year_totals;

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
    Scene& scene, Shader* simple_shader, Shader* rectangles_shader,
    Shader* font_shader, const std::shared_ptr<Font>& font) {
  CalendarSceneNodes nodes;

  auto page_shape = std::make_shared<QuadrilateralShape>(simple_shader);
  nodes.page = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kPageName), page_shape);
  scene.Root().AddChild(nodes.page);

  // Selection-highlight overlay: a single translucent quad drawn on top of
  // everything, covering the scene-tree-selected node and its subtree. It is a
  // rendering aid, not part of the user's scene, so it is hidden from the
  // snapshot. Updated in place (no rebuild) when the selection changes.
  auto selection_shape = std::make_shared<QuadrilateralShape>(simple_shader);
  nodes.selection_overlay = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kSelectionOverlayName), selection_shape);
  nodes.selection_overlay->SetSnapshotHidden(true);
  scene.Root().AddChild(nodes.selection_overlay);

  auto print_area_shape = std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.print_area = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kPrintAreaName), print_area_shape);
  nodes.page->AddChild(nodes.print_area);

  auto title_frame_shape = std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.title_frame = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kTitleFrameName), title_frame_shape);
  nodes.print_area->AddChild(nodes.title_frame);

  auto row_labels_shape = std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.row_labels = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kRowLabelsName), row_labels_shape);
  nodes.print_area->AddChild(nodes.row_labels);

  auto column_labels_shape =
      std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.column_labels = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kColumnLabelsName), column_labels_shape);
  nodes.print_area->AddChild(nodes.column_labels);

  auto year_cells_shape = std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.year_cells = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kYearCellsName), year_cells_shape);
  nodes.print_area->AddChild(nodes.year_cells);

  auto month_cells_shape = std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.month_cells = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kMonthCellsName), month_cells_shape);
  nodes.print_area->AddChild(nodes.month_cells);

  auto day_cells_shape = std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.day_cells = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kDayCellsName), day_cells_shape);
  nodes.print_area->AddChild(nodes.day_cells);

  auto sunday_cells_shape =
      std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.sunday_cells = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kSundayCellsName), sunday_cells_shape);
  nodes.print_area->AddChild(nodes.sunday_cells);

  nodes.date_bars = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kDateBarsName));
  nodes.print_area->AddChild(nodes.date_bars);

  auto year_totals_shape = std::make_shared<RectanglesShape>(rectangles_shader);
  nodes.year_totals = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kYearTotalsName), year_totals_shape);
  nodes.print_area->AddChild(nodes.year_totals);

  nodes.year_total_labels = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kYearTotalLabelsName));
  nodes.print_area->AddChild(nodes.year_total_labels);

  auto legend_frame_shape =
      std::make_shared<RectanglesShape>(rectangles_shader);
  auto legend_frame = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kLegendFrameName), legend_frame_shape);
  nodes.print_area->AddChild(legend_frame);

  nodes.legend_entries = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kLegendEntriesName));
  nodes.print_area->AddChild(nodes.legend_entries);

  nodes.legend_labels = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kLegendLabelsName));
  nodes.print_area->AddChild(nodes.legend_labels);

  auto title_text_shape = std::make_shared<FontShape>(font_shader);
  title_text_shape->SetFont(font);
  nodes.title_text = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kTitleTextName), title_text_shape);
  nodes.print_area->AddChild(nodes.title_text);

  nodes.month_labels = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kMonthLabelsName));
  nodes.print_area->AddChild(nodes.month_labels);

  nodes.year_labels = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kYearLabelsName));
  nodes.print_area->AddChild(nodes.year_labels);

  nodes.date_bar_labels = std::make_shared<SceneNode>(
      std::string(CalendarSceneNodes::kDateBarLabelsName));
  nodes.print_area->AddChild(nodes.date_bar_labels);

  nodes.page->SetDrawLayer(calendar_layers::kPage);
  nodes.print_area->SetDrawLayer(calendar_layers::kFrame);
  nodes.title_frame->SetDrawLayer(calendar_layers::kFrame);
  legend_frame->SetDrawLayer(calendar_layers::kFrame);
  nodes.row_labels->SetDrawLayer(calendar_layers::kGrid);
  nodes.column_labels->SetDrawLayer(calendar_layers::kGrid);
  nodes.year_cells->SetDrawLayer(calendar_layers::kGrid);
  nodes.month_cells->SetDrawLayer(calendar_layers::kGrid);
  nodes.day_cells->SetDrawLayer(calendar_layers::kGrid);
  nodes.sunday_cells->SetDrawLayer(calendar_layers::kGrid);
  nodes.year_totals->SetDrawLayer(calendar_layers::kBars);
  nodes.title_text->SetDrawLayer(calendar_layers::kText);
  nodes.selection_overlay->SetDrawLayer(calendar_layers::kOverlay);

  return nodes;
}

#endif  // CALENDAR_SCENE_NODES_HPP
