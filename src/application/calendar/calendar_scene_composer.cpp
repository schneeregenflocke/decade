#include "calendar_scene_composer.hpp"

#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../../domain/calendar_config.hpp"
#include "../../domain/date_entry_bars.hpp"
#include "../../domain/date_group.hpp"
#include "../../domain/font_config.hpp"
#include "../../domain/scene_snapshot.hpp"
#include "../../domain/shape_configuration.hpp"
#include "../../domain/text_edit_view.hpp"
#include "../../domain/title_config.hpp"
#include "../../infrastructure/graphics/font.hpp"
#include "../../infrastructure/graphics/graphics_engine.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"
#include "../../infrastructure/graphics/rect.hpp"
#include "../../infrastructure/graphics/scene.hpp"
#include "../../infrastructure/graphics/scene_graph.hpp"
#include "../../infrastructure/graphics/shaders.hpp"
#include "../../infrastructure/graphics/shapes.hpp"
#include "bar_sections.hpp"
#include "calendar_layout.hpp"
#include "calendar_scene_nodes.hpp"
#include "grid_sections.hpp"
#include "legend_section.hpp"
#include "scene_highlighter.hpp"
#include "scene_snapshot_builder.hpp"
#include "section_context.hpp"
#include "title_section.hpp"

CalendarSceneComposer::CalendarSceneComposer(
    GraphicsEngine& graphics_engine_in, Scene& scene_in,
    const std::shared_ptr<Font>& font_in, const FontConfig& font_config_in,
    const RectF& page_size_in, const RectF& page_margin_in,
    const TitleConfig& title_config_in, CalendarConfig& calendar_config_in,
    const ShapeConfigSet& shape_config_in, const DateGroups& date_groups_in,
    const DateEntryBars& date_entry_bars_in)
    : scene_(scene_in),
      graphics_engine_(graphics_engine_in),
      rectangles_shader_(
          RequireShader(graphics_engine_in, "Rectangles Shader")),
      font_shader_(RequireShader(graphics_engine_in, "Font Shader")),
      font_(font_in),
      font_config_(font_config_in),
      page_size_(page_size_in),
      page_margin_(page_margin_in),
      title_config_(title_config_in),
      calendar_config_(calendar_config_in),
      shape_config_(shape_config_in),
      date_groups_(date_groups_in),
      date_entry_bars_(date_entry_bars_in) {
  graphics_engine_.SetScene(scene_);
  Shader& simple_shader = RequireShader(graphics_engine_, "Simple Shader");

  // The fixed scene skeleton (named nodes, their painter layers and parent
  // attachments) is built once here; the handles drive the section builders.
  nodes_ = BuildCalendarSceneNodes(scene_, simple_shader, rectangles_shader_,
                                   font_shader_, font_);
}

Shader& CalendarSceneComposer::RequireShader(GraphicsEngine& graphics_engine,
                                             const std::string& name) {
  const auto found = graphics_engine.SearchShader(name);
  if (!found.has_value()) {
    throw std::runtime_error("shader not found: " + name);
  }
  return found->get();
}

void CalendarSceneComposer::Build() {
  FillShape& page_shape = nodes_.page.Shape();
  page_shape.SetShape(page_size_);
  page_shape.SetColor(glm::vec4(kOne, kOne, kOne, kOne));

  // The auto span derives the calendar's year range from the data; it must
  // run before the layout, which sizes the rows from the span length.
  if (calendar_config_.IsAutoCalendarSpan() && !date_entry_bars_.is_empty()) {
    calendar_config_.SetSpan(
        CalendarSpan::YearSpan{.first_year = date_entry_bars_.GetFirstYear(),
                               .last_year = date_entry_bars_.GetLastYear()});
  }

  layout_ = CalendarLayout(page_size_, page_margin_, title_config_.AreaHeight(),
                           calendar_config_.GetSpanLengthYears(),
                           calendar_config_.GetSpacingProportions());

  // The print-area node carries the print area's offset within the page;
  // every descendant is computed in print-area-local coordinates (origin at
  // the print area's bottom-left). The page rectangle itself stays in
  // absolute page space on the untransformed page node above.
  nodes_.print_area.Node()->SetModelMatrix(
      glm::translate(glm::mat4(1.0F), layout_.PrintAreaOrigin()));

  const calendar_sections::SectionContext ctx = MakeContext();
  calendar_sections::BuildPrintArea(ctx);
  pick_boxes_.clear();
  pick_boxes_.push_back(calendar_sections::BuildTitle(ctx));
  calendar_sections::BuildCalendarLabels(ctx);
  calendar_sections::BuildDays(ctx);
  calendar_sections::BuildMonths(ctx);
  calendar_sections::BuildYears(ctx);
  calendar_sections::BarSceneResult bars = calendar_sections::BuildBars(ctx);
  pick_boxes_.insert(pick_boxes_.end(), bars.pick_boxes.begin(),
                     bars.pick_boxes.end());
  calendar_sections::BuildYearTotals(ctx);
  calendar_sections::BuildLegend(ctx);

  // Hand the fresh bar nodes to the highlighter, which re-applies the
  // persisted hover and selection highlights to the new geometry.
  highlighter_.Refresh(std::move(bars.bar_nodes));
}

calendar_sections::SectionContext CalendarSceneComposer::MakeContext() const {
  return calendar_sections::SectionContext{
      .nodes = nodes_,
      .layout = layout_,
      .shape_config = shape_config_,
      .calendar_config = calendar_config_,
      .title_config = title_config_,
      .date_groups = date_groups_,
      .date_entry_bars = date_entry_bars_,
      .text_edit = text_edit_,
      .font = font_,
      .font_config = font_config_,
      .rectangles_shader = rectangles_shader_,
      .font_shader = font_shader_};
}

SceneNodeSnapshot CalendarSceneComposer::SceneSnapshot() const {
  return BuildSceneSnapshot(scene_.Root());
}

const std::vector<PickBox>& CalendarSceneComposer::PickBoxes() const {
  return pick_boxes_;
}

void CalendarSceneComposer::SetHovered(const std::optional<PickId>& hovered) {
  highlighter_.SetHovered(hovered);
}

void CalendarSceneComposer::SetSelectedNode(
    const std::optional<std::string>& path) {
  highlighter_.SetSelectedNode(path);
}

void CalendarSceneComposer::SetTextEdit(
    const std::optional<TextEditView>& text_edit) {
  text_edit_ = text_edit;
}

std::optional<std::string> CalendarSceneComposer::NodePathFor(
    const PickId& picked) const {
  const auto node = highlighter_.NodeFor(picked);
  if (!node) {
    return std::nullopt;
  }
  return FindNodePath(scene_.Root(), *node);
}

std::size_t CalendarSceneComposer::TitleCaretIndexAt(
    glm::vec2 page_point) const {
  const calendar_sections::SectionContext ctx = MakeContext();
  return calendar_sections::title_edit::CaretIndexAt(
      ctx, calendar_sections::title_edit::Layout(ctx), page_point);
}
